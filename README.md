# serious-io-library
For serious programmers.

# st

Stands for single-threaded. An experiment in creating a single-threaded versions of following concepts in C++:
- `future` - from serious-execution-library
- `coroutine` - from serious-execution-library
- `event-queue`/`epoll` - moved here from serious-execution-library

Fully working single-threaded TCP echo-server on epoll is [here](examples/src/00_epoll_st.cpp).

# EXAMPLES

## BUILD

```bash
cmake -DCMAKE_BUILD_TYPE=Release -S . -B ./build
cmake --build ./build --parallel "$(nproc)" --target serious-io-library_00_epoll_st
```

## RUN

```bash
./build/examples/serious-io-library_00_epoll_st 8080 10
```
