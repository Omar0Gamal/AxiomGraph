# AxiomGraph API Reference

This document outlines the public API for interacting with the `axiomgraph` core engine.

---

## `axiomgraph.Memory`

The primary entry point for the AxiomGraph engine.

### `Memory.create(path: str, dimensions: int, use_gpu: bool = True) -> Memory`
Class method to initialize a new database or load an existing one from disk.
* **`path`** *(str)*: Base file path prefix for saving/loading the database state.
* **`dimensions`** *(int)*: Dimensionality of the vectors being stored.
* **`use_gpu`** *(bool)*: Whether to attempt initializing the GPU-accelerated index (using cuVS/RAFT). Gracefully falls back to CPU if unavailable.
* **Returns**: A new `Memory` instance.

### `Memory(path: str, dimensions: int, use_gpu: bool = True)`
Standard constructor, identical arguments to `create()`.

### `memory.hot`
Returns the `HotMemoryManager` instance attached to this memory block, used for modifying the working graph in real-time.

### `memory.consolidate(keep_in_hot: bool = False)`
The Hot-to-Cold transition trigger. Flattens all nodes and edges from the mutable Hot Memory into highly optimized CSR arrays and builds the Vector Search index.
* **`keep_in_hot`** *(bool)*: If `False`, the Hot Memory is cleared after consolidation.

### `memory.query(vector: np.ndarray, top_k: int = 5, depth: int = 1, search_hot: bool = True, filter_dict: dict = None) -> GraphContext`
Performs a hybrid cognitive search using GraphRAG techniques.
* **`vector`** *(np.ndarray)*: The query vector (dense embedding).
* **`top_k`** *(int)*: Number of initial semantic matches to retrieve via Vector Search.
* **`depth`** *(int)*: Graph traversal depth (BFS steps) starting from the `top_k` nodes.
* **`search_hot`** *(bool)*: If `True`, includes edges from the un-consolidated Hot Memory in the traversal.
* **`filter_dict`** *(dict, optional)*: Metadata filters to apply before search (e.g., `{"type": "Person"}`).
* **Returns**: A `GraphContext` object containing the localized subgraph.

### `memory.save()`
Flushes the current state of both Hot and Cold memory to disk atomically.

### `memory.load()`
Loads the memory state from disk using the `path` prefix provided during initialization.

---

## `axiomgraph.HotMemoryManager`

Manages the CPU-bound Episodic Memory. This is a highly mutable scratchpad where agents can write facts, create nodes, and link relationships in real-time without triggering expensive GPU index rebuilds. Accessed via `memory.hot`.

### `add_node(vector: np.ndarray, label: str, type_: str, properties: dict = None) -> int`
Adds a new entity to the working memory graph.
* **`vector`** *(np.ndarray)*: The vector embedding representing the node.
* **`label`** *(str)*: Human-readable name or label for the node.
* **`type_`** *(str)*: The semantic type/category of the node.
* **`properties`** *(dict, optional)*: Arbitrary key-value metadata.
* **Returns**: The generated `node_id` (int).

### `delete_node(node_id: int)`
Deletes a node from memory.
* **`node_id`** *(int)*: The ID of the node to delete.

### `connect(source: int, target: int, relation: str, weight: float = 1.0, bidirectional: bool = False, reverse_relation: str = None)`
Creates a directed, weighted edge between two nodes.
* **`source`** *(int)*: The ID of the source node.
* **`target`** *(int)*: The ID of the target node.
* **`relation`** *(str)*: String descriptor of the relationship (e.g., "KNOWS").
* **`weight`** *(float)*: Edge weight/strength.
* **`bidirectional`** *(bool)*: If `True`, automatically creates an inverse edge.
* **`reverse_relation`** *(str, optional)*: Explicit label for the reverse edge. If omitted, defaults to `"REV_{relation}"`.

---

## Data Models

AxiomGraph returns structured data models representing semantic entities and relationships.

### `axiomgraph.Node`
Represents a semantic entity in the cognitive graph.
* **`id`** *(int)*: Unique identifier.
* **`label`** *(str)*: Human-readable label.
* **`type`** *(str)*: Entity category.
* **`properties`** *(dict)*: Dictionary of metadata.

### `axiomgraph.Edge`
Represents a directed logical relationship between two entities.
* **`source_id`** *(int)*: Origin node ID.
* **`target_id`** *(int)*: Destination node ID.
* **`relation`** *(str)*: Relationship type descriptor.
* **`weight`** *(float)*: Edge strength (default: `1.0`).
* **`source_label`** *(str)*: The label of the source node (if available).
* **`target_label`** *(str)*: The label of the target node (if available).

### `axiomgraph.GraphContext`
The localized GraphRAG context retrieved during a `memory.query()`.
* **`nodes`** *(List[Node])*: List of nodes retrieved in the search.
* **`edges`** *(List[Edge])*: List of edges connecting the retrieved nodes.
