#pragma once
#include "memory_arena.hpp"
#include <vector>
#include <shared_mutex>
#include <string>
#include <cuda_runtime.h>
#include <stdexcept>

namespace axiomgraph {

template <typename T>
struct CudaPinnedAllocator {
    using value_type = T;
    CudaPinnedAllocator() noexcept = default;
    template <typename U> CudaPinnedAllocator(const CudaPinnedAllocator<U>&) noexcept {}
    
    T* allocate(std::size_t n) {
        T* ptr = nullptr;
        // Allocate page-locked memory accessible to the device (zero-copy)
        if (cudaHostAlloc((void**)&ptr, n * sizeof(T), cudaHostAllocMapped) != cudaSuccess) {
            throw std::bad_alloc();
        }
        return ptr;
    }
    
    void deallocate(T* p, std::size_t n) noexcept {
        cudaFreeHost(p);
    }
};

class VectorBuffer {
public:
    explicit VectorBuffer(size_t dimensions);
    ~VectorBuffer() = default;

    void add_vector(NodeID id, const float* data);
    size_t size() const;
    size_t dimensions() const;
    const float* data() const;
    
    void save(const std::string& path) const;
    void load(const std::string& path);

private:
    size_t dimensions_;
    std::vector<float, CudaPinnedAllocator<float>> buffer_;
    mutable std::shared_mutex rw_mutex_;
};

} // namespace axiomgraph
