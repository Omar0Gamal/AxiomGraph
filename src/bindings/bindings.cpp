#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/string.h>
#include <nanobind/ndarray.h>
#include "memory_arena.hpp"
#include "hot_graph.hpp"
#include "vector_buffer.hpp"
#include "csr_graph.hpp"
#include "cpu_index.hpp"
#include <stdexcept>

#ifdef USE_GPU_INDEX
#include "gpu_index.hpp"
#endif

namespace nb = nanobind;
using namespace axiomgraph;

static std::vector<NodeID> cpu_search_ext(const CPUIndex& idx, const VectorBuffer& vecs, nb::ndarray<float, nb::ndim<1>, nb::c_contig> query, int top_k, const std::vector<NodeID>& allowed_ids) {
    return idx.search(vecs, query.data(), top_k, allowed_ids);
}

#ifdef USE_GPU_INDEX
static std::vector<NodeID> gpu_search_ext(GPUIndex& idx, nb::ndarray<float, nb::ndim<1>, nb::c_contig> query, int top_k, const std::vector<NodeID>& allowed_ids) {
    return idx.search(query.data(), top_k, allowed_ids);
}
#endif

NB_MODULE(ext, m) {
    m.doc() = "AxiomGraph C++/CUDA bindings for Python";

    nb::class_<MemoryArena>(m, "MemoryArena")
        .def(nb::init<>())
        .def("allocate_id", &MemoryArena::allocate_id)
        .def("free_id", &MemoryArena::free_id)
        .def("active_count", &MemoryArena::active_count)
        .def("capacity", &MemoryArena::capacity);

    nb::class_<Edge>(m, "Edge")
        .def_rw("target", &Edge::target)
        .def_rw("weight", &Edge::weight)
        .def_rw("type_hash", &Edge::type_hash);

    nb::class_<HotGraph>(m, "HotGraph")
        .def(nb::init<>())
        .def("add_edge", &HotGraph::add_edge, nb::arg("source"), nb::arg("target"), nb::arg("weight"), nb::arg("type_hash"))
        .def("remove_node", &HotGraph::remove_node, nb::arg("source"))
        .def("get_neighbors", &HotGraph::get_neighbors, nb::arg("source"))
        .def("size", &HotGraph::size)
        .def("save", &HotGraph::save, nb::arg("path"))
        .def("load", &HotGraph::load, nb::arg("path"));

    nb::class_<VectorBuffer>(m, "VectorBuffer")
        .def(nb::init<size_t>(), nb::arg("dimensions"))
        .def("size", &VectorBuffer::size)
        .def("dimensions", &VectorBuffer::dimensions)
        .def("add_vector", [](VectorBuffer& vb, NodeID id, nb::ndarray<float, nb::ndim<1>, nb::c_contig> vec) {
            if (vec.shape(0) != vb.dimensions()) {
                throw std::invalid_argument("Vector dimensions do not match VectorBuffer dimensions.");
            }
            vb.add_vector(id, vec.data());
        }, nb::arg("id"), nb::arg("vec"))
        .def("save", &VectorBuffer::save, nb::arg("path"))
        .def("load", &VectorBuffer::load, nb::arg("path"));

    nb::class_<CSRGraph>(m, "CSRGraph")
        .def(nb::init<>())
        .def("build_from_hot", &CSRGraph::build_from_hot, nb::arg("hot"))
        .def("get_neighbors", &CSRGraph::get_neighbors, nb::arg("source"))
        .def("row_ptr", &CSRGraph::row_ptr)
        .def("col_ind", &CSRGraph::col_ind)
        .def("weights", &CSRGraph::weights)
        .def("type_hashes", &CSRGraph::type_hashes)
        .def("save", &CSRGraph::save, nb::arg("path"))
        .def("load", &CSRGraph::load, nb::arg("path"));

    nb::class_<CPUIndex>(m, "CPUIndex")
        .def(nb::init<>())
        .def("build_index", &CPUIndex::build_index, nb::arg("vectors"))
        .def("search", &cpu_search_ext, nb::arg("vectors"), nb::arg("query"), nb::arg("top_k"), nb::arg("allowed_ids"))
        .def("save", &CPUIndex::save, nb::arg("path"))
        .def("load", &CPUIndex::load, nb::arg("path"));

#ifdef USE_GPU_INDEX
    nb::class_<CagraConfig>(m, "CagraConfig")
        .def(nb::init<>())
        .def_rw("graph_degree", &CagraConfig::graph_degree)
        .def_rw("intermediate_graph_degree", &CagraConfig::intermediate_graph_degree);

    nb::class_<VamanaConfig>(m, "VamanaConfig")
        .def(nb::init<>())
        .def_rw("graph_degree", &VamanaConfig::graph_degree);

    nb::class_<GPUIndex>(m, "GPUIndex")
        .def(nb::init<>())
        .def("build_cagra_index", &GPUIndex::build_cagra_index, nb::arg("vectors"), nb::arg("config"))
        .def("build_vamana_index", &GPUIndex::build_vamana_index, nb::arg("vectors"), nb::arg("config"))
        .def("search", &gpu_search_ext, nb::arg("query"), nb::arg("top_k"), nb::arg("allowed_ids"))
        .def("save", &GPUIndex::save, nb::arg("path"))
        .def("load", &GPUIndex::load, nb::arg("path"));
#endif
}
