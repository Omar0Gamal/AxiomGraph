#include <gtest/gtest.h>
#include "memory_arena.hpp"
#include "vector_buffer.hpp"
#include "hot_graph.hpp"
#include "csr_graph.hpp"
#include "cpu_index.hpp"
#include <vector>

using namespace axiomgraph;

TEST(MemoryArenaTest, AllocateAndFree) {
    MemoryArena arena;
    EXPECT_EQ(arena.active_count(), 0);
    
    NodeID id1 = arena.allocate_id();
    NodeID id2 = arena.allocate_id();
    
    EXPECT_EQ(id1, 0);
    EXPECT_EQ(id2, 1);
    EXPECT_EQ(arena.active_count(), 2);
    
    arena.free_id(id1);
    EXPECT_EQ(arena.active_count(), 1);
    
    NodeID id3 = arena.allocate_id();
    EXPECT_EQ(id3, 0); // Should reuse freed ID
    EXPECT_EQ(arena.active_count(), 2);
}

TEST(VectorBufferTest, AddAndRetrieve) {
    size_t dims = 4;
    VectorBuffer vb(dims);
    
    std::vector<float> vec1 = {1.0, 2.0, 3.0, 4.0};
    std::vector<float> vec2 = {0.1, 0.2, 0.3, 0.4};
    
    vb.add_vector(0, vec1.data());
    vb.add_vector(1, vec2.data());
    
    EXPECT_EQ(vb.size(), 2);
    EXPECT_EQ(vb.dimensions(), dims);
    
    const float* data = vb.data();
    EXPECT_FLOAT_EQ(data[0], 1.0f); // vec1[0]
    EXPECT_FLOAT_EQ(data[4], 0.1f); // vec2[0]
}

TEST(GraphTest, HotToCSRConversion) {
    HotGraph hot;
    hot.add_edge(0, 1, 1.0f, 123);
    hot.add_edge(0, 2, 0.5f, 456);
    hot.add_edge(2, 0, 0.1f, 789);
    
    EXPECT_EQ(hot.size(), 3);
    
    auto neighbors_0 = hot.get_neighbors(0);
    EXPECT_EQ(neighbors_0.size(), 2);
    EXPECT_EQ(neighbors_0[0].target, 1);
    
    CSRGraph csr;
    csr.build_from_hot(hot);
    
    const auto& row_ptr = csr.row_ptr();
    const auto& col_ind = csr.col_ind();
    const auto& weights = csr.weights();
    
    EXPECT_EQ(row_ptr.size(), 4); // max node is 2 -> size is 3 + 1
    EXPECT_EQ(row_ptr[0], 0);
    EXPECT_EQ(row_ptr[1], 2); // node 0 has 2 edges
    EXPECT_EQ(row_ptr[2], 2); // node 1 has 0 edges
    EXPECT_EQ(row_ptr[3], 3); // node 2 has 1 edge
    
    EXPECT_EQ(col_ind[0], 1);
    EXPECT_EQ(col_ind[1], 2);
    EXPECT_EQ(col_ind[2], 0);
    
    EXPECT_FLOAT_EQ(weights[0], 1.0f);
    EXPECT_FLOAT_EQ(weights[1], 0.5f);
    EXPECT_FLOAT_EQ(weights[2], 0.1f);
}

TEST(CPUIndexTest, ExactSearch) {
    VectorBuffer vb(2);
    std::vector<float> v0 = {1.0, 0.0};
    std::vector<float> v1 = {0.0, 1.0};
    std::vector<float> v2 = {0.5, 0.5};
    
    vb.add_vector(0, v0.data());
    vb.add_vector(1, v1.data());
    vb.add_vector(2, v2.data());
    
    CPUIndex idx;
    idx.build_index(vb);
    
    std::vector<float> query = {1.0, 0.0};
    auto results = idx.search(vb, query.data(), 2, std::vector<NodeID>{});
    
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], 0); // v0 is exact match
    EXPECT_EQ(results[1], 2); // v2 is next closest
}

TEST(CPUIndexTest, FilteredSearch) {
    VectorBuffer vb(2);
    std::vector<float> v0 = {1.0, 0.0};
    std::vector<float> v1 = {0.0, 1.0};
    std::vector<float> v2 = {0.9, 0.1};
    
    vb.add_vector(0, v0.data());
    vb.add_vector(1, v1.data());
    vb.add_vector(2, v2.data());
    
    CPUIndex idx;
    idx.build_index(vb);
    
    std::vector<float> query = {1.0, 0.0};
    // only allow nodes 1 and 2
    auto results = idx.search(vb, query.data(), 2, std::vector<NodeID>{1, 2});
    
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], 2); // v2 is closest among allowed
    EXPECT_EQ(results[1], 1);
}
