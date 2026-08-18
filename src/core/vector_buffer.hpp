#pragma once
#include "memory_arena.hpp"
#include <vector>
#include <shared_mutex>
#include <string>
#ifdef USE_CUDA
#include <cuda_runtime.h>
#endif
#include <stdexcept>
#include <cstdlib>

namespace axiomgraph {

template <typename T>
struct CudaPinnedAllocator {
    using value_type = T;
    CudaPinnedAllocator() noexcept = default;
    template <typename U> CudaPinnedAllocator(const CudaPinnedAllocator<U>&) noexcept {}
    
    T* allocate(std::size_t n) {
        T* ptr = nullptr;
#ifdef USE_CUDA
        // Allocate page-locked memory accessible to the device (zero-copy)
        if (cudaHostAlloc((void**)&ptr, n * sizeof(T), cudaHostAllocMapped) != cudaSuccess) {
            throw std::bad_alloc();
        }
#else
        ptr = static_cast<T*>(std::malloc(n * sizeof(T)));
        if (!ptr) throw std::bad_alloc();
#endif
        return ptr;
    }
    
    void deallocate(T* p, std::size_t n) noexcept {
#ifdef USE_CUDA
        cudaFreeHost(p);
#else
        std::free(p);
#endif
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
