#ifndef __BUFFER_H
#define __BUFFER_H

#include <list>
#include <string_view>
#include <vector>
#include <string>
#include <functional>
#include <iostream>

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

    void traverseBuffers(std::function<void(void* data, ssize_t size)> callback) {
        for (auto iter = buffers_.begin(); iter != buffers_.end(); ++iter) {
            callback(iter->data(), iter->size());
        }
    }

    void show() {
        int i = 0; 
        traverseBuffers([&i](void* buffer, ssize_t buf_size){
            std::cout << "buffer-" << i;
            std::cout << ", buffer_size:" << buf_size << ", buffer:\n";
            for (ssize_t j = 0; j < buf_size; ++j) {
                std::cout << int(*(reinterpret_cast<uint8_t*>(buffer) + j)) << " ";
            }
            ++i;
            std::cout << std::endl;
        });
    }

    std::vector<uint8_t> flat() {
        std::vector<uint8_t> ret;
        ret.resize(size_);
        uint8_t* dest = ret.data();
        traverseBuffers([&dest](void* buffer, ssize_t buf_size){
            std::memcpy(dest, buffer, buf_size);
            dest += buf_size;
        });
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

    ssize_t read_string(std::string& str, ssize_t offset) {
        if (str.size() == 0 || offset >= size_) {
            return 0;
        }
        return read(str.data(), str.size(), offset);
    }
 
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

// 在发送网络消息的过程中，需要对数据包层层封装，一般是上层的消息包会先序列化，然后作为下层包的payload，在这个过程中payload会被不断拷贝，
// WriteBuffer是针对这种情况作的优化，在使用时直接调用append方法把消息写到WriteBuffer中，最后再flat即可(也可以分段发送)

using WriteBuffer = Buffer<BackwardBlock>;
using ReadBuffer = Buffer<ForwardBlock>;

#endif