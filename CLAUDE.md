# Qt Creator

This project maintains two parallel build system descriptions: CMake (`CMakeLists.txt`) and qbs (`.qbs` files). They must be kept in sync.

## Build system sync rule

Whenever you modify a `CMakeLists.txt` file, also update the corresponding `.qbs` file in the same directory (and vice versa). The two files describe the same targets, sources, and dependencies — changes to one must be reflected in the other.
