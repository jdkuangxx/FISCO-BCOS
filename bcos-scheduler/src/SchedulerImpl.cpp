#include "SchedulerImpl.h"
#include "Common.h"
#include "bcos-framework/interfaces/ledger/LedgerConfig.h"
#include "bcos-framework/interfaces/protocol/ProtocolTypeDef.h"
#include <bcos-utilities/Error.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>
#include <memory>
#include <mutex>
#include <variant>

using namespace bcos::scheduler;

// Note: this function only called when undeterministic-recovery
void SchedulerImpl::getExecResult(bcos::protocol::BlockNumber _blockNumber,
    std::function<void(bcos::Error::Ptr&&, bcos::protocol::Block::Ptr&&)> _callback)
{
    SCHEDULER_LOG(INFO) << LOG_DESC("getExecResult") << LOG_KV("index", _blockNumber);
    std::unique_lock<std::mutex> execLock(m_executeMutex);
    std::unique_lock<std::mutex> blocksLock(m_blocksMutex);
    {
        ReadGuard l(x_executedBlocks);
        if (m_executedBlocks.count(_blockNumber))
        {
            auto block = m_executedBlocks.at(_blockNumber);
            _callback(nullptr, std::move(block));
            return;
        }
    }
    if (m_blocks.empty())
    {
        _callback(std::make_shared<bcos::Error>(-1, "Not executed block"), nullptr);
        return;
    }
    auto& frontBlock = m_blocks.front();
    auto& backBlock = m_blocks.back();
    // Block already executed
    if (_blockNumber >= frontBlock->number() && _blockNumber <= backBlock->number())
    {
        SCHEDULER_LOG(INFO) << "getExecResult success, return executed block"
                            << LOG_KV("block number", _blockNumber);

        auto it = m_blocks.begin();
        while (it->get()->number() != _blockNumber)
        {
            ++it;
        }

        SCHEDULER_LOG(TRACE) << "BlockHeader stateRoot: " << std::hex
                             << it->get()->result()->stateRoot();

        auto block = it->get()->block();
        _callback(nullptr, std::move(block));
        return;
    }
    _callback(std::make_shared<bcos::Error>(-1, "Not executed block"), nullptr);
    return;
}

