import unittest
import numpy as np
import os
import sys

# Ensure axiomgraph is in path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from axiomgraph import Memory

class TestAxiomGraphMemory(unittest.TestCase):
    def setUp(self):
        self.db_path = "./test_db"
        self.dims = 4
        self.db = Memory.create(self.db_path, dimensions=self.dims, use_gpu=False)

    def tearDown(self):
        # Cleanup files
        for ext in [".vectors", ".hot", ".csr", ".index", ".meta.json"]:
            path = f"{self.db_path}{ext}"
            if os.path.exists(path):
                os.remove(path)

    def test_node_addition_and_properties(self):
        vec = np.array([1.0, 0.0, 0.0, 0.0], dtype=np.float32)
        nid = self.db.hot.add_node(vec, "TestNode", "TYPE_A", {"key": "value"})
        
        self.assertIn(nid, self.db.hot._nodes)
        self.assertEqual(self.db.hot._nodes[nid].label, "TestNode")
        self.assertEqual(self.db.hot._nodes[nid].properties["key"], "value")

    def test_graph_traversal(self):
        # Exact vectors to force matches
        vec1 = np.array([1.0, 0.0, 0.0, 0.0], dtype=np.float32)
        vec2 = np.array([0.0, 1.0, 0.0, 0.0], dtype=np.float32)
        
        n1 = self.db.hot.add_node(vec1, "Node1", "A")
        n2 = self.db.hot.add_node(vec2, "Node2", "B")
        
        self.db.hot.connect(n1, n2, "RELATES_TO")
        self.db.consolidate()
        
        # Query matching vec1 perfectly, depth 1 should pull in n2
        context = self.db.query(vec1, top_k=1, depth=1)
        
        labels = [n.label for n in context.nodes]
        self.assertIn("Node1", labels)
        self.assertIn("Node2", labels)
        
        relations = [e.relation for e in context.edges]
        self.assertIn("RELATES_TO", relations)

    def test_persistence(self):
        vec = np.array([0.5, 0.5, 0.5, 0.5], dtype=np.float32)
        nid = self.db.hot.add_node(vec, "PersistNode", "P")
        self.db.consolidate()
        self.db.save()
        
        # Load into new instance
        db2 = Memory.create(self.db_path, dimensions=self.dims, use_gpu=False)
        self.assertIn(nid, db2.hot._nodes)
        self.assertEqual(db2.hot._nodes[nid].label, "PersistNode")

if __name__ == '__main__':
    unittest.main()
