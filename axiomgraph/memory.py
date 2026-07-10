import os
import json
import numpy as np
from typing import Optional
import threading

# Graceful import of the C++ backend
try:
    from . import ext
except ImportError as e:
    raise ImportError(f"AxiomGraph C++ extension not found. Did you compile it? Error: {e}")

from .models import Node, Edge, GraphContext

class HotMemoryManager:
    """
    Manages the CPU-bound Episodic Memory.
    A highly mutable scratchpad where agents can write facts, create nodes, 
    and link relationships in real-time without triggering GPU index rebuilds.
    """
    def __init__(self, memory: 'Memory'):
        self._memory = memory
        self._nodes = {}  # NodeID -> Node mapping

    def add_node(self, vector: np.ndarray, label: str, type_: str, properties: Optional[dict] = None) -> int:
        """Adds a new entity to the working memory graph."""
        with self._memory._lock:
            if vector.dtype != np.float32:
                vector = vector.astype(np.float32)
                
            node_id = self._memory._arena.allocate_id()
            self._memory._vectors.add_vector(node_id, vector)
            
            props = properties or {}
            self._nodes[node_id] = Node(id=node_id, label=label, type=type_, properties=props)
            return node_id

    def delete_node(self, node_id: int):
        """Deletes a node from memory."""
        with self._memory._lock:
            if node_id in self._nodes:
                del self._nodes[node_id]
            self._memory._arena.free_id(node_id)
            self._memory._hot_graph.remove_node(node_id)
            self._memory._edge_meta = [e for e in self._memory._edge_meta if e.source_id != node_id and e.target_id != node_id]

    def connect(self, source: int, target: int, relation: str, weight: float = 1.0, bidirectional: bool = False, reverse_relation: str = None):
        """Creates a directed, weighted edge between two nodes."""
        with self._memory._lock:
            rel_hash = hash(relation) & 0xFFFFFFFF
            self._memory._hot_graph.add_edge(source, target, weight, rel_hash)
            
            edge = Edge(source_id=source, target_id=target, relation=relation, weight=weight)
            if source in self._nodes:
                edge.source_label = self._nodes[source].label
            if target in self._nodes:
                edge.target_label = self._nodes[target].label
                
            self._memory._edge_meta.append(edge)

            if bidirectional:
                rev_rel = reverse_relation or f"REV_{relation}"
                rev_hash = hash(rev_rel) & 0xFFFFFFFF
                self._memory._hot_graph.add_edge(target, source, weight, rev_hash)
                rev_edge = Edge(source_id=target, target_id=source, relation=rev_rel, weight=weight)
                if target in self._nodes: rev_edge.source_label = self._nodes[target].label
                if source in self._nodes: rev_edge.target_label = self._nodes[source].label
                self._memory._edge_meta.append(rev_edge)


