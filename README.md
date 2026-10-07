# glue

<!-- template-setup:start -->
## Use as a template

Run the one-shot initializer with a lowercase kebab-case name:

```sh
python3 tools/apply-project-name.py my-game
```

This updates the vcpkg package, CMake target, include directory, include paths, README,
and C++ namespace. Hyphens become underscores in the namespace, so `my-game` uses
`namespace my_game`. After a successful rename, the initializer removes itself and leaves
`tools/.gitkeep` behind.
<!-- template-setup:end -->

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
