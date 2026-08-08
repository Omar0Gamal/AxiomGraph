#include "httplib.h"
#include "json.hpp"
#include "memory_arena.hpp"
#include "hot_graph.hpp"
#include "csr_graph.hpp"
#include "cpu_index.hpp"
#include "vector_buffer.hpp"

#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <shared_mutex>
#include <iostream>
#include <set>
#include <algorithm>
#include "sqlite3.h"

using json = nlohmann::json;
using namespace axiomgraph;

struct NodeMeta {
    uint32_t id;
    std::string label;
    std::string type;
    json properties;
};

struct EdgeMeta {
    uint32_t source_id;
    uint32_t target_id;
    std::string relation;
    float weight;
};

class MetadataStore {
public:
    MetadataStore(const std::string& db_path) {
        if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
            throw std::runtime_error("Failed to open metadata db");
        }
        sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
        sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
        
        sqlite3_exec(db_, "CREATE TABLE IF NOT EXISTS nodes (id INTEGER PRIMARY KEY, label TEXT, type TEXT, properties TEXT);", nullptr, nullptr, nullptr);
        sqlite3_exec(db_, "CREATE TABLE IF NOT EXISTS edges (source INTEGER, target INTEGER, relation TEXT, weight REAL);", nullptr, nullptr, nullptr);
        
        sqlite3_prepare_v2(db_, "INSERT OR REPLACE INTO nodes (id, label, type, properties) VALUES (?, ?, ?, ?);", -1, &insert_stmt_, nullptr);
        sqlite3_prepare_v2(db_, "SELECT label, type, properties FROM nodes WHERE id = ?;", -1, &select_stmt_, nullptr);
        sqlite3_prepare_v2(db_, "DELETE FROM nodes WHERE id = ?;", -1, &delete_stmt_, nullptr);
        
        sqlite3_prepare_v2(db_, "INSERT INTO edges (source, target, relation, weight) VALUES (?, ?, ?, ?);", -1, &insert_edge_stmt_, nullptr);
        sqlite3_prepare_v2(db_, "DELETE FROM edges WHERE source = ? OR target = ?;", -1, &delete_edge_stmt_, nullptr);
    }
    
    ~MetadataStore() {
        sqlite3_finalize(insert_stmt_);
        sqlite3_finalize(select_stmt_);
        sqlite3_finalize(delete_stmt_);
        sqlite3_finalize(insert_edge_stmt_);
        sqlite3_finalize(delete_edge_stmt_);
        sqlite3_close(db_);
    }

    void put_node(uint32_t id, const std::string& label, const std::string& type, const std::string& props) {
        std::lock_guard<std::mutex> lock(mutex_);
        sqlite3_bind_int(insert_stmt_, 1, id);
        sqlite3_bind_text(insert_stmt_, 2, label.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert_stmt_, 3, type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert_stmt_, 4, props.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(insert_stmt_);
        sqlite3_reset(insert_stmt_);
    }

    bool get_node(uint32_t id, NodeMeta& meta) {
        std::lock_guard<std::mutex> lock(mutex_);
        sqlite3_bind_int(select_stmt_, 1, id);
        bool found = false;
        if (sqlite3_step(select_stmt_) == SQLITE_ROW) {
            meta.id = id;
            meta.label = reinterpret_cast<const char*>(sqlite3_column_text(select_stmt_, 0));
            meta.type = reinterpret_cast<const char*>(sqlite3_column_text(select_stmt_, 1));
            meta.properties = json::parse(reinterpret_cast<const char*>(sqlite3_column_text(select_stmt_, 2)));
            found = true;
        }
        sqlite3_reset(select_stmt_);
        return found;
    }

    void remove_node(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        sqlite3_bind_int(delete_stmt_, 1, id);
        sqlite3_step(delete_stmt_);
        sqlite3_reset(delete_stmt_);
        
        sqlite3_bind_int(delete_edge_stmt_, 1, id);
        sqlite3_bind_int(delete_edge_stmt_, 2, id);
        sqlite3_step(delete_edge_stmt_);
        sqlite3_reset(delete_edge_stmt_);
    }

    void put_edge(uint32_t source, uint32_t target, const std::string& relation, float weight) {
        std::lock_guard<std::mutex> lock(mutex_);
        sqlite3_bind_int(insert_edge_stmt_, 1, source);
        sqlite3_bind_int(insert_edge_stmt_, 2, target);
        sqlite3_bind_text(insert_edge_stmt_, 3, relation.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(insert_edge_stmt_, 4, weight);
        sqlite3_step(insert_edge_stmt_);
        sqlite3_reset(insert_edge_stmt_);
    }

    std::vector<EdgeMeta> get_edges(const std::set<uint32_t>& context_nodes) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<EdgeMeta> edges;
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_, "SELECT source, target, relation, weight FROM edges;", -1, &stmt, nullptr);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            uint32_t s = sqlite3_column_int(stmt, 0);
            uint32_t t = sqlite3_column_int(stmt, 1);
            if (context_nodes.count(s) && context_nodes.count(t)) {
                edges.push_back({
                    s, t, 
                    reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)), 
                    static_cast<float>(sqlite3_column_double(stmt, 3))
                });
            }
        }
        sqlite3_finalize(stmt);
        return edges;
    }

    std::vector<uint32_t> query_filter(const json& filter) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string sql = "SELECT id FROM nodes WHERE 1=1";
        if (filter.contains("type")) {
            sql += " AND type = '" + filter["type"].get<std::string>() + "'";
        }
        for (auto it = filter.begin(); it != filter.end(); ++it) {
            if (it.key() == "type") continue;
            sql += " AND json_extract(properties, '$." + it.key() + "') = ";
            if (it.value().is_string()) {
                sql += "'" + it.value().get<std::string>() + "'";
            } else {
                sql += it.value().dump();
            }
        }
        
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        std::vector<uint32_t> ids;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ids.push_back(sqlite3_column_int(stmt, 0));
        }
        sqlite3_finalize(stmt);
        return ids;
    }
    
    std::vector<uint32_t> get_all_node_ids() {
        std::lock_guard<std::mutex> lock(mutex_);
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_, "SELECT id FROM nodes;", -1, &stmt, nullptr);
        std::vector<uint32_t> ids;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ids.push_back(sqlite3_column_int(stmt, 0));
        }
        sqlite3_finalize(stmt);
        return ids;
    }

