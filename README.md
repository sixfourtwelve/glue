# glue

## Clone

Clone the repository with its `rt-system` submodule:

```sh
git clone --recurse-submodules git@github.com:sixfourtwelve/glue.git
cd glue
```

For an existing checkout:

```sh
git submodule update --init --recursive
```

## Build

Requires CMake, Ninja, and vcpkg with `VCPKG_ROOT` set.

```sh
cmake --preset debug
cmake --build --preset debug
./build/debug/glue
```