void SchedulerImpl::executeBlock(bcos::protocol::Block::Ptr block, bool verify, bool _clearCache,
    std::function<void(bcos::Error::Ptr&&, bcos::protocol::BlockHeader::Ptr&&)> callback)
{
    uint64_t waitT = 0;
    if (m_lastExecuteFinishTime > 0)
    {
        waitT = utcTime() - m_lastExecuteFinishTime;
    }
    auto signature = block->blockHeaderConst()->signatureList();
    SCHEDULER_LOG(INFO) << "ExecuteBlock request"
                        << LOG_KV("block number", block->blockHeaderConst()->number())
                        << LOG_KV("verify", verify) << LOG_KV("signatureSize", signature.size())
                        << LOG_KV("tx count", block->transactionsSize())
                        << LOG_KV("meta tx count", block->transactionsMetaDataSize())
                        << LOG_KV("clearCache", _clearCache) << LOG_KV("waitT", waitT);
    auto executeLock =
        std::make_shared<std::unique_lock<std::mutex>>(m_executeMutex, std::try_to_lock);
    if (!executeLock->owns_lock())
    {
        auto message = "Another block is executing!";
        SCHEDULER_LOG(ERROR) << "ExecuteBlock error, " << message;
        callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidStatus, message), nullptr);
        return;
    }
    if (_clearCache && !m_blocks.empty())
    {
        std::unique_lock<std::mutex> blocksLock(m_blocksMutex);
        for (auto it = m_blocks.begin(); it != m_blocks.end();)
        {
            auto index = it->get()->block()->blockHeader()->number();
            if (index >= block->blockHeader()->number())
            {
                SCHEDULER_LOG(INFO)
                    << "ExecuteBlock: remove undeterministic block" << LOG_KV("index", index);
                // remove the state
                it->get()->removeAllState();
                removeAllOldPreparedBlock(index);
                {
                    WriteGuard l(x_executedBlocks);
                    m_executedBlocks.insert(std::make_pair(index, it->get()->block()));
                }
                it = m_blocks.erase(it);
                continue;
            }
            it++;
        }
        preExecuteBlock(block, verify);
        m_lastExecutedBlockNumber.store(block->blockHeader()->number() - 1);
    }
    std::unique_lock<std::mutex> blocksLock(m_blocksMutex);
    // Note: if hit the cache, may return synced blockHeader with signatureList in some cases
    if (!m_blocks.empty())
    {
        auto requestNumber = block->blockHeaderConst()->number();
        auto& frontBlock = m_blocks.front();
        auto& backBlock = m_blocks.back();
        // Block already executed
        if (requestNumber >= frontBlock->number() && requestNumber <= backBlock->number())
        {
            SCHEDULER_LOG(INFO) << "ExecuteBlock success, return executed block"
                                << LOG_KV("block number", block->blockHeaderConst()->number())
                                << LOG_KV("signatureSize", signature.size())
                                << LOG_KV("verify", verify)
                                << LOG_KV(
                                       "undeterministic", block->blockHeader()->undeterministic());

            auto it = m_blocks.begin();
            while (it->get()->number() != requestNumber)
            {
                ++it;
            }

            SCHEDULER_LOG(TRACE) << "BlockHeader stateRoot: " << std::hex
                                 << it->get()->result()->stateRoot();

            auto blockHeader = it->get()->result();

            blocksLock.unlock();
            executeLock->unlock();
            callback(nullptr, std::move(blockHeader));
            return;
        }

        if (requestNumber - backBlock->number() != 1)
        {
            auto message =
                "Invalid block number: " +
                boost::lexical_cast<std::string>(block->blockHeaderConst()->number()) +
                " current last number: " + boost::lexical_cast<std::string>(backBlock->number());
            SCHEDULER_LOG(ERROR) << "ExecuteBlock error, " << message;

            blocksLock.unlock();
            executeLock->unlock();
            callback(
                BCOS_ERROR_PTR(SchedulerError::InvalidBlockNumber, std::move(message)), nullptr);

            return;
        }
    }
    else
    {
        auto lastExecutedNumber = m_lastExecutedBlockNumber.load();
        if (lastExecutedNumber != 0 && lastExecutedNumber >= block->blockHeaderConst()->number())
        {
            auto message =
                (boost::format("Try to execute an executed block: %ld, last executed number: %ld") %
                    block->blockHeaderConst()->number() % lastExecutedNumber)
                    .str();
            SCHEDULER_LOG(ERROR) << "ExecuteBlock error, " << message;
            callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidBlockNumber, message), nullptr);
            return;
        }
    }

    BlockExecutive::Ptr blockExecutive = getPreparedBlock(
        block->blockHeaderConst()->number(), block->blockHeaderConst()->timestamp());

    if (blockExecutive == nullptr)
    {
        // the block has not been prepared, just make a new one here
        blockExecutive = std::make_shared<BlockExecutive>(std::move(block), this, 0,
            m_transactionSubmitResultFactory, false, m_blockFactory, verify);
    }
    else
    {
        blockExecutive->block()->setBlockHeader(block->blockHeader());
    }


    m_blocks.emplace_back(blockExecutive);

    blockExecutive = m_blocks.back();

    blocksLock.unlock();
    blockExecutive->asyncExecute([this, callback = std::move(callback), executeLock](
                                     Error::UniquePtr error, protocol::BlockHeader::Ptr header) {
        if (error)
        {
            SCHEDULER_LOG(ERROR) << "Unknown error, " << boost::diagnostic_information(*error);
            {
                std::unique_lock<std::mutex> blocksLock(m_blocksMutex);
                m_blocks.pop_front();
            }
            executeLock->unlock();
            callback(
                BCOS_ERROR_WITH_PREV_PTR(SchedulerError::UnknownError, "Unknown error", *error),
                nullptr);
            return;
        }
        auto signature = header->signatureList();
        SCHEDULER_LOG(INFO) << "ExecuteBlock success" << LOG_KV("block number", header->number())
                            << LOG_KV("hash", header->hash().abridged())
                            << LOG_KV("state root", header->stateRoot().hex())
                            << LOG_KV("receiptRoot", header->receiptsRoot().hex())
                            << LOG_KV("txsRoot", header->txsRoot().abridged())
                            << LOG_KV("gasUsed", header->gasUsed())
                            << LOG_KV("signatureSize", signature.size());

        m_lastExecutedBlockNumber.store(header->number());
        m_lastExecuteFinishTime = utcTime();
        executeLock->unlock();
        callback(std::move(error), std::move(header));
    });
}