private:
    sqlite3* db_ = nullptr;
    sqlite3_stmt* insert_stmt_ = nullptr;
    sqlite3_stmt* select_stmt_ = nullptr;
    sqlite3_stmt* delete_stmt_ = nullptr;
    sqlite3_stmt* insert_edge_stmt_ = nullptr;
    sqlite3_stmt* delete_edge_stmt_ = nullptr;
    std::mutex mutex_;
};

class GraphEngine {
public:
    GraphEngine(int dimensions) : dimensions_(dimensions), vectors_(dimensions), metadata_("metadata.db") {}

    uint32_t add_node(const std::vector<float>& vec, const std::string& label, const std::string& type, const json& props) {
        std::unique_lock<std::shared_mutex> lock(rw_mutex_);
        if (vec.size() != dimensions_) throw std::invalid_argument("Vector dimension mismatch");
        
        uint32_t node_id = arena_.allocate_id();
        vectors_.add_vector(node_id, vec.data());
        
        metadata_.put_node(node_id, label, type, props.dump());
        return node_id;
    }

    void delete_node(uint32_t node_id) {
        std::unique_lock<std::shared_mutex> lock(rw_mutex_);
        arena_.free_id(node_id);
        hot_graph_.remove_node(node_id);
        metadata_.remove_node(node_id);
    }

    void connect(uint32_t source, uint32_t target, const std::string& relation, float weight, bool bidirectional, const std::string& rev_relation) {
        std::unique_lock<std::shared_mutex> lock(rw_mutex_);
        uint32_t rel_hash = std::hash<std::string>{}(relation) & 0xFFFFFFFF;
        hot_graph_.add_edge(source, target, weight, rel_hash);
        metadata_.put_edge(source, target, relation, weight);

        if (bidirectional) {
            std::string rev_rel = rev_relation.empty() ? ("REV_" + relation) : rev_relation;
            uint32_t rev_hash = std::hash<std::string>{}(rev_rel) & 0xFFFFFFFF;
            hot_graph_.add_edge(target, source, weight, rev_hash);
            metadata_.put_edge(target, source, rev_rel, weight);
        }
    }

    void consolidate() {
        std::unique_lock<std::shared_mutex> lock(rw_mutex_);
        csr_graph_.build_from_hot(hot_graph_);
        cpu_index_.build_index(vectors_);
        // Clear hot graph after consolidation
        hot_graph_.clear(); 
    }

