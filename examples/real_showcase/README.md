# AxiomGraph + Ollama GraphRAG Showcase

This example demonstrates a complete GraphRAG pipeline running locally on your machine.
It uses **Ollama** to host both the Embedding model and the Text Generation LLM, making it incredibly lightweight and perfect for machines with around **4GB of VRAM**.

## Prerequisites

1. Install [Ollama](https://ollama.com/) on your machine.
2. Start the Ollama server (usually runs in the background automatically, or via `ollama serve` on Linux).
3. Pull the required models (this will download ~2.5GB total, which fits easily in low VRAM):
   ```bash
   ollama pull nomic-embed-text  # Fast 768-dim embeddings
   ollama pull phi3              # Small, smart 3.8B parameter text model by Microsoft
   ```
4. Install Python dependencies:
   ```bash
   pip install -r requirements.txt
   ```
5. Ensure the AxiomGraph C++ extension is compiled via CMake in the parent directory so that `axiomgraph` can be imported.

## Running

Execute the application:
```bash
python app.py
```
