#pragma once

#include <bcos-framework/storage/Common.h>
#include <bcos-framework/protocol/Exceptions.h>
#include <map>
#include <memory>

namespace bcos::precompiled
{
class Condition
{
public:

    Condition() = default;
    ~Condition() = default;
    
    #define PUSH_COND(OP)                                                                     \
        if (mConditions.count(idx) <= 0)                                                      \
        {                                                                                     \
            mConditions.insert(std::make_pair(idx, std::make_shared<storage::Condition>()));  \
        }                                                                                     \
        mConditions[idx]->OP(value)

    void EQ(uint32_t idx, const std::string& value) { PUSH_COND(EQ); }
    void NE(uint32_t idx, const std::string& value) { PUSH_COND(NE); }
    void GT(uint32_t idx, const std::string& value) { PUSH_COND(GT); }
    void GE(uint32_t idx, const std::string& value) { PUSH_COND(GE); }
    void LT(uint32_t idx, const std::string& value) { PUSH_COND(LT); }
    void LE(uint32_t idx, const std::string& value) { PUSH_COND(LE); }
    
    #undef PUSH_COND

    void limit(size_t start, size_t count) { mLimit = std::pair<size_t, size_t>(start, count); }
    std::pair<size_t, size_t> getLimit() { return mLimit; }

    bool isValid(const std::vector<std::string>& values) const
    {
        if(mConditions.empty())
            return true;

        for(auto iter = mConditions.begin(); iter != mConditions.end(); ++iter)
        {
            if(iter->first > values.size())
            {
                PRECOMPILED_LOG(INFO) << LOG_DESC("The field index is greater than the size of fields");                                                       
                BOOST_THROW_EXCEPTION(bcos::protocol::PrecompiledError("The field index is greater than the size of fields")); 
            }
            if(!iter->second->isValid(values[iter->first - 1]))
                return false;
        }
        return true;
    }
private:
    std::map<uint32_t, std::shared_ptr<storage::Condition>> mConditions;
    std::pair<size_t, size_t> mLimit;
};
}  // namespace bcos::precompiled