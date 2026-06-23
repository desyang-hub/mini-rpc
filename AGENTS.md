# AGENTS.md

C++17 RPC framework. CMake build, muduo for networking, nacos for service discovery.

## Build Commands

```bash
make dev          # debug build with tests + examples
make release      # release build, no tests
make test         # build without examples + run ctest
make clean        # rm build/
```

Under the hood these call `scripts/build.sh {dev|release|test}` then `scripts/test.sh`.

Manual cmake:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

## Running Tests

```bash
cd build && ctest --output-on-failure -j$(nproc)
```

Test executables: `test_common`, `test_protocol`, `test_net`, `test_security`. Note: `test_core` is commented out in `tests/CMakeLists.txt:80` — not registered as a ctest. `test_server` exists but also not registered as ctest (no `add_test` for it).

## Architecture

Four library layers (bottom → top):
- `common` — logging, config (toml++), utilities
- `protocol` — header-only, message encoding/decoding (magic `0x5250`, CRC32)
- `net` — TCP server/client, epoll ET, connection pool (links muduo)
- `core` — RPC framework glue, Nacos service registry

Source mirrors include layout: `src/minirpc/{common,core,net,protocol}` ↔ `include/minirpc/{common,core,net,protocol}`.

## Dependencies

All fetched via FetchContent (no vcpkg/conan): nlohmann/json v3.11.2, muduo v2.0.2, nacos-sdk-cpp v1.1.3, googletest v1.14.0.

System deps: `cmake`, `g++`, `libcurl4-openssl-dev`, `protobuf-compiler`, `libprotobuf-dev`, `libprotoc-dev`.

## Key Conventions

- C++17 standard required
- Wire protocol: 2-byte magic (`0x5250`) + CRC32 checksum, max body 64MB
- Service binding via macros: `RPC_SERVICE_BIND`, `RPC_SERVICE_REGISTER`
- Version managed in `cmake/version.h.in` → generated to `build/include/minirpc/version.h`
- CI runs on `ubuntu-latest`, push to `dev`/`refactor/v3.1`, PRs to `main`
