/**
 *  Copyright (C) 2022 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file TablePrecompiled.h
 * @author: kyonGuo
 * @date 2022/4/20
 */

#pragma once

#include "../vm/Precompiled.h"
#include "bcos-executor/src/precompiled/common/Common.h"
#include "bcos-executor/src/precompiled/common/Condition.h"
#include <bcos-crypto/interfaces/crypto/CommonType.h>
#include <bcos-framework/storage/Table.h>

namespace bcos::precompiled
{
class TablePrecompiled : public bcos::precompiled::Precompiled
{
public:
    using Ptr = std::shared_ptr<TablePrecompiled>;
    TablePrecompiled(crypto::Hash::Ptr _hashImpl);
    ~TablePrecompiled() override = default;

    std::shared_ptr<PrecompiledExecResult> call(
        std::shared_ptr<executor::TransactionExecutive> _executive,
        PrecompiledExecResult::Ptr _callParameters) override;

private:
    void selectByKey(const std::string& tableName,
        const std::shared_ptr<executor::TransactionExecutive>& _executive, bytesConstRef& data,
        const PrecompiledGas::Ptr& gasPricer, PrecompiledExecResult::Ptr const& _callParameters);
    void selectByCondition(const std::string& tableName,
        const std::shared_ptr<executor::TransactionExecutive>& _executive, bytesConstRef& data,
        const PrecompiledGas::Ptr& gasPricer, PrecompiledExecResult::Ptr const& _callParameters);
    void count(const std::string& tableName,
        const std::shared_ptr<executor::TransactionExecutive>& _executive, bytesConstRef& data,
        const PrecompiledGas::Ptr& gasPricer, PrecompiledExecResult::Ptr const& _callParameters);
    void insert(const std::string& tableName,
        const std::shared_ptr<executor::TransactionExecutive>& _executive, bytesConstRef& data,
        const PrecompiledGas::Ptr& gasPricer, PrecompiledExecResult::Ptr const& _callParameters);
    void updateByKey(const std::string& tableName,
        const std::shared_ptr<executor::TransactionExecutive>& _executive, bytesConstRef& data,
        const PrecompiledGas::Ptr& gasPricer, PrecompiledExecResult::Ptr const& _callParameters);
    void updateByCondition(const std::string& tableName,
        const std::shared_ptr<executor::TransactionExecutive>& _executive, bytesConstRef& data,
        const PrecompiledGas::Ptr& gasPricer, PrecompiledExecResult::Ptr const& _callParameters);
    void removeByKey(const std::string& tableName,
        const std::shared_ptr<executor::TransactionExecutive>& _executive, bytesConstRef& data,
        const PrecompiledGas::Ptr& gasPricer, PrecompiledExecResult::Ptr const& _callParameters);
    void removeByCondition(const std::string& tableName,
        const std::shared_ptr<executor::TransactionExecutive>& _executive, bytesConstRef& data,
        const PrecompiledGas::Ptr& gasPricer, PrecompiledExecResult::Ptr const& _callParameters);
    void buildKeyCondition(std::optional<storage::Condition>& keyCondition,
        const std::vector<precompiled::ConditionTuple>& conditions,
        const precompiled::LimitTuple& limit) const;
    bool buildConditions(std::optional<storage::Condition>& keyCondition,
        std::optional<precompiled::Condition>& valueCondition,
        const precompiled::Conditions& conditions,
        const precompiled::LimitTuple& limit, uint32_t version,
        size_t cloumSize, bool isNumericalOrder) const;
    void desc(precompiled::TableInfo& _tableInfo, const std::string& tableName,
        const std::shared_ptr<executor::TransactionExecutive>& _executive,
        PrecompiledExecResult::Ptr const& _callParameters) const;
    bool isNumericalOrder(std::shared_ptr<executor::TransactionExecutive> _executive,
        PrecompiledExecResult::Ptr _callParameters, const std::string& _tableName);
    static bool isNumericalOrder(const TableInfoTupleV320& tableInfo);
};
}  // namespace bcos::precompiled