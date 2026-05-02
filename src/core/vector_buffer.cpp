#include "vector_buffer.hpp"
#include <cstring>
#include <algorithm>
#include <fstream>

namespace axiomgraph {

VectorBuffer::VectorBuffer(size_t dimensions) : dimensions_(dimensions) {}

void VectorBuffer::add_vector(NodeID id, const float* data_ptr) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    size_t required_size = (id + 1) * dimensions_;
    if (buffer_.size() < required_size) {
        buffer_.resize(required_size, 0.0f);
    }
    std::memcpy(buffer_.data() + id * dimensions_, data_ptr, dimensions_ * sizeof(float));
}

size_t VectorBuffer::size() const {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    return buffer_.size() / dimensions_;
}

size_t VectorBuffer::dimensions() const {
    return dimensions_;
}

const float* VectorBuffer::data() const {
    return buffer_.data();
}

void VectorBuffer::save(const std::string& path) const {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);
    std::ofstream os(path, std::ios::binary);
    if (!os) throw std::runtime_error("Cannot open file for writing: " + path);
    size_t num_elements = buffer_.size();
    os.write(reinterpret_cast<const char*>(&dimensions_), sizeof(dimensions_));
    os.write(reinterpret_cast<const char*>(&num_elements), sizeof(num_elements));
    os.write(reinterpret_cast<const char*>(buffer_.data()), num_elements * sizeof(float));
}

void VectorBuffer::load(const std::string& path) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    std::ifstream is(path, std::ios::binary);
    if (!is) throw std::runtime_error("Cannot open file for reading: " + path);
    size_t num_elements = 0;
    is.read(reinterpret_cast<char*>(&dimensions_), sizeof(dimensions_));
    is.read(reinterpret_cast<char*>(&num_elements), sizeof(num_elements));
    buffer_.resize(num_elements);
    is.read(reinterpret_cast<char*>(buffer_.data()), num_elements * sizeof(float));
}

} // namespace axiomgraph
