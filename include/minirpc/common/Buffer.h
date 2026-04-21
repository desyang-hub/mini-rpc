#pragma once

#include <iostream>
#include <cstdint>   // 添加这行
#include <cstddef>   // 可选，提供 size_t
#include <vector>
#include <algorithm>
#include <cstring>
#include <cassert>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

class RingBuffer {
private:
    std::vector<char> buffer_;
    size_t read_pos_;     // 读位置
    size_t write_pos_;    // 写位置
    size_t size_;         // 当前数据大小
    size_t capacity_;     // 缓冲区容量
    
    static constexpr size_t kInitialSize = 1024;

public:
    RingBuffer() : buffer_(kInitialSize), read_pos_(0), write_pos_(0), size_(0), capacity_(kInitialSize) {}
    
    explicit RingBuffer(size_t initial_size) 
        : buffer_(initial_size), read_pos_(0), write_pos_(0), size_(0), capacity_(initial_size) {}

    size_t readable_bytes() const {
        return size_;
    }
    
    size_t writable_bytes() const {
        return capacity_ - size_;
    }
    
    // 获取指定长度的数据，用于读取header等
    bool peek_data(void* dest, size_t len) const {
        if (len > size_) return false;
        
        if (read_pos_ + len <= capacity_) {
            // 不跨越边界
            memcpy(dest, &buffer_[read_pos_], len);
        } else {
            // 跨越边界，分两部分复制
            size_t first_part = capacity_ - read_pos_;
            memcpy(dest, &buffer_[read_pos_], first_part);
            memcpy(static_cast<char*>(dest) + first_part, &buffer_[0], len - first_part);
        }
        return true;
    }
    
    // 检查是否有足够的数据用于解析
    bool has_enough_data(size_t len) const {
        return size_ >= len;
    }
    
    // 获取当前可读数据的第一个连续块（用于零拷贝场景）
    const char* peek_contiguous_data() const {
        return size_ == 0 ? nullptr : &buffer_[read_pos_];
    }
    
    // 获取第一个连续块的大小
    size_t contiguous_readable_bytes() const {
        if (size_ == 0) return 0;
        
        if (read_pos_ + size_ <= capacity_) {
            // 整个可读区域都是连续的
            return size_;
        } else {
            // 返回从read_pos到缓冲区末尾的部分
            return capacity_ - read_pos_;
        }
    }
    
    // 获取完整包数据（如果需要拷贝的话）
    bool get_package_data(void* dest, size_t len) {
        if (!has_enough_data(len)) return false;
        
        if (read_pos_ + len <= capacity_) {
            // 不跨越边界，零拷贝
            memcpy(dest, &buffer_[read_pos_], len);
        } else {
            // 跨越边界，分段拷贝
            size_t first_part = capacity_ - read_pos_;
            memcpy(dest, &buffer_[read_pos_], first_part);
            memcpy(static_cast<char*>(dest) + first_part, &buffer_[0], len - first_part);
        }
        
        retrieve(len);
        return true;
    }
    
    // 仅检索（消费）数据而不复制
    void retrieve(size_t len) {
        if (len >= size_) {
            clear();
            return;
        }
        
        read_pos_ = (read_pos_ + len) % capacity_;
        size_ -= len;
    }
    
    void retrieve_all() {
        clear();
    }
    
    void clear() {
        read_pos_ = 0;
        write_pos_ = 0;
        size_ = 0;
    }
    
    void append(const char* data, size_t len) {
        if (len > writable_bytes()) {
            expand_if_needed(len);
        }
        
        if (len == 0) return;
        
        if (write_pos_ + len <= capacity_) {
            // 不跨越边界
            memcpy(&buffer_[write_pos_], data, len);
            write_pos_ = (write_pos_ + len) % capacity_;
        } else {
            // 跨越边界，分两部分写入
            size_t first_part = capacity_ - write_pos_;
            memcpy(&buffer_[write_pos_], data, first_part);
            memcpy(&buffer_[0], data + first_part, len - first_part);
            write_pos_ = (len - first_part) % capacity_;
        }
        
        size_ += len;
    }
    
    void append(const std::string& str) {
        append(str.data(), str.size());
    }
    
    // 从文件描述符读取数据
    ssize_t read_fd(int fd, int* saved_errno) {
        if (writable_bytes() == 0) {
            expand_if_needed(1);  // 确保有空间
        }
        
        size_t writable = writable_bytes();
        ssize_t n = 0;
        
        if (write_pos_ + writable <= capacity_) {
            // 不跨越边界，直接读取
            n = ::read(fd, &buffer_[write_pos_], writable);
        } else {
            // 跨越边界，使用 readv
            struct iovec iov[2];
            iov[0].iov_base = &buffer_[write_pos_];
            iov[0].iov_len = capacity_ - write_pos_;
            iov[1].iov_base = &buffer_[0];
            iov[1].iov_len = std::min(writable - (capacity_ - write_pos_), 
                                     static_cast<size_t>(capacity_));
            
            int iovcnt = (iov[1].iov_len > 0) ? 2 : 1;
            n = ::readv(fd, iov, iovcnt);
        }
        
        if (n > 0) {
            write_pos_ = (write_pos_ + n) % capacity_;
            size_ += n;
        } else if (n < 0) {
            *saved_errno = errno;
        }
        
        return n;
    }
    
    ssize_t write_fd(int fd, int* saved_errno) {
        if (size_ == 0) return 0;
        
        ssize_t n = 0;
        
        if (read_pos_ + size_ <= capacity_) {
            // 不跨越边界，直接写
            n = ::write(fd, &buffer_[read_pos_], size_);
        } else {
            // 跨越边界，使用 writev
            struct iovec iov[2];
            iov[0].iov_base = &buffer_[read_pos_];
            iov[0].iov_len = capacity_ - read_pos_;
            iov[1].iov_base = &buffer_[0];
            iov[1].iov_len = size_ - (capacity_ - read_pos_);
            
            n = ::writev(fd, iov, 2);
        }
        
        if (n > 0) {
            retrieve(n);
        } else if (n < 0) {
            *saved_errno = errno;
        }
        
        return n;
    }

private:
    void expand_if_needed(size_t additional_size) {
        size_t needed_capacity = size_ + additional_size;
        if (needed_capacity <= capacity_) return;
        
        // 计算新的容量
        size_t new_capacity = capacity_;
        while (new_capacity < needed_capacity) {
            new_capacity *= 2;
        }
        
        std::vector<char> new_buffer(new_capacity);
        size_t readable = readable_bytes();
        
        if (readable > 0) {
            if (read_pos_ + readable <= capacity_) {
                // 原数据不跨越边界
                std::copy(&buffer_[read_pos_], &buffer_[read_pos_ + readable], 
                         new_buffer.begin());
            } else {
                // 原数据跨越边界
                size_t first_part = capacity_ - read_pos_;
                std::copy(&buffer_[read_pos_], &buffer_[capacity_], 
                         new_buffer.begin());
                std::copy(&buffer_[0], &buffer_[write_pos_], 
                         new_buffer.begin() + first_part);
            }
        }
        
        buffer_ = std::move(new_buffer);
        read_pos_ = 0;
        write_pos_ = readable;
        capacity_ = new_capacity;
    }
};