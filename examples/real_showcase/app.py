import ollama
import numpy as np
import sys
import os

# Add parent directory to path so it can find axiomgraph if compiled locally
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))
from axiomgraph import Memory

EMBED_MODEL = "nomic-embed-text"
CHAT_MODEL = "phi3"

def embed_text(text: str) -> np.ndarray:
    """Uses Ollama to generate embeddings and normalizes them for fast dot-product search."""
    response = ollama.embeddings(model=EMBED_MODEL, prompt=text)
    vec = np.array(response['embedding'], dtype=np.float32)
    vec /= np.linalg.norm(vec)
    return vec

def build_store_knowledge(db):
    print("  -> Generating vectors and populating Store Knowledge Graph...")
    
    # 1. Policies
    refund_pol_id = db.hot.add_node(
        vector=embed_text("Refund and Return Policy: Items can be returned within 30 days of purchase if they are defective. A cracked screen on arrival is covered, but user damage is not."),
        label="Refund Policy", 
        type_="POLICY",
        properties={"days": 30, "requires_receipt": True}
    )
    
    # 2. Products & Categories
    electronics_id = db.hot.add_node(
        vector=embed_text("Electronics Category: Laptops, Phones, and Gadgets."),
        label="Electronics Category", 
        type_="CATEGORY"
    )
    
    laptop_id = db.hot.add_node(
        vector=embed_text("UltraBook X1: High performance laptop with OLED screen."),
        label="UltraBook X1", 
        type_="PRODUCT",
        properties={"price": 1299.99, "warranty": "1 year"}
    )
    
    # 3. Customer & Orders
    customer_id = db.hot.add_node(
        vector=embed_text("Alice Johnson, loyal customer since 2022."),
        label="Alice Johnson", 
        type_="CUSTOMER",
        properties={"email": "alice@example.com"}
    )
    
    order_id = db.hot.add_node(
        vector=embed_text("Order #99214: Delivered 3 days ago. Status: Completed."),
        label="Order #99214", 
        type_="ORDER",
        properties={"date": "2026-08-10", "status": "Delivered"}
    )
    
    # Wire the relationships
    db.hot.connect(laptop_id, electronics_id, "BELONGS_TO")
    db.hot.connect(electronics_id, refund_pol_id, "GOVERNED_BY")
    db.hot.connect(customer_id, order_id, "PLACED")
    db.hot.connect(order_id, laptop_id, "CONTAINS_ITEM")
    
    print("  -> Consolidating memory indices...")
    db.consolidate()

def main():
    print("==================================================")
    print("  AxiomGraph: Customer Support Agent Showcase")
    print("==================================================\n")
    
    db = Memory.create("./store_db", dimensions=768, use_gpu=False)
    
    print("[1] Initializing Store Database...")
    build_store_knowledge(db)
    
    print("\n[2] Customer Chat Initiated")
    customer_message = "Hi, I'm Alice. I bought the UltraBook X1 a few days ago (Order #99214) but the screen was cracked when I opened it. Can I get a refund?"
    print(f"👤 Customer (Alice): {customer_message}")
    
    print("\n[3] Executing GraphRAG (Semantic Search + Topology Traversal)...")
    query_vec = embed_text(customer_message)
    
    # We find the top 2 semantic matches (likely the order and laptop), then traverse 2 hops 
    # to naturally pull in the customer details and the refund policy connected to the category.
    context = db.query(query_vec, top_k=2, depth=2)
    
    context_str = "KNOWLEDGE GRAPH CONTEXT:\n"
    for node in context.nodes:
        context_str += f" - {node.label} ({node.type}): {node.properties}\n"
    context_str += "\nLOGICAL RELATIONSHIPS:\n"
    for edge in context.edges:
        context_str += f" - {edge.source_label} -> {edge.relation} -> {edge.target_label}\n"
        
    print("  -> Extracted Graph Context:")
    print("--------------------------------------------------")
    print(context_str.strip())
    print("--------------------------------------------------")
    
    prompt = f"""You are a helpful customer support agent for a retail store. 
Use the following knowledge graph context to answer the customer's query.
Be polite, address them by name if known, confirm their order details, and explain the policy clearly based on the context.

{context_str}

Customer Query: {customer_message}
Support Agent Response:"""

    print("\n[4] Generating Support Response (Phi-3)...\n")
    stream = ollama.chat(
        model=CHAT_MODEL,
        messages=[{'role': 'user', 'content': prompt}],
        stream=True
    )
    
    print("🤖 Support Agent: ", end="", flush=True)
    for chunk in stream:
        print(chunk['message']['content'], end="", flush=True)
    print("\n\nShowcase complete!")

if __name__ == "__main__":
    main()
