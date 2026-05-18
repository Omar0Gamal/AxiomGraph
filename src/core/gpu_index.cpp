#include "gpu_index.hpp"
#include <raft/core/handle.hpp>
#include <raft/core/device_mdspan.hpp>
#include <cuvs/neighbors/cagra.hpp>
#include <cuvs/neighbors/vamana.hpp>
#include <rmm/device_uvector.hpp>
#include <stdexcept>

namespace axiomgraph {

struct GPUIndex::Impl {
    raft::handle_t handle;
    std::unique_ptr<cuvs::neighbors::cagra::index<float, uint32_t>> cagra_idx;
    std::unique_ptr<cuvs::neighbors::vamana::index<float, uint32_t>> vamana_idx;
    bool is_cagra = false;
    bool is_vamana = false;
};

GPUIndex::GPUIndex() : impl_(std::make_unique<Impl>()) {}
GPUIndex::~GPUIndex() = default;

void GPUIndex::build_cagra_index(const VectorBuffer& vectors, const CagraConfig& config) {
    cuvs::neighbors::cagra::index_params params;
    params.graph_degree = config.graph_degree;
    params.intermediate_graph_degree = config.intermediate_graph_degree;
    
    size_t num_vectors = vectors.size();
    size_t dims = vectors.dimensions();
    const float* host_data = vectors.data();
    
    // Pass the CUDA mapped pointer directly to raft device matrix view
    auto dataset = raft::make_device_matrix_view<const float, int64_t>(host_data, num_vectors, dims);
    
    impl_->cagra_idx = std::make_unique<cuvs::neighbors::cagra::index<float, uint32_t>>(
        cuvs::neighbors::cagra::build(impl_->handle, params, dataset)
    );
    impl_->is_cagra = true;
    impl_->is_vamana = false;
    
    raft::resource::sync_stream(impl_->handle);
}

void GPUIndex::build_vamana_index(const VectorBuffer& vectors, const VamanaConfig& config) {
    cuvs::neighbors::vamana::index_params params;
    params.graph_degree = config.graph_degree;
    
    size_t num_vectors = vectors.size();
    size_t dims = vectors.dimensions();
    const float* host_data = vectors.data();
    
    auto dataset = raft::make_device_matrix_view<const float, int64_t>(host_data, num_vectors, dims);
    
    impl_->vamana_idx = std::make_unique<cuvs::neighbors::vamana::index<float, uint32_t>>(
        cuvs::neighbors::vamana::build(impl_->handle, params, dataset)
    );
    impl_->is_vamana = true;
    impl_->is_cagra = false;
    
    raft::resource::sync_stream(impl_->handle);
}

std::vector<NodeID> GPUIndex::search(const float* query, int top_k, const std::vector<NodeID>& allowed_ids) {
    if (!impl_->is_cagra && !impl_->is_vamana) {
        throw std::runtime_error("Index not built");
    }
    
    size_t dims = impl_->is_cagra ? impl_->cagra_idx->dim() : impl_->vamana_idx->dim();
    
    // Copy query to device memory
    rmm::device_uvector<float> d_query(dims, impl_->handle.get_stream());
    raft::copy(d_query.data(), query, dims, impl_->handle.get_stream());
    auto query_view = raft::make_device_matrix_view<const float, int64_t>(d_query.data(), 1, dims);
    
    int search_k = allowed_ids.empty() ? top_k : std::min(top_k * 50, 10000);
    
    rmm::device_uvector<NodeID> d_neighbors(search_k, impl_->handle.get_stream());
    rmm::device_uvector<float> d_distances(search_k, impl_->handle.get_stream());
    
    auto neighbors_view = raft::make_device_matrix_view<NodeID, int64_t>(d_neighbors.data(), 1, search_k);
    auto distances_view = raft::make_device_matrix_view<float, int64_t>(d_distances.data(), 1, search_k);
    
    if (impl_->is_cagra) {
        cuvs::neighbors::cagra::search_params params;
        cuvs::neighbors::cagra::search(impl_->handle, params, *impl_->cagra_idx, query_view, neighbors_view, distances_view);
    } else {
        cuvs::neighbors::vamana::search_params params;
        cuvs::neighbors::vamana::search(impl_->handle, params, *impl_->vamana_idx, query_view, neighbors_view, distances_view);
    }
    
    std::vector<NodeID> h_neighbors(search_k);
    raft::copy(h_neighbors.data(), d_neighbors.data(), search_k, impl_->handle.get_stream());
    
    raft::resource::sync_stream(impl_->handle);
    
    if (allowed_ids.empty()) {
        h_neighbors.resize(std::min(h_neighbors.size(), (size_t)top_k));
        return h_neighbors;
    }
    
    std::vector<NodeID> filtered;
    for (NodeID id : h_neighbors) {
        if (std::find(allowed_ids.begin(), allowed_ids.end(), id) != allowed_ids.end()) {
            filtered.push_back(id);
            if (filtered.size() == (size_t)top_k) break;
        }
    }
    return filtered;
}

void GPUIndex::save(const std::string& path) const {
    if (impl_->is_cagra) {
        cuvs::neighbors::cagra::serialize(impl_->handle, path, *impl_->cagra_idx);
    } else if (impl_->is_vamana) {
        cuvs::neighbors::vamana::serialize(impl_->handle, path, *impl_->vamana_idx);
    } else {
        throw std::runtime_error("Index not built, cannot save");
    }
}

void GPUIndex::load(const std::string& path) {
    // Note: in a real production system you might want metadata to identify index type.
    // Assuming CAGRA for this load path as default.
    impl_->cagra_idx = std::make_unique<cuvs::neighbors::cagra::index<float, uint32_t>>();
    cuvs::neighbors::cagra::deserialize(impl_->handle, path, *impl_->cagra_idx);
    impl_->is_cagra = true;
    impl_->is_vamana = false;
}

} // namespace axiomgraph
