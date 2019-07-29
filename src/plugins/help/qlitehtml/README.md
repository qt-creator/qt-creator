# Qt backend for litehtml

Provides

* A QPainter based rendering backend for the light-weight HTML/CSS rendering engine [litehtml].
* A QWidget that uses the QPainter based backend and provides API for simply setting the HTML text
  and a base URL plus hook that are used for requesting referenced resources.

## How to build

Build and install [litehtml]. It is recommended to build [litehtml] in release mode

```
cd litehtml
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX="$PWD/../install" -DCMAKE_BUILD_TYPE=Release -G Ninja ..
cmake --build .
cmake --install .
```

Add the [litehtml] installation path to the `CMAKE_PREFIX_PATH` when building the Qt backend

[litehtml]: https://github.com/litehtml/litehtml
