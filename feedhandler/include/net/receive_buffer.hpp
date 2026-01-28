#pragma once

#include <cstddef>
#include <cstring>

namespace feedhandler {
namespace net {

// Circular receive buffer: handles TCP fragmentation gracefully
// Stores incomplete messages and resumes parsing after new data arrives
class ReceiveBuffer {
public:
    static constexpr size_t BUFFER_SIZE = 8192;
    
    ReceiveBuffer();
    
    // Write incoming bytes to buffer (from recv())
    size_t write(const char* data, size_t len);
    
    // Read from buffer without consuming (peek)
    const char* read_ptr() const { return buffer_ + read_pos_; }
    size_t readable_bytes() const;
    
    // Consume N bytes after parsing
    void consume(size_t len);
    
    // Check if buffer has space for more data
    bool has_space() const;
    
    // Reset buffer (empty it)
    void reset();
    
    // Get underlying buffer for direct use
    char* write_buffer() { return buffer_ + write_pos_; }
    size_t available_write() const { return BUFFER_SIZE - write_pos_; }
    void advance_write(size_t len) { write_pos_ += len; }
    
private:
    alignas(64) char buffer_[BUFFER_SIZE];  // 64-byte aligned for cache efficiency
    size_t write_pos_;  // Where next recv() data goes
    size_t read_pos_;   // Where parser reads from
};

} // namespace net
} // namespace feedhandler
