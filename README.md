# glue

## Use as a template

1. Change the `name` field in `project.json`. Use lowercase kebab-case, such as `my-game`.
2. Apply it from the repository root:

```sh
python3 tools/apply-project-name.py
```

This updates the vcpkg package, CMake target, include directory, include paths, README,
and C++ namespace. Hyphens become underscores in the namespace, so `my-game` uses
`namespace my_game`.

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