void SchedulerImpl::commitBlock(bcos::protocol::BlockHeader::Ptr header,
    std::function<void(bcos::Error::Ptr&&, bcos::ledger::LedgerConfig::Ptr&&)> callback)
{
    auto startT = utcTime();
    SCHEDULER_LOG(INFO) << "CommitBlock request" << LOG_KV("block number", header->number());
    {
        WriteGuard l(x_executedBlocks);
        m_executedBlocks.erase(header->number());
    }
    auto commitLock =
        std::make_shared<std::unique_lock<std::mutex>>(m_commitMutex, std::try_to_lock);
    if (!commitLock->owns_lock())
    {
        std::string message;
        {
            std::unique_lock<std::mutex> blocksLock(m_blocksMutex);
            if (m_blocks.empty())
            {
                message = (boost::format("commitBlock: empty block queue, maybe the block has been "
                                         "committed! Block number: %ld, hash: %s") %
                           header->number() % header->hash().abridged())
                              .str();
            }
            else
            {
                auto& frontBlock = m_blocks.front();
                message =
                    (boost::format(
                         "commitBlock: Another block is committing! Block number: %ld, hash: %s") %
                        frontBlock->block()->blockHeaderConst()->number() %
                        frontBlock->block()->blockHeaderConst()->hash().abridged())
                        .str();
            }
        }
        SCHEDULER_LOG(ERROR) << "CommitBlock error, " << message;
        callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidStatus, message), nullptr);
        return;
    }

    if (m_blocks.empty())
    {
        auto message = "No uncommitted block";
        SCHEDULER_LOG(ERROR) << "CommitBlock error, " << message;

        commitLock->unlock();
        callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidBlocks, message), nullptr);
        return;
    }

    auto& frontBlock = m_blocks.front();
    if (!frontBlock->result())
    {
        auto message = "Block is executing";
        SCHEDULER_LOG(ERROR) << "CommitBlock error, " << message;

        commitLock->unlock();
        callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidStatus, message), nullptr);
        return;
    }

    if (header->number() != frontBlock->number())
    {
        auto message = "Invalid block number, available block number: " +
                       boost::lexical_cast<std::string>(frontBlock->number());
        SCHEDULER_LOG(ERROR) << "CommitBlock error, " << message;

        commitLock->unlock();
        callback(BCOS_ERROR_UNIQUE_PTR(SchedulerError::InvalidBlockNumber, message), nullptr);
        return;
    }
    // Note: only when the signatureList is empty need to reset the header
    // in case of the signatureList of the header is accessing by the sync module while frontBlock
    // is setting newBlockHeader, which will cause the signatureList ilegal
    auto executedHeader = frontBlock->block()->blockHeader();
    auto signature = executedHeader->signatureList();
    auto currentBlockNumber = executedHeader->number();
    if (signature.size() == 0)
    {
        frontBlock->block()->setBlockHeader(std::move(header));
    }
    SCHEDULER_LOG(INFO) << "Start notify block result: " << currentBlockNumber;
    auto notifyStartT = utcTime();
    frontBlock->asyncNotify(
        m_txNotifier, [notifyStartT, startT, this, currentBlockNumber](Error::Ptr) mutable {
            if (m_blockNumberReceiver)
            {
                m_blockNumberReceiver(currentBlockNumber);
            }
            SCHEDULER_LOG(DEBUG) << LOG_DESC("Notify block result success")
                                 << LOG_KV("num", currentBlockNumber)
                                 << LOG_KV("notifyT", (utcTime() - notifyStartT))
                                 << LOG_KV("timecost", (utcTime() - startT));
        });

    frontBlock->asyncCommit([this, startT, callback = std::move(callback),
                                block = frontBlock->block(), commitLock](Error::UniquePtr&& error) {
        if (error)
        {
            SCHEDULER_LOG(ERROR) << "CommitBlock error, " << boost::diagnostic_information(*error);

            commitLock->unlock();
            callback(BCOS_ERROR_WITH_PREV_UNIQUE_PTR(
                         SchedulerError::UnknownError, "CommitBlock error", *error),
                nullptr);
            return;
        }
        auto blockNumber = block->blockHeader()->number();
        auto hash = block->blockHeader()->hash();
        asyncGetLedgerConfig([this, startT, blockNumber, hash, commitLock = std::move(commitLock),
                                 callback = std::move(callback)](
                                 Error::Ptr error, ledger::LedgerConfig::Ptr ledgerConfig) {
            if (error)
            {
                SCHEDULER_LOG(ERROR)
                    << "Get system config error, " << boost::diagnostic_information(*error);

                commitLock->unlock();
                callback(BCOS_ERROR_WITH_PREV_UNIQUE_PTR(
                             SchedulerError::UnknownError, "Get system config error", *error),
                    nullptr);
                return;
            }
            ledgerConfig->setBlockNumber(blockNumber);
            ledgerConfig->setHash(hash);
            auto recordT = utcTime();
            {
                std::unique_lock<std::mutex> blocksLock(m_blocksMutex);
                bcos::protocol::BlockNumber number = m_blocks.front()->number();
                removeAllOldPreparedBlock(number);
                m_blocks.pop_front();
                SCHEDULER_LOG(DEBUG)
                    << LOG_DESC("CommitBlock and notify block result success")
                    << LOG_KV("num", blockNumber) << LOG_KV("eraseCacheT", (utcTime() - recordT))
                    << LOG_KV("timecost", (utcTime() - startT));
            }

            commitLock->unlock();

            // Note: only after the block notify finished can call the callback
            callback(std::move(error), std::move(ledgerConfig));
        });
    });
}