class Memory:
    """
    The primary entry point for the AxiomGraph engine.
    Manages the lifecycle of mutable Hot Memory and consolidated Cold Memory (CSR/Vector DB).
    """
    def __init__(self, path: str, dimensions: int, use_gpu: bool = True):
        self.path = path
        self.dimensions = dimensions
        
        # GPU capability check & graceful fallback
        self.use_gpu = use_gpu
        if self.use_gpu:
            try:
                self._index = ext.GPUIndex()
            except Exception as e:
                print(f"Warning: GPU requested but failed to initialize ({e}). Falling back to CPU SIMD Index.")
                self.use_gpu = False
                self._index = ext.CPUIndex()
        else:
            self._index = ext.CPUIndex()

        # C++ Engine Core Initialization
        self._arena = ext.MemoryArena()
        self._vectors = ext.VectorBuffer(dimensions)
        self._hot_graph = ext.HotGraph()
        self._csr_graph = ext.CSRGraph()
        
        self._lock = threading.RLock()
            
        # Metadata storage (Python-side for dynamic schemaless JSON)
        self._edge_meta = []
        
        self.hot = HotMemoryManager(self)
        
        # Attempt to load state if exists
        if os.path.exists(f"{self.path}.vectors"):
            self.load()

    @classmethod
    def create(cls, path: str, dimensions: int, use_gpu: bool = True) -> 'Memory':
        """Initializes a new database or loads an existing one from disk."""
        return cls(path, dimensions, use_gpu)
        
    def consolidate(self, keep_in_hot: bool = False):
        """
        The Hot-to-Cold transition trigger. 
        Flattens nodes and edges into CSR arrays and builds the Vector Search index.
        """
        with self._lock:
            self._csr_graph.build_from_hot(self._hot_graph)
            
            if self.use_gpu:
                config = ext.CagraConfig()
                self._index.build_cagra_index(self._vectors, config)
            else:
                self._index.build_index(self._vectors)
                
            if not keep_in_hot:
                self._hot_graph = ext.HotGraph()

    def query(self, vector: np.ndarray, top_k: int = 5, depth: int = 1, search_hot: bool = True, filter_dict: dict = None) -> GraphContext:
        """
        Performs a hybrid cognitive search using GraphRAG techniques.
        """
        with self._lock:
            if vector.dtype != np.float32:
                vector = vector.astype(np.float32)
                
            allowed_ids = []
            if filter_dict:
                for nid, node in self.hot._nodes.items():
                    match = True
                    for k, v in filter_dict.items():
                        if k == "type":
                            if node.type != v: match = False
                        elif node.properties.get(k) != v:
                            match = False
                    if match:
                        allowed_ids.append(nid)
                if not allowed_ids:
                    return GraphContext() # No matches
            else:
                allowed_ids = list(self.hot._nodes.keys())
                
            # 1. Semantic Vector Search
            if self.use_gpu:
                initial_ids = self._index.search(vector, top_k, allowed_ids)
            else:
                initial_ids = self._index.search(self._vectors, vector, top_k, allowed_ids)
                
            # 2. Graph Traversal (BFS)
            context_nodes = set(initial_ids)
            frontier = list(initial_ids)
            
            for _ in range(depth):
                next_frontier = []
                for node_id in frontier:
                    # Get cold memory neighbors
                    neighbors = self._csr_graph.get_neighbors(node_id)
                    # Get hot memory neighbors
                    if search_hot:
                        hot_edges = self._hot_graph.get_neighbors(node_id)
                        neighbors.extend([e.target for e in hot_edges])
                        
                    for nbr in neighbors:
                        if nbr not in context_nodes:
                            context_nodes.add(nbr)
                            next_frontier.append(nbr)
                frontier = next_frontier
                
            # 3. Reconstruct Context
            nodes = [self.hot._nodes[nid] for nid in context_nodes if nid in self.hot._nodes]
            edges = [e for e in self._edge_meta if e.source_id in context_nodes and e.target_id in context_nodes]
            
            return GraphContext(nodes=nodes, edges=edges)

    def save(self):
        """Flushes the current state of both Hot and Cold memory to disk atomically."""
        with self._lock:
            tmp_vec = f"{self.path}.vectors.tmp"
            tmp_hot = f"{self.path}.hot.tmp"
            tmp_csr = f"{self.path}.csr.tmp"
            tmp_idx = f"{self.path}.index.tmp"
            tmp_meta = f"{self.path}.meta.json.tmp"
            
            self._vectors.save(tmp_vec)
            self._hot_graph.save(tmp_hot)
            self._csr_graph.save(tmp_csr)
            self._index.save(tmp_idx)
            
            meta = {
                "nodes": {k: {"id": v.id, "label": v.label, "type": v.type, "properties": v.properties} 
                         for k, v in self.hot._nodes.items()},
                "edges": [{"source_id": e.source_id, "target_id": e.target_id, "relation": e.relation, "weight": e.weight} 
                         for e in self._edge_meta]
            }
            with open(tmp_meta, "w") as f:
                json.dump(meta, f)
                
            # Atomic replace
            os.replace(tmp_vec, f"{self.path}.vectors")
            os.replace(tmp_hot, f"{self.path}.hot")
            os.replace(tmp_csr, f"{self.path}.csr")
            os.replace(tmp_idx, f"{self.path}.index")
            os.replace(tmp_meta, f"{self.path}.meta.json")

    def load(self):
        """Loads memory state from disk."""
        with self._lock:
            self._vectors.load(f"{self.path}.vectors")
            self._hot_graph.load(f"{self.path}.hot")
            self._csr_graph.load(f"{self.path}.csr")
            try:
                self._index.load(f"{self.path}.index")
            except Exception:
                pass # Index might not exist yet
                
            if os.path.exists(f"{self.path}.meta.json"):
                with open(f"{self.path}.meta.json", "r") as f:
                    meta = json.load(f)
                    
                for k, v in meta["nodes"].items():
                    self.hot._nodes[int(k)] = Node(**v)
                    
                self._edge_meta = [Edge(**e) for e in meta["edges"]]
