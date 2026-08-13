import sys
from axiomgraph.memory import Memory

def generate_mock_embedding(seed, dims=768):
    """Generate a stable pseudo-random embedding for demonstration."""
    import math
    return [math.sin(seed * i) for i in range(dims)]

def main():
    print("==================================================")
    print("  AxiomGraph: Customer Support Agentic RAG")
    print("==================================================")
    print("\n[1] Initializing Database...")
    db = Memory(dimensions=768)

    # 1. Insert Graph Nodes
    print("  -> Populating Store Knowledge Graph...")
    
    refund_pol_id = db.hot.add_node(
        vector=generate_mock_embedding(1),
        label="Refund Policy",
        type="POLICY",
        properties={"days": 30, "requires_receipt": True}
    )

    electronics_id = db.hot.add_node(
        vector=generate_mock_embedding(2),
        label="Electronics Category",
        type="CATEGORY",
        properties={}
    )

    laptop_id = db.hot.add_node(
        vector=generate_mock_embedding(3),
        label="UltraBook X1",
        type="PRODUCT",
        properties={"price": 1299.99, "warranty": "1 year"}
    )

    # 2. Add Edges (Relationships)
    print("  -> Creating topological edges...")
    db.hot.connect(
        source=laptop_id, 
        target=electronics_id, 
        relation="BELONGS_TO", 
        weight=1.0, 
        bidirectional=True
    )
    
    db.hot.connect(
        source=electronics_id, 
        target=refund_pol_id, 
        relation="GOVERNED_BY", 
        weight=1.0, 
        bidirectional=True
    )

    # 3. Consolidate memory
    print("  -> Consolidating memory indices (Building HNSW and CSR)...")
    db.consolidate()

    print("\n[2] Customer Chat Initiated")
    print("👤 Customer: Hi, I bought the UltraBook X1. The screen was cracked. Can I get a refund?")

    print("\n[3] Executing GraphRAG (Semantic Search + Topology Traversal)...")
    # Simulate an embedding for the customer query
    query_vector = generate_mock_embedding(3) 
    
    results = db.query(
        vector=query_vector,
        top_k=1,
        depth=2,
        filter_dict={},
        search_hot=False
    )
    
    print("  -> Extracted Graph Context:")
    print("-" * 50)
    print("KNOWLEDGE GRAPH CONTEXT:")
    
    for n in results["nodes"]:
        print(f" - {n['label']} ({n['type']}): {n['properties']}")
        
    print("\nLOGICAL RELATIONSHIPS:")
    for e in results["edges"]:
        print(f" - {e['source_id']} -> {e['relation']} -> {e['target_id']}")
    print("-" * 50)
    
    print("\n[4] LLM Response Generation (Simulated)...")
    print("🤖 Agent: Yes, since you purchased the UltraBook X1, it falls under the Electronics Category, which is governed by our 30-day Refund Policy. Assuming you have your receipt, you are eligible for a full refund!")
    print("\n✅ Task Complete.\n")

if __name__ == "__main__":
    main()
