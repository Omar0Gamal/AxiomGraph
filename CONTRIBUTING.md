# Contributing to AxiomGraph

We welcome contributions to AxiomGraph. Please follow the guidelines below to ensure a smooth contribution process.

## Code of Conduct

By participating in this project, you are expected to uphold standard professional conduct. Be respectful and constructive in issues and pull requests.

## Development Setup

1. Fork the repository and clone it locally.
2. Install dependencies:
   * CMake (>= 3.24)
   * GCC or Clang (C++17 support required)
   * Python (>= 3.8) for SDK testing
3. Build the core project:
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```
4. Run tests:
   ```bash
   ctest --output-on-failure
   ```

## Pull Request Process

1. Create a new branch for your feature or bug fix.
2. Ensure your code adheres to the existing style.
3. Write GoogleTest unit tests for any new C++ functionality, or `pytest` tests for Python SDK changes.
4. Update the `README.md` if you are introducing a new user-facing feature.
5. Submit a pull request and fill out the provided template.

## Bug Reports and Feature Requests

Please use the provided GitHub Issue templates to file bugs and feature requests. Include as much detail as possible, such as logs and minimal reproducible examples.