    json query(const std::vector<float>& vec, int top_k, int depth, bool search_hot, const json& filter) {
        std::shared_lock<std::shared_mutex> lock(rw_mutex_);
        
        std::vector<uint32_t> allowed_ids;
        if (!filter.is_null() && filter.is_object() && !filter.empty()) {
            allowed_ids = metadata_.query_filter(filter);
            if (allowed_ids.empty()) return {{"nodes", json::array()}, {"edges", json::array()}};
        } else {
            allowed_ids = metadata_.get_all_node_ids();
        }

        std::vector<uint32_t> initial_ids = cpu_index_.search(vectors_, vec.data(), top_k, allowed_ids);
        
        std::set<uint32_t> context_nodes(initial_ids.begin(), initial_ids.end());
        std::vector<uint32_t> frontier = initial_ids;

        for (int d = 0; d < depth; ++d) {
            std::vector<uint32_t> next_frontier;
            for (uint32_t nid : frontier) {
                auto csr_edges = csr_graph_.get_neighbors(nid);
                std::vector<uint32_t> neighbors(csr_edges.begin(), csr_edges.end());

                if (search_hot) {
                    auto hot_edges = hot_graph_.get_neighbors(nid);
                    for (const auto& e : hot_edges) neighbors.push_back(e.target);
                }

                for (uint32_t nbr : neighbors) {
                    if (context_nodes.find(nbr) == context_nodes.end()) {
                        context_nodes.insert(nbr);
                        next_frontier.push_back(nbr);
                    }
                }
            }
            frontier = next_frontier;
        }

        json res_nodes = json::array();
        for (uint32_t nid : context_nodes) {
            NodeMeta n;
            if (metadata_.get_node(nid, n)) {
                res_nodes.push_back({
                    {"id", n.id},
                    {"label", n.label},
                    {"type", n.type},
                    {"properties", n.properties}
                });
            }
        }

        json res_edges = json::array();
        auto edges = metadata_.get_edges(context_nodes);
        for (const auto& e : edges) {
            res_edges.push_back({
                {"source_id", e.source_id},
                {"target_id", e.target_id},
                {"relation", e.relation},
                {"weight", e.weight}
            });
        }

        return {
            {"nodes", res_nodes},
            {"edges", res_edges}
        };
    }

    void save(const std::string& prefix) {
        std::unique_lock<std::shared_mutex> lock(rw_mutex_);
        vectors_.save(prefix + "_vectors.bin");
        hot_graph_.save(prefix + "_hot.bin");
        csr_graph_.save(prefix + "_csr.bin");
        cpu_index_.save(prefix + "_index");
    }

    void load(const std::string& prefix) {
        std::unique_lock<std::shared_mutex> lock(rw_mutex_);
        try { vectors_.load(prefix + "_vectors.bin"); } catch(...) {}
        try { hot_graph_.load(prefix + "_hot.bin"); } catch(...) {}
        try { csr_graph_.load(prefix + "_csr.bin"); } catch(...) {}
        try { cpu_index_.load(prefix + "_index"); } catch(...) {}
    }

private:
    int dimensions_;
    std::shared_mutex rw_mutex_;
    
    MemoryArena arena_;
    VectorBuffer vectors_;
    HotGraph hot_graph_;
    CSRGraph csr_graph_;
    CPUIndex cpu_index_;
    MetadataStore metadata_;
};

int main() {
    httplib::Server svr;
    GraphEngine engine(768); // Default 768 dimensions

    svr.Post("/node", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            auto vec = j["vector"].get<std::vector<float>>();
            std::string label = j["label"];
            std::string type = j["type"];
            json props = j.value("properties", json::object());

            uint32_t id = engine.add_node(vec, label, type, props);
            res.set_content(json{{"status", "success"}, {"id", id}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    svr.Delete(R"(/node/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            uint32_t id = std::stoul(req.matches[1]);
            engine.delete_node(id);
            res.set_content(json{{"status", "success"}, {"id", id}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    svr.Post("/connect", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            uint32_t source = j["source"];
            uint32_t target = j["target"];
            std::string relation = j["relation"];
            float weight = j.value("weight", 1.0f);
            bool bidi = j.value("bidirectional", false);
            std::string rev_rel = j.value("reverse_relation", "");

            engine.connect(source, target, relation, weight, bidi, rev_rel);
            res.set_content(json{{"status", "success"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    svr.Post("/consolidate", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            engine.consolidate();
            res.set_content(json{{"status", "success"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    svr.Post("/save", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            engine.save("axiom_db");
            res.set_content(json{{"status", "success"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    svr.Post("/load", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            engine.load("axiom_db");
            res.set_content(json{{"status", "success"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    svr.Post("/query", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            auto vec = j["vector"].get<std::vector<float>>();
            int top_k = j.value("top_k", 5);
            int depth = j.value("depth", 1);
            bool search_hot = j.value("search_hot", true);
            json filter = j.value("filter_dict", json::object());

            json result = engine.query(vec, top_k, depth, search_hot, filter);
            res.set_content(result.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    std::cout << "AxiomGraph Native C++ HTTP Server listening on port 8000...\n";
    svr.listen("0.0.0.0", 8000);
    return 0;
}
