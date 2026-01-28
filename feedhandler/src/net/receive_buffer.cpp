#include "net/receive_buffer.hpp"
#include <algorithm>

namespace feedhandler {
namespace net {

ReceiveBuffer::ReceiveBuffer() : write_pos_(0), read_pos_(0) {
    memset(buffer_, 0, BUFFER_SIZE);
}

size_t ReceiveBuffer::write(const char* data, size_t len) {
    size_t available = BUFFER_SIZE - write_pos_;
    size_t to_write = std::min(len, available);
    
    if (to_write > 0) {
        memcpy(buffer_ + write_pos_, data, to_write);
        write_pos_ += to_write;
    }
    
    return to_write;
}

size_t ReceiveBuffer::readable_bytes() const {
    return write_pos_ - read_pos_;
}

void ReceiveBuffer::consume(size_t len) {
    read_pos_ += std::min(len, readable_bytes());
    
    // Compact buffer when read_pos_ is far ahead
    if (read_pos_ > BUFFER_SIZE / 2) {
        size_t remaining = write_pos_ - read_pos_;
        if (remaining > 0) {
            memmove(buffer_, buffer_ + read_pos_, remaining);
        }
        write_pos_ = remaining;
        read_pos_ = 0;
    }
}

bool ReceiveBuffer::has_space() const {
    return write_pos_ < BUFFER_SIZE;
}

void ReceiveBuffer::reset() {
    write_pos_ = 0;
    read_pos_ = 0;
    memset(buffer_, 0, BUFFER_SIZE);
}

} // namespace net
} // namespace feedhandler
