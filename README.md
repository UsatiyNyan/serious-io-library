# serious-io-library
For serious programmers.

# EXAMPLES

Fully working concurrent TCP echo-server on epoll is [here](examples/src/00_epoll.cpp).

## BUILD

```bash
cmake -DCMAKE_BUILD_TYPE=Release -S . -B ./build
cmake --build ./build --parallel "$(nproc)" --target serious-io-library_00_epoll
```

## RUN

Usage: serious-io-library_00_epoll PORT MAX_CLIENTS

```bash
./build/examples/serious-io-library_00_epoll 8080 10
```
