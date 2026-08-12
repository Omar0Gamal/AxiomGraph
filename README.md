# AxiomGraph

AxiomGraph is a hybrid graph-vector database designed for GraphRAG workloads and agentic systems. It combines multi-dimensional vector search with a mutable adjacency list graph within a unified C++ engine.

## Features

*   **C++ Execution Engine**: The core engine manages graph topologies (`HotGraph`, `CSRGraph`) and memory allocation (`MemoryArena`) with fine-grained locking and lock striping for high concurrency.
*   **Vector Search**: Supports HNSW (Hierarchical Navigable Small World) for CPU-based vector indexing, with fallbacks to GPU-accelerated indices (`cuVS` / `cagra`) when hardware is available.
*   **Embedded Metadata Storage**: Uses embedded SQLite to store node metadata (labels, types, properties), reducing memory overhead compared to in-memory JSON storage.
*   **Python Bindings**: Provides a Python SDK via `nanobind` for direct integration into Python applications.
*   **HTTP API**: Includes a standalone C++ HTTP server for production deployments requiring remote access.

## Repository Layout

```text
AxiomGraph/
├── CMakeLists.txt
├── README.md
├── CONTRIBUTING.md
├── LICENSE
├── docs/                 # Documentation and architecture references
├── examples/             # Example applications
├── src/
│   ├── bindings/         # Python bindings (nanobind)
│   ├── core/             # Core C++ graph and vector engine
│   └── server/           # C++ HTTP API server
├── tests/                # C++ unit tests (GoogleTest)
└── third_party/          # Dependencies (SQLite, HNSW, httplib, JSON)
```

## Getting Started

### Building the C++ Engine

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Starting the HTTP Server

```bash
./build/server_http
```

### Installing the Python SDK

```bash
pip install .
```

## Python SDK Usage

```python
from axiomgraph.memory import Memory

# Initialize Memory Store
db = Memory(dimensions=768)

# Insert nodes with embeddings
db.hot.add_node(
    vector=[0.1, 0.4, ...],
    label="Refund Policy", 
    type="POLICY", 
    properties={"days": 30}
)

# Consolidate mutable graph into static CSR graph for optimized querying
db.consolidate()

# Perform hybrid search (Vector KNN + Graph Traversal)
results = db.query(
    vector=[0.1, 0.5, ...], 
    top_k=2, 
    depth=1, 
    filter_dict={"type": "POLICY"}
)
```

## HTTP API Endpoints

*   `POST /node`: Insert a node with vector and metadata.
*   `DELETE /node/{id}`: Delete a node, vector, and related edges.
*   `POST /connect`: Create a relationship between nodes.
*   `POST /consolidate`: Flush the mutable `HotGraph` into the `CSRGraph` and build indices.
*   `POST /query`: Perform a semantic vector search combined with graph traversal.
*   `POST /save`: Serialize the database to disk.
*   `POST /load`: Load a serialized database into memory.

## License
MIT License. See [LICENSE](LICENSE) for details.
