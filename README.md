# QThread

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)  [![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)  

**Header-only C++ thread pool with a safe task queue and future-based results. **

Qthread combines a **queue** + **thread pool** in a simple, portable, single-header library.
It makes parallel execution easy: submit lambdas, functors, or functions, and retrieve results via `std::future`.

---

## Features
- **Header-only** - just drop `qthread.hpp` into your project.
- **Thread pool** - manage a fixed number of worker threads.
- **Safe Task queue** - tasks are stored and processed in FIFO order.
- **Future-based results** - submit tasks and get back  `std::future<T>`.
- **No dependencies** - pure C++ standard library.

---

## Instalation
Just copy the `include/qthread.hpp` into your codebase.

## Usage Example
```cpp
#include "qthread.hpp"
#include <iostream>

int main() {
    qthread::ThreadPool pool(4);

    auto f1 = pool.submit([] { return 42; });
    auto f2 = pool.submit([](int a, int b) { return a + b; }, 10, 32);

    std::cout << "f1 = " << f1.get() << "\n";
    std::cout << "f2 = " << f2.get() << "\n";

    pool.wait_for_completion();
}
```

## Building and Testing
```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Roadmap
Planned features for future versions:

- [ ] Task priorities.
- [ ] Task Cancellation
- [ ] Dynamic thread pool resizing
- [ ] Work-stealing scheduler
- [ ] Timeout and deadline support
- [ ] Thread affinity / CPU pinnig
- [ ] metrics and instrumentation

## Contributing
Contributions are Welcome!

- Fork the repo and create a new branch
- Add your changes with tests if possible
- Ensure formatting passes `clang-format`
- Submit a PR