void SchedulerImpl::status(
    std::function<void(Error::Ptr&&, bcos::protocol::Session::ConstPtr&&)> callback)
{
    (void)callback;
}

void SchedulerImpl::call(protocol::Transaction::Ptr tx,
    std::function<void(Error::Ptr&&, protocol::TransactionReceipt::Ptr&&)> callback)
{
    // call but to is empty,
    // it will cause tx message be marked as 'create' falsely when asyncExecute tx
    if (tx->to().empty())
    {
        callback(BCOS_ERROR_PTR(SchedulerError::UnknownError, "Call address is empty"), nullptr);
        return;
    }
    // set attribute before call
    tx->setAttribute(m_isWasm ? bcos::protocol::Transaction::Attribute::LIQUID_SCALE_CODEC :
                                bcos::protocol::Transaction::Attribute::EVM_ABI_CODEC);
    // Create temp block
    auto block = m_blockFactory->createBlock();
    block->appendTransaction(std::move(tx));

    // Create temp executive
    auto blockExecutive = std::make_shared<BlockExecutive>(std::move(block), this,
        m_calledContextID++, m_transactionSubmitResultFactory, true, m_blockFactory, false);

    blockExecutive->asyncCall([callback = std::move(callback)](Error::UniquePtr&& error,
                                  protocol::TransactionReceipt::Ptr&& receipt) {
        if (error)
        {
            SCHEDULER_LOG(ERROR) << "Unknown error, " << boost::diagnostic_information(*error);
            callback(
                BCOS_ERROR_WITH_PREV_PTR(SchedulerError::UnknownError, "Unknown error", *error),
                nullptr);
            return;
        }
        SCHEDULER_LOG(INFO) << "Call success";
        callback(nullptr, std::move(receipt));
    });
}

void SchedulerImpl::registerExecutor(std::string name,
    bcos::executor::ParallelTransactionExecutorInterface::Ptr executor,
    std::function<void(Error::Ptr&&)> callback)
{
    try
    {
        SCHEDULER_LOG(INFO) << "registerExecutor request: " << LOG_KV("name", name);
        m_executorManager->addExecutor(name, executor);
    }
    catch (std::exception& e)
    {
        SCHEDULER_LOG(ERROR) << "registerExecutor error: " << boost::diagnostic_information(e);
        callback(BCOS_ERROR_WITH_PREV_PTR(-1, "addExecutor error", e));
        return;
    }

    SCHEDULER_LOG(INFO) << "registerExecutor success";
    callback(nullptr);
}

