#pragma once

#include <cassert>
#include <cstdint>

namespace tdc {
namespace runtime {

class DataType {
   public:
    enum TypeCode { kInt = 0, kUint, kFloat, kBFloat, kHandle };

    DataType() {}
    DataType(TypeCode code, int bits, int lanes, bool is_scalable = false)
        : code_(code), bits_(bits) {
        if (is_scalable) {
            assert(lanes > 1);
        }
        lanes_ = is_scalable ? static_cast<uint16_t>(-lanes)
                             : static_cast<uint16_t>(lanes);
    }
    DataType(const DataType& other)
        : code_(other.code_), bits_(other.bits_), lanes_(other.lanes_) {}
    DataType& operator=(const DataType& other) {
        code_ = other.code_;
        bits_ = other.bits_;
        lanes_ = other.lanes_;
        return *this;
    }
    bool operator==(const DataType& other) const {
        return code_ == other.code_ && bits_ == other.bits_ &&
               lanes_ == other.lanes_;
    }
    bool operator!=(const DataType& other) const { return !operator==(other); }

    int code() const { return code_; }
    int bits() const { return static_cast<int>(bits_); }
    int bytes() const { return (bits() + 7) / 8; }
    int lanes() const {
        int lanes_as_int = static_cast<int16_t>(lanes_);
        assert(lanes_as_int > 0);
        return lanes_as_int;
    }

    // TODO: 添加isBool, isInt等等的检测函数
    bool is_scalable_vector() const {
        return static_cast<int16_t>(lanes_) < -1;
    }
    bool is_fixed_length_vector() const {
        return static_cast<int16_t>(lanes_) > 1;
    }
    bool is_vector() const { return lanes() > 1; }

    // 构造默认的数据类型
    static DataType Int(int bits, int lanes = 1) {
        return DataType(kInt, bits, lanes);
    }
    static DataType Uint(int bits, int lanes = 1) {
        return DataType(kUint, bits, lanes);
    }
    static DataType Float(int bits, int lanes = 1) {
        return DataType(kFloat, bits, lanes);
    }
    static DataType BFloat(int lanes = 1) {
        return DataType(kBFloat, 16, lanes);
    }
    static DataType Boolean(int lanes = 1, bool is_scalable = false) {
        return DataType(kUint, lanes, is_scalable);
    }
    static DataType Handle(int bits = 64, int lanes = 1) {
        return DataType(kHandle, bits, lanes);
    }
    static DataType Void() { return DataType(kHandle, 0, 0); }
    static DataType ShapeIndex() { return Uint(64); }

   private:
    TypeCode code_;
    uint8_t bits_;
    // lanes_ < 0 表示为变长的向量, lanes > 0表示为定长向量
    uint16_t lanes_;
};

}  // namespace runtime
}  // namespace tdc
