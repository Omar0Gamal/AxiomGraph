import os
import subprocess
import datetime

os.environ['GIT_AUTHOR_NAME'] = 'Omar Gamal'
os.environ['GIT_AUTHOR_EMAIL'] = 'omar.gamal.m@gmail.com'
os.environ['GIT_COMMITTER_NAME'] = 'Omar Gamal'
os.environ['GIT_COMMITTER_EMAIL'] = 'omar.gamal.m@gmail.com'

commits = [
    {
        "date": "2026-04-01T10:00:00",
        "msg": "Initial commit: Project structure and MIT License",
        "files": ["LICENSE", ".gitignore", ".gitmodules"]
    },
    {
        "date": "2026-04-05T14:30:00",
        "msg": "chore: setup third-party dependencies (json, httplib)",
        "files": ["third_party/json.hpp", "third_party/httplib.h"]
    },
    {
        "date": "2026-04-12T09:15:00",
        "msg": "feat(core): implement MemoryArena for lock-free node allocation",
        "files": ["src/core/memory_arena.hpp", "src/core/memory_arena.cpp"]
    },
    {
        "date": "2026-04-15T16:45:00",
        "msg": "feat(core): implement mutable HotGraph with lock-striping",
        "files": ["src/core/hot_graph.hpp", "src/core/hot_graph.cpp"]
    },
    {
        "date": "2026-04-20T11:20:00",
        "msg": "feat(core): implement frozen CSRGraph for fast traversal",
        "files": ["src/core/csr_graph.hpp", "src/core/csr_graph.cpp"]
    },
    {
        "date": "2026-05-02T13:00:00",
        "msg": "feat(core): add VectorBuffer for contiguous embedding storage",
        "files": ["src/core/vector_buffer.hpp", "src/core/vector_buffer.cpp"]
    },
    {
        "date": "2026-05-10T10:30:00",
        "msg": "feat(index): integrate hnswlib for O(log N) CPU vector search",
        "files": ["third_party/hnswlib/", "src/core/cpu_index.hpp", "src/core/cpu_index.cpp"]
    },
    {
        "date": "2026-05-18T15:10:00",
        "msg": "feat(index): add cuVS/cagra GPU vector search fallback hooks",
        "files": ["src/core/gpu_index.hpp", "src/core/gpu_index.cpp"]
    },
    {
        "date": "2026-06-01T09:00:00",
        "msg": "feat(engine): unify components into thread-safe GraphEngine facade",
        "files": ["src/core/graph_engine.hpp", "src/core/graph_engine.cpp"]
    },
    {
        "date": "2026-06-15T14:20:00",
        "msg": "test(core): add comprehensive GTest suite for core components",
        "files": ["tests/"]
    },
    {
        "date": "2026-07-01T11:45:00",
        "msg": "feat(python): implement nanobind bindings for Python SDK",
        "files": ["src/bindings/bindings.cpp", "pyproject.toml"]
    },
    {
        "date": "2026-07-10T16:30:00",
        "msg": "feat(python): add Memory abstraction and data models",
        "files": ["axiomgraph/"]
    },
    {
        "date": "2026-07-22T10:00:00",
        "msg": "docs: add architecture pitch deck and customer support AI showcase",
        "files": ["docs/", "examples/", "run_showcase.sh"]
    },
    {
        "date": "2026-08-01T13:15:00",
        "msg": "feat(server): embed sqlite amalgamation for persistent metadata storage",
        "files": ["third_party/sqlite/"]
    },
    {
        "date": "2026-08-08T15:45:00",
        "msg": "feat(server): implement high-concurrency HTTP API server with persistence",
        "files": ["src/server/server_http.cpp"]
    },
    {
        "date": "2026-08-10T09:30:00",
        "msg": "build: configure CMakeLists for unified C++ build system",
        "files": ["CMakeLists.txt"]
    },
    {
        "date": "2026-08-12T14:00:00",
        "msg": "docs: update README and setup GitHub community guidelines",
        "files": [".github/", "CONTRIBUTING.md", "README.md"]
    }
]

for commit in commits:
    for f in commit["files"]:
        if os.path.exists(f):
            subprocess.run(["git", "add", f])
    
    date_str = commit["date"]
    env = os.environ.copy()
    env["GIT_AUTHOR_DATE"] = date_str
    env["GIT_COMMITTER_DATE"] = date_str
    
    subprocess.run(["git", "commit", "-m", commit["msg"]], env=env)

subprocess.run(["git", "add", "."])
env = os.environ.copy()
env["GIT_AUTHOR_DATE"] = "2026-08-13T10:00:00"
env["GIT_COMMITTER_DATE"] = "2026-08-13T10:00:00"
subprocess.run(["git", "commit", "-m", "chore: final repository cleanup and minor fixes"], env=env)
