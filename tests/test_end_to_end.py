import os
import numpy as np
from axiomgraph.memory import Memory

def main():
    print("Initializing memory...")
    db = Memory(path="./test_graph", dimensions=4, use_gpu=False)
    
    print("Adding nodes...")
    v1 = np.array([1.0, 0.0, 0.0, 0.0], dtype=np.float32)
    v2 = np.array([0.0, 1.0, 0.0, 0.0], dtype=np.float32)
    
    n1 = db.hot.add_node(vector=v1, label="Node 1", type_="TEST")
    n2 = db.hot.add_node(vector=v2, label="Node 2", type_="TEST")
    
    db.hot.connect(source=n1, target=n2, relation="TEST_REL")
    
    print("Consolidating...")
    db.consolidate()
    
    print("Querying...")
    results = db.query(
        vector=np.array([1.0, 0.1, 0.0, 0.0], dtype=np.float32),
        top_k=1,
        depth=1
    )
    
    print(f"Nodes found: {len(results.nodes)}")
    for n in results.nodes:
        print(f" - {n.label}")
        
    print(f"Edges found: {len(results.edges)}")
    
    print("Saving...")
    db.save()
    print("Done!")

if __name__ == "__main__":
    main()
