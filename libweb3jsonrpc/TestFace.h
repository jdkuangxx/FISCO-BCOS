/**
 * This file is generated by jsonrpcstub, DO NOT CHANGE IT MANUALLY!
 */

#ifndef JSONRPC_CPP_STUB_DEV_RPC_TESTFACE_H_
#define JSONRPC_CPP_STUB_DEV_RPC_TESTFACE_H_

#include <libweb3jsonrpc/common.h> 

#include "ModularServer.h"

namespace dev {
    namespace rpc {
        class TestFace : public ServerInterface<TestFace>
        {
            public:
                TestFace()
                {
                    this->bindAndAddMethod(jsonrpc::Procedure("test_setChainParams", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_OBJECT, NULL), &dev::rpc::TestFace::test_setChainParamsI);
                    this->bindAndAddMethod(jsonrpc::Procedure("test_mineBlocks", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_INTEGER, NULL), &dev::rpc::TestFace::test_mineBlocksI);
                    this->bindAndAddMethod(jsonrpc::Procedure("test_modifyTimestamp", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_INTEGER, NULL), &dev::rpc::TestFace::test_modifyTimestampI);
                    this->bindAndAddMethod(jsonrpc::Procedure("test_addBlock", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_STRING, NULL), &dev::rpc::TestFace::test_addBlockI);
                    this->bindAndAddMethod(jsonrpc::Procedure("test_rewindToBlock", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_INTEGER, NULL), &dev::rpc::TestFace::test_rewindToBlockI);
                }

                inline virtual void test_setChainParamsI(const Json::Value &request, Json::Value &response)
                {
                    response = this->test_setChainParams(request[0u]);
                }
                inline virtual void test_mineBlocksI(const Json::Value &request, Json::Value &response)
                {
                    int param1;
                    bool valid = dev::rpc::checkParamInt(param1, request[0u], response);
                    if(valid)
                        response = this->test_mineBlocks(param1);
                }
                inline virtual void test_modifyTimestampI(const Json::Value &request, Json::Value &response)
                {
                    int param1;
                    bool valid = dev::rpc::checkParamInt(param1, request[0u], response);
                    if(valid)
                        response = this->test_modifyTimestamp(param1);
                }
                inline virtual void test_addBlockI(const Json::Value &request, Json::Value &response)
                {
                    response = this->test_addBlock(request[0u].asString());
                }
                inline virtual void test_rewindToBlockI(const Json::Value &request, Json::Value &response)
                {
                    int param1;
                    bool valid = dev::rpc::checkParamInt(param1, request[0u], response);
                    if(valid)
                        response = this->test_rewindToBlock(param1);
                }
                virtual bool test_setChainParams(const Json::Value& param1) = 0;
                virtual bool test_mineBlocks(int param1) = 0;
                virtual bool test_modifyTimestamp(int param1) = 0;
                virtual bool test_addBlock(const std::string& param1) = 0;
                virtual bool test_rewindToBlock(int param1) = 0;
        };

    }
}
#endif //JSONRPC_CPP_STUB_DEV_RPC_TESTFACE_H_
