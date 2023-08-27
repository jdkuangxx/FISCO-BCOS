#ifndef __BUFFER_H
#define __BUFFER_H

#include <list>
#include <string_view>
#include <vector>
#include <string>
#include <functional>
#include <iostream>
#include <assert.h>

#include "Block.h"

template <typename T>
concept Integral = std::is_integral<T>::value;

template<typename BlockType, bool forward = BlockType::is_forward_block>
class Buffer {
public:
    using Ptr = std::shared_ptr<Buffer>;    

    static const ssize_t DEFAULT_MIN_BLOCK_SIZE = 256;

    Buffer() : size_(0) {}
    
    virtual ~Buffer() = default;

    DISALLOW_COPY_AND_ASSIGN(Buffer);

    void push(BlockHandle handle) {
        if constexpr (forward) {
            buffers_.push_back(handle);
        } else {
            buffers_.push_front(handle);
        }
    }

    const void* nextSrcPos(const void* src, ssize_t size) {
        if constexpr (forward) {
            return reinterpret_cast<const uint8_t*>(src) + size;
        } else {
            return src;
        }
    }

    void rTraverseBuffers(std::function<void(const void* data, ssize_t size)>&& callback) const {
        for (auto iter = buffers_.rbegin(); iter != buffers_.rend(); ++iter) {
            callback(iter->data(), iter->size());
        }    
    }

    void sTraverseBuffers(std::function<void(const void* data, ssize_t size)>&& callback) const {
        for (auto iter = buffers_.begin(); iter != buffers_.end(); ++iter) {
            callback(iter->data(), iter->size());
        }
    }

    void show() const {
        int i = 0; 
        sTraverseBuffers([&i](const void* buffer, ssize_t buf_size) {
            std::cout << "buffer-" << i;
            std::cout << ", buffer_size:" << buf_size << ", buffer:\n";
            for (ssize_t j = 0; j < buf_size; ++j) {
                std::cout << int(*(reinterpret_cast<const uint8_t*>(buffer) + j)) << " ";
            }
            ++i;
            std::cout << std::endl;
        });
    }

    std::vector<uint8_t> flat(ssize_t offset = 0) const {
        std::vector<uint8_t> ret;
        if (offset >= size_) {
            return ret;
        }
        ret.resize(size_ - offset);
        read(ret.data(), ret.size(), offset);
        return ret;
    }

    void write(const void* data, ssize_t size) {
        if (buffers_.empty() || cur_block_ == nullptr || cur_block_->full()) {
            cur_block_ = std::make_shared<BlockType>(std::max(min_block_size_, size));
        }

        while (size > 0) {
            BlockHandle handle = cur_block_->write(data, size);
            ssize_t w_size = handle.size();

            if (w_size > 0) {
                push(handle);
            }

            size -= w_size;
            size_ += w_size;
            
            if (size > 0 && cur_block_->full()) {
                cur_block_ = std::make_shared<BlockType>(std::max(min_block_size_, size));
            }
            data = nextSrcPos(data, w_size);
        }
    }

    ssize_t read(void* output_buffer, ssize_t output_size, ssize_t offset) const {
        uint8_t* _output_buffer = reinterpret_cast<uint8_t*>(output_buffer);
        for (auto iter = buffers_.begin(); iter != buffers_.end() && output_size > 0; ++iter) {

            ssize_t temp = offset - iter->size();
            
            if (temp >= 0) {
                offset = temp;
                continue;
            }
    
            ssize_t read_size = std::min(output_size, iter->size() - offset);
            std::memcpy(_output_buffer, reinterpret_cast<const uint8_t*>(iter->data()) + offset, read_size);
            output_size -= read_size;
            _output_buffer += read_size;
            offset = 0;
        }
        return reinterpret_cast<intptr_t>(_output_buffer) - reinterpret_cast<intptr_t>(output_buffer);
    }

    template<Integral Int>
    bool read_int(Int& ret, ssize_t offset) const {
        if (size_ - offset < sizeof(Int)) {
            return false;
        }
        read(&ret, sizeof(Int), offset);
        return true;
    }

