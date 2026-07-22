#!/bin/bash
set -e

echo "=========================================="
echo " Setting up AxiomGraph Environment "
echo "=========================================="

echo "[1] Creating Virtual Environment..."
python3 -m venv venv
source venv/bin/activate

echo "[2] Installing NVIDIA libraries (cuVS/RAFT) and Build Tools..."
pip install --extra-index-url=https://pypi.nvidia.com cuvs-cu12 pylibraft-cu12 scikit-build-core nanobind numpy ollama

echo "[3] Building and Installing AxiomGraph locally..."
# This seamlessly runs CMake, builds the nanobind C++ extension, and installs the python package
pip install -e .

echo "[4] Running the AI Customer Support Showcase..."
cd examples/real_showcase
python app.py
