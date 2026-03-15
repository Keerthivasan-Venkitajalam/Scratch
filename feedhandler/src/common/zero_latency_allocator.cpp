#include "common/zero_latency_allocator.hpp"
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>

namespace feedhandler {
namespace common {

ZeroLatencyAllocator::ZeroLatencyAllocator(size_t total_size) 
    : memory_base_(nullptr), total_size_(total_size), current_offset_(0), allocation_count_(0) {
    
    // Align total size to page boundary
    size_t page_size = getpagesize();
    total_size_ = (total_size + page_size - 1) & ~(page_size - 1);
    
    // Try to setup huge pages first, fallback to regular pages
    if (!setup_huge_pages()) {
        // Fallback to regular mmap
        memory_base_ = mmap(nullptr, total_size_, 
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (memory_base_ == MAP_FAILED) {
            throw std::bad_alloc();
        }
    }
    
    // Prefault all pages to avoid page faults during allocation
    prefault_memory();
}

ZeroLatencyAllocator::~ZeroLatencyAllocator() {
    if (memory_base_ && memory_base_ != MAP_FAILED) {
        munmap(memory_base_, total_size_);
    }
}

bool ZeroLatencyAllocator::setup_huge_pages() {
#ifdef __linux__
    // Try 2MB huge pages first
    memory_base_ = mmap(nullptr, total_size_,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    
    if (memory_base_ != MAP_FAILED) {
        return true;
    }
    
    // Try transparent huge pages
    memory_base_ = mmap(nullptr, total_size_,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (memory_base_ != MAP_FAILED) {
        // Advise kernel to use huge pages
        madvise(memory_base_, total_size_, MADV_HUGEPAGE);
        return true;
    }
#else
    // On macOS, just use regular mmap
    memory_base_ = mmap(nullptr, total_size_,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (memory_base_ != MAP_FAILED) {
        return true;
    }
#endif
    
    return false;
}

void ZeroLatencyAllocator::prefault_memory() {
    if (!memory_base_) return;
    
    // Touch every page to prefault
    size_t page_size = getpagesize();
    volatile char* ptr = static_cast<volatile char*>(memory_base_);
    
    for (size_t offset = 0; offset < total_size_; offset += page_size) {
        ptr[offset] = 0; // Touch page to fault it in
    }
}

void* ZeroLatencyAllocator::allocate(size_t size, size_t alignment) noexcept {
    if (size == 0) return nullptr;
    
    // Align size
    size_t aligned_size = align_size(size, alignment);
    
    // Atomic bump allocation
    size_t old_offset = current_offset_.load(std::memory_order_relaxed);
    size_t new_offset;
    
    do {
        // Align offset to requested alignment
        size_t aligned_offset = (old_offset + alignment - 1) & ~(alignment - 1);
        new_offset = aligned_offset + aligned_size;
        
        // Check if we have enough space
        if (new_offset > total_size_) {
            return nullptr; // Out of memory
        }
        
    } while (!current_offset_.compare_exchange_weak(
        old_offset, new_offset, 
        std::memory_order_relaxed, std::memory_order_relaxed));
    
    // Calculate aligned pointer
    size_t aligned_offset = (old_offset + alignment - 1) & ~(alignment - 1);
    void* ptr = static_cast<char*>(memory_base_) + aligned_offset;
    
    allocation_count_.fetch_add(1, std::memory_order_relaxed);
    
    return ptr;
}

void ZeroLatencyAllocator::reset() noexcept {
    current_offset_.store(0, std::memory_order_relaxed);
    allocation_count_.store(0, std::memory_order_relaxed);
}

ZeroLatencyAllocator::Stats ZeroLatencyAllocator::get_stats() const noexcept {
    size_t allocated = current_offset_.load(std::memory_order_relaxed);
    size_t count = allocation_count_.load(std::memory_order_relaxed);
    
    return Stats{
        .total_size = total_size_,
        .allocated_size = allocated,
        .remaining_size = total_size_ - allocated,
        .allocation_count = count,
        .utilization_percent = static_cast<double>(allocated) / total_size_ * 100.0
    };
}

bool ZeroLatencyAllocator::owns(void* ptr) const noexcept {
    if (!ptr || !memory_base_) return false;
    
    char* char_ptr = static_cast<char*>(ptr);
    char* base = static_cast<char*>(memory_base_);
    
    return char_ptr >= base && char_ptr < base + total_size_;
}

size_t ZeroLatencyAllocator::align_size(size_t size, size_t alignment) const noexcept {
    return (size + alignment - 1) & ~(alignment - 1);
}

} // namespace common
} // namespace feedhandler