void SchedulerImpl::unregisterExecutor(
    const std::string& name, std::function<void(Error::Ptr&&)> callback)
{
    (void)name;
    (void)callback;
}

void SchedulerImpl::reset(std::function<void(Error::Ptr&&)> callback)
{
    (void)callback;
}

void SchedulerImpl::registerBlockNumberReceiver(
    std::function<void(protocol::BlockNumber blockNumber)> callback)
{
    m_blockNumberReceiver = std::move(callback);
}

void SchedulerImpl::getCode(
    std::string_view contract, std::function<void(Error::Ptr, bcos::bytes)> callback)
{
    auto executor = m_executorManager->dispatchExecutor(contract);
    executor->getCode(contract, std::move(callback));
}

void SchedulerImpl::registerTransactionNotifier(std::function<void(bcos::protocol::BlockNumber,
        bcos::protocol::TransactionSubmitResultsPtr, std::function<void(Error::Ptr)>)>
        txNotifier)
{
    m_txNotifier = std::move(txNotifier);
}

BlockExecutive::Ptr SchedulerImpl::getPreparedBlock(
    bcos::protocol::BlockNumber blockNumber, int64_t timestamp)
{
    bcos::ReadGuard readGuard(x_preparedBlockMutex);

    if (m_preparedBlocks.count(blockNumber) != 0 &&
        m_preparedBlocks[blockNumber].count(timestamp) != 0)
    {
        return m_preparedBlocks[blockNumber][timestamp];
        ;
    }
    else
    {
        return nullptr;
    }
}

void SchedulerImpl::setPreparedBlock(
    bcos::protocol::BlockNumber blockNumber, int64_t timestamp, BlockExecutive::Ptr blockExecutive)
{
    bcos::WriteGuard writeGuard(x_preparedBlockMutex);

    m_preparedBlocks[blockNumber][timestamp] = blockExecutive;
}

void SchedulerImpl::removeAllOldPreparedBlock(bcos::protocol::BlockNumber oldBlockNumber)
{
    bcos::WriteGuard writeGuard(x_preparedBlockMutex);

    // erase all preparedBlock <= oldBlockNumber
    for (auto itr = m_preparedBlocks.begin(); itr != m_preparedBlocks.end();)
    {
        if (itr->first <= oldBlockNumber)
        {
            SCHEDULER_LOG(DEBUG) << LOG_BADGE("prepareBlockExecutive")
                                 << LOG_DESC("removeAllOldPreparedBlock")
                                 << LOG_KV("block number", itr->first);
            itr = m_preparedBlocks.erase(itr);
        }
        else
        {
            itr++;
        }
    }
}

void SchedulerImpl::preExecuteBlock(bcos::protocol::Block::Ptr block, bool verify)
{
    auto blockNumber = block->blockHeaderConst()->number();
    int64_t timestamp = block->blockHeaderConst()->timestamp();
    BlockExecutive::Ptr blockExecutive = getPreparedBlock(blockNumber, timestamp);
    if (blockExecutive != nullptr)
    {
        SCHEDULER_LOG(DEBUG) << LOG_BADGE("prepareBlockExecutive")
                             << "Duplicate block to prepare, dropped."
                             << LOG_KV("blockHeader.timestamp", timestamp)
                             << LOG_KV("index", block->blockHeader()->number())
                             << LOG_KV("hash", block->blockHeader()->hash().abridged())
                             << LOG_KV("undeterministic", block->blockHeader()->undeterministic());
        return;
    }
    SCHEDULER_LOG(INFO) << LOG_BADGE("preExecuteBlock")
                        << LOG_KV("index", block->blockHeader()->number())
                        << LOG_KV("hash", block->blockHeader()->hash().abridged())
                        << LOG_KV("undeterministic", block->blockHeader()->undeterministic());
    blockExecutive = std::make_shared<BlockExecutive>(
        std::move(block), this, 0, m_transactionSubmitResultFactory, false, m_blockFactory, verify);
    blockExecutive->prepare();

    setPreparedBlock(blockNumber, timestamp, blockExecutive);
}