    ssize_t read_string(std::string& str, ssize_t offset) const {
        if (str.size() == 0 || offset >= size_) {
            return 0;
        }
        return read(str.data(), str.size(), offset);
    }
    
    template<typename BufferType>
    void write_buffer(const BufferType& buffer);

    template<Integral Int>
    void write_int(const Int number) {
        write(&number, sizeof(Int));
    }

    void write_string(std::string_view str) {
        write(str.data(), str.size());
    }

    void set_min_block_size(ssize_t min_block_size) { 
        min_block_size_ = min_block_size;
    }

    size_t size() const {
        return size_;
    }

protected:
    std::list<BlockHandle> buffers_;
    BlockType::Ptr cur_block_;
    ssize_t size_;
    ssize_t min_block_size_ = DEFAULT_MIN_BLOCK_SIZE;
};

template<typename BufferType>
class BufferWapper {
public:
    BufferWapper(const BufferType& buffer, ssize_t offset) 
    : buffer_(buffer), offset_(offset) { }

    template<Integral Int>
    BufferWapper& read_int(Int& ret) {
        if (buffer_.read_int(ret, offset_)) {
            offset_ += sizeof(Int);
        }
        return *this;
    }

    BufferWapper& read_string(std::string& str, ssize_t size) {
        str.resize(size);
        ssize_t read_size = buffer_.read_string(str, offset_);
        str.resize(read_size);
        offset_ += read_size;
        return *this;
    }

private:
    const BufferType& buffer_;
    ssize_t offset_;
};

// 针对发送过程中，消息总是层层封装，上层消息作为底层消息的payload，为了减少逐层封装消息时的内存拷贝，特意提供一种可以从后往前写的buffer
using WriteBuffer = Buffer<BackwardBlock>;
// 针对接受过程中，先收Head，再收payload的，抽象出一种从前往后写的buffer
using ReadBuffer = Buffer<ForwardBlock>;

using WriteBufferWapper = BufferWapper<WriteBuffer>; 
using ReadBufferWapper = BufferWapper<ReadBuffer>; 

template<>
template<>
void WriteBuffer::write_buffer<WriteBuffer>(const WriteBuffer& buffer) {
    if (this == &buffer) {
        return;
    }

    for (auto iter = buffer.buffers_.rbegin(); iter != buffer.buffers_.rend(); ++iter) {
        // 执行BlockHandle的拷贝构造
        push(*iter);
        size_ += iter->size();
    }
}

template<>
template<>
void WriteBuffer::write_buffer<ReadBuffer>(const ReadBuffer& buffer) {
    buffer.rTraverseBuffers(
        [this](const void* data, ssize_t size){
            if (size > 0) {
                auto block = std::make_shared<ForwardBlock>(size);
                BlockHandle handle = block->write(data, size);
                assert(handle.size() == size);
                push(handle);
                size_ += size;
            }

        });
}

template<>
template<>
void ReadBuffer::write_buffer<ReadBuffer>(const ReadBuffer& buffer) {
    if (this == &buffer) {
        return;
    }

    for (auto iter = buffer.buffers_.begin(); iter != buffer.buffers_.end(); ++iter) {
        // 执行BlockHandle的拷贝构造
        push(*iter);
        size_ += iter->size();
    }
}

template<>
template<>
void ReadBuffer::write_buffer<WriteBuffer>(const WriteBuffer& buffer) {
    buffer.sTraverseBuffers(
        [this](const void* data, ssize_t size){
            if (size > 0) {
                auto block = std::make_shared<ForwardBlock>(size);
                BlockHandle handle = block->write(data, size);
                assert(handle.size() == size);
                push(handle);
                size_ += size;
            }

        });
}

template<typename BufferType, Integral Int>
BufferType& operator<<(BufferType& buffer, Int num) {
    buffer.write_int(num);
    return buffer;
}

template<typename BufferType>
BufferType& operator<<(BufferType& buffer, std::string_view str) {
    buffer.write_string(str);
    return buffer;
}

#endif