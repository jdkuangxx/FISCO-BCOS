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
 * @brief codec for KnowledgeProof data
 * @file KnowledgeProofData.cpp
 * @date 2022.07.18
 * @author yujiechen
 */
#include "KnowledgeProofData.h"
#include "bcos-crypto/zkp/Exceptions.h"

using namespace bcos;
using namespace bcos::crypto;

void KnowledgeProofData::checkKnowledgeProof() const
{
    if (m_t1.size() < m_pointLen || m_m1.size() < m_scalarLen || m_m2.size() < m_scalarLen)
    {
        BOOST_THROW_EXCEPTION(
            InvalidKnowledgeProof() << errinfo_comment(
                "InvalidKnowledgeProof: the scalar data size must be at least " +
                std::to_string(m_scalarLen) + ", the point len size must be at least " +
                std::to_string(m_pointLen)));
    }
}

bytesPointer KnowledgeProofData::encode() const
{
    checkKnowledgeProof();
    auto encodedData = std::make_shared<bytes>();
    // encode t1
    encodedData->insert(encodedData->end(), m_t1.data(), m_t1.data() + m_pointLen);
    // m1
    encodedData->insert(encodedData->end(), m_m1.data(), m_m1.data() + m_scalarLen);
    // m2
    encodedData->insert(encodedData->end(), m_m2.data(), m_m2.data() + m_scalarLen);
    return encodedData;
}

void KnowledgeProofData::decode(bytesConstRef _proofData)
{
    // check the knowledge-proof
    if (_proofData.size() <= m_proofSize)
    {
        BOOST_THROW_EXCEPTION(
            InvalidKnowledgeProof() << errinfo_comment(
                "InvalidKnowledgeProof: the knowledge proof data size must be at least " +
                std::to_string(m_proofSize)));
    }
    // decode t1
    auto pos = _proofData.data();
    m_t1 = bytes(pos, pos + m_pointLen);
    pos += m_pointLen;
    // decode m1
    m_m1 = bytes(pos, pos + m_scalarLen);
    pos += m_scalarLen;
    // decode m2
    m_m2 = bytes(pos, pos + m_scalarLen);
}

CKnowledgeProof KnowledgeProofData::toKnowledgeProof() const
{
    return CKnowledgeProof{
        (char*)m_t1.data(), (char*)m_m1.data(), (char*)m_m2.data(), m_scalarLen, m_pointLen};
}