template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void SchedulerImpl::asyncGetLedgerConfig(
    std::function<void(Error::Ptr, ledger::LedgerConfig::Ptr ledgerConfig)> callback)
{
    auto ledgerConfig = std::make_shared<ledger::LedgerConfig>();
    auto callbackPtr = std::make_shared<decltype(callback)>(std::move(callback));
    auto summary =
        std::make_shared<std::tuple<size_t, std::atomic_size_t, std::atomic_size_t>>(4, 0, 0);

    auto collecter = [summary = std::move(summary), ledgerConfig = std::move(ledgerConfig),
                         callback = std::move(callbackPtr)](Error::Ptr error,
                         std::variant<std::tuple<bool, consensus::ConsensusNodeListPtr>,
                             std::tuple<int, std::string>, bcos::protocol::BlockNumber,
                             bcos::crypto::HashType>&& result) mutable {
        auto& [total, success, failed] = *summary;

        if (error)
        {
            SCHEDULER_LOG(ERROR) << "Get ledger config with errors: "
                                 << boost::diagnostic_information(*error);
            ++failed;
        }
        else
        {
            std::visit(
                overloaded{
                    [&ledgerConfig](std::tuple<bool, consensus::ConsensusNodeListPtr>& nodeList) {
                        auto& [isSealer, list] = nodeList;

                        if (isSealer)
                        {
                            ledgerConfig->setConsensusNodeList(*list);
                        }
                        else
                        {
                            ledgerConfig->setObserverNodeList(*list);
                        }
                    },
                    [&ledgerConfig](std::tuple<int, std::string> config) {
                        auto& [type, value] = config;
                        switch (type)
                        {
                        case 0:
                            ledgerConfig->setBlockTxCountLimit(
                                boost::lexical_cast<uint64_t>(value));
                            break;
                        case 1:
                            ledgerConfig->setLeaderSwitchPeriod(
                                boost::lexical_cast<uint64_t>(value));
                            break;
                        default:
                            BOOST_THROW_EXCEPTION(BCOS_ERROR(SchedulerError::UnknownError,
                                "Unknown type: " + boost::lexical_cast<std::string>(type)));
                            break;
                        }
                    },
                    [&ledgerConfig](bcos::protocol::BlockNumber number) {
                        ledgerConfig->setBlockNumber(number);
                    },
                    [&ledgerConfig](bcos::crypto::HashType hash) { ledgerConfig->setHash(hash); }},
                result);

            ++success;
        }

        // Collect done
        if (success + failed == total)
        {
            if (failed > 0)
            {
                SCHEDULER_LOG(ERROR) << "Get ledger config with error: " << failed;
                (*callback)(
                    BCOS_ERROR_PTR(SchedulerError::UnknownError, "Get ledger config with error"),
                    nullptr);

                return;
            }

            (*callback)(nullptr, std::move(ledgerConfig));
        }
    };

    m_ledger->asyncGetNodeListByType(ledger::CONSENSUS_SEALER,
        [collecter](Error::Ptr error, consensus::ConsensusNodeListPtr list) mutable {
            collecter(std::move(error), std::tuple{true, std::move(list)});
        });
    m_ledger->asyncGetNodeListByType(ledger::CONSENSUS_OBSERVER,
        [collecter](Error::Ptr error, consensus::ConsensusNodeListPtr list) mutable {
            collecter(std::move(error), std::tuple{false, std::move(list)});
        });
    m_ledger->asyncGetSystemConfigByKey(ledger::SYSTEM_KEY_TX_COUNT_LIMIT,
        [collecter](Error::Ptr error, std::string config, protocol::BlockNumber) mutable {
            collecter(std::move(error), std::tuple{0, std::move(config)});
        });
    m_ledger->asyncGetSystemConfigByKey(ledger::SYSTEM_KEY_CONSENSUS_LEADER_PERIOD,
        [collecter](Error::Ptr error, std::string config, protocol::BlockNumber) mutable {
            collecter(std::move(error), std::tuple{1, std::move(config)});
        });
}