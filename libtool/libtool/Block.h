#ifndef __BLOCK_H
#define __BLOCK_H

#include <cstdlib>
#include <cstring>
#include <memory>
#include "Macros.h"

class BlockHandle;
class BlockBase : public std::enable_shared_from_this<BlockBase> {
public:
    using Ptr = std::shared_ptr<BlockBase>;
    BlockBase() 
    : buffer_(nullptr)
    , capacity_(0)
    , offset_(0) 
    {}
    
    BlockBase(ssize_t size)
    : buffer_(std::malloc(size))
    , capacity_(size)
    , offset_(0) 
    {}

    virtual ~BlockBase() {
      if (buffer_ != nullptr) {
        std::free(buffer_);
        buffer_ = nullptr;
      }
      capacity_ = 0;
      offset_ = 0;
    }

    virtual BlockHandle write(const void* data, ssize_t size) = 0;

    ssize_t capacity() const { return capacity_; }

    ssize_t offset() const { return offset_; }

    ssize_t available_capacity() const { return capacity_ - offset_; }

    bool full() const { return capacity_ == offset_; }

    DISALLOW_COPY_AND_ASSIGN(BlockBase);

protected:
    void *buffer_;
    ssize_t capacity_;
    ssize_t offset_;
};

class BlockHandle {
public:
    BlockHandle() : buf_(nullptr), data_(nullptr), size_(0) {}

    BlockHandle(BlockBase::Ptr buf) 
    : buf_(buf)
    , data_(nullptr)
    , size_(0) 
    {}

    BlockHandle(BlockBase::Ptr buf, void* data, ssize_t size) 
    : buf_(buf)
    , data_(data)
    , size_(size) 
    {}

    ~BlockHandle() = default;

    const void* data() const {
        return reinterpret_cast<const uint8_t*>(data_);
    }

    void* data() {
        return reinterpret_cast<uint8_t*>(data_);
    }

    ssize_t size() const {
        return size_;
    }

    void set_buf(BlockBase::Ptr buf) { buf_ = buf; }
    void set_data(void* data) { data_ = data; }
    void set_size(ssize_t size) { size_ = size; }

private:  
    BlockBase::Ptr buf_;
    void* data_;
    ssize_t size_;
};

class BackwardBlock :  public BlockBase {
public:
    using Ptr = std::shared_ptr<BackwardBlock>;
    static const bool is_forward_block = false;
    BackwardBlock() : BlockBase() {}

    BackwardBlock(ssize_t size) : BlockBase(size) {}

    ~BackwardBlock() = default;
    
    // | unused | reserve | used |
    //       end <---- begin
    // capacity_ = 8, offset_ = 2, size = 4
    // 0, 1, 2, 3, 4, 5, 6, 7
    //                   x, x
    //       y, y, y, y          
    BlockHandle write(const void* data, ssize_t size) override {
        ssize_t w_size = std::min(size, capacity_ - offset_);
        if (w_size <= 0) {
            return BlockHandle();
        }
        uint8_t* dest =  locateStart() - w_size;
        const uint8_t* src = reinterpret_cast<const uint8_t*>(data) + size - w_size;
        std::memcpy(dest, src, w_size);
        offset_ += w_size;
        return BlockHandle(shared_from_this(), dest, w_size);
    }

private:
    inline uint8_t* locateStart() const {
        return reinterpret_cast<uint8_t*>(buffer_) + capacity_ - offset_;
    }
};

class ForwardBlock  :  public BlockBase  {
public:
    using Ptr = std::shared_ptr<ForwardBlock>;
    static const bool is_forward_block = true;
    ForwardBlock() : BlockBase() {}

    ForwardBlock(ssize_t size) : BlockBase(size) {}

    ~ForwardBlock() = default;
    
    // | used | reserve | unused |
    //       begin ----> end
    // capacity_ = 8, offset_ = 2, size = 4
    // 0, 1, 2, 3, 4, 5, 6, 7
    // x, x              
    //       y, y, y, y          
    BlockHandle write(const void* data, ssize_t size) override {
        ssize_t w_size = std::min(size, capacity_ - offset_);
        if (w_size <= 0) {
            return BlockHandle();
        }
        uint8_t* dest =  locateStart();
        const uint8_t* src = reinterpret_cast<const uint8_t*>(data);
        std::memcpy(dest, src, w_size);
        offset_ += w_size;
        return BlockHandle(shared_from_this(), dest, w_size);
    }

private:
    inline uint8_t* locateStart() const {
        return reinterpret_cast<uint8_t*>(buffer_) + offset_;
    }
};

#endif