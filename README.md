# AxiomGraph

[![Build Status](https://github.com/Omar0Gamal/AxiomGraph/actions/workflows/build_wheels.yml/badge.svg)](https://github.com/Omar0Gamal/AxiomGraph/actions/workflows/build_wheels.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

AxiomGraph is a hybrid graph-vector database engineered for GraphRAG workloads and agentic AI systems. It combines multi-dimensional vector search with a mutable adjacency list graph within a unified, highly concurrent C++ engine.

## Features

*   **Hybrid Querying**: Perform semantic vector search combined with deep graph traversals (GraphRAG).
*   **High Concurrency**: Core engine utilizes fine-grained locking and lock striping for thread-safe mutations.
*   **Hardware Acceleration**: HNSW for CPU-based vector indexing, with seamless fallbacks to GPU-accelerated indices (`cuVS` / `cagra`) on supported hardware.
*   **Memory Efficiency**: Embedded SQLite for node metadata (labels, properties) reduces memory overhead compared to in-memory JSON storage.
*   **Flexible Access**: Direct integration via Python SDK (`nanobind`) or remote access via a standalone C++ HTTP server.

## Installation

### Python SDK

We publish two variants of the pre-compiled wheels on our [GitHub Releases](https://github.com/Omar0Gamal/AxiomGraph/releases) page:
*   **`axiomgraph`**: Full GPU acceleration via CUDA and RAPIDS (`cuVS`). Large wheel size.
*   **`axiomgraph-cpu`**: CPU-only fallback using HNSW. Lightweight wheel size (~650 KB).

1. Navigate to the [Releases](https://github.com/Omar0Gamal/AxiomGraph/releases) page.
2. Download the `.whl` file matching your OS, Python version, and preferred variant.
3. Install directly via URL:

```bash
# GPU version for Linux (Python 3.10)
pip install https://github.com/Omar0Gamal/AxiomGraph/releases/download/v0.1.0/axiomgraph-0.1.0-cp310-cp310-manylinux_2_17_x86_64.manylinux2014_x86_64.whl

# CPU-only version for Linux (Python 3.10)
pip install https://github.com/Omar0Gamal/AxiomGraph/releases/download/v0.1.0/axiomgraph_cpu-0.1.0-cp310-cp310-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
```

#### Building from Source

To compile locally, a C++17 compiler is required. For GPU acceleration on Linux/Windows, ensure the **CUDA Toolkit** (12.x) is installed.

```bash
git clone https://github.com/Omar0Gamal/AxiomGraph.git
cd AxiomGraph
pip install .
```

### Standalone C++ Server

To build the standalone HTTP REST API server:

```bash
mkdir build && cd build
cmake .. -DENABLE_CUDA=ON  # Set OFF for macOS/CPU-only
cmake --build .
./server_http
```

## Quick Start (Python)

```python
from axiomgraph.memory import Memory

# Initialize Memory Store
db = Memory(path="./data/my_graph", dimensions=768, use_gpu=True)

# Insert nodes with embeddings
node_id = db.hot.add_node(
    vector=[0.1, 0.4, ...],
    label="Refund Policy", 
    type="POLICY", 
    properties={"days": 30}
)

# Connect nodes
db.hot.connect(source=1, target=2, relation="RELATED_TO")

# Consolidate mutable graph into static CSR graph for optimized querying
db.consolidate()

# Perform hybrid search (Vector KNN + Graph Traversal)
results = db.query(
    vector=[0.1, 0.5, ...], 
    top_k=5, 
    depth=2, 
    filter_dict={"type": "POLICY"}
)

# Save state to disk
db.save()
```

## Documentation

* [Python API Reference](docs/api_reference.md)
* [HTTP API Specification](#http-api)

### HTTP API
When running `server_http`, the following endpoints are exposed:
*   `POST /node`: Insert a node with vector and metadata.
*   `DELETE /node/{id}`: Delete a node, vector, and related edges.
*   `POST /connect`: Create a relationship between nodes.
*   `POST /consolidate`: Flush the mutable `HotGraph` into the `CSRGraph` and build indices.
*   `POST /query`: Perform a semantic vector search combined with graph traversal.
*   `POST /save`: Serialize the database to disk.
*   `POST /load`: Load a serialized database into memory.

## License

AxiomGraph is released under the [MIT License](LICENSE).
