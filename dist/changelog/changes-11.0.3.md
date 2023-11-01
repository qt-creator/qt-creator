Qt Creator 11.0.3
=================

Qt Creator version 11.0.3 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v11.0.2..v11.0.3

Editing
-------

### C++

* Fixed a crash with constructs that look similar to a function with initializer
  ([QTCREATORBUG-29386](https://bugreports.qt.io/browse/QTCREATORBUG-29386))
* Fixed an issue with Clang headers
  ([QTCREATORBUG-29571](https://bugreports.qt.io/browse/QTCREATORBUG-29571))
* Fixed missing lightbulbs for Clangd refactoring actions
  ([QTCREATORBUG-29493](https://bugreports.qt.io/browse/QTCREATORBUG-29493))

### QML

* Fixed wrong M16 warnings
  ([QTCREATORBUG-28468](https://bugreports.qt.io/browse/QTCREATORBUG-28468))

### Language Server Protocol

* Fixed the loading of client settings
  ([QTCREATORBUG-29477](https://bugreports.qt.io/browse/QTCREATORBUG-29477))

Projects
--------

* Fixed that issue descriptions were cut off
  ([QTCREATORBUG-29458](https://bugreports.qt.io/browse/QTCREATORBUG-29458))
* Fixed an issue when running in terminal
  ([QTCREATORBUG-29503](https://bugreports.qt.io/browse/QTCREATORBUG-29503))

### CMake

* Fixed a crash when loading a project
  ([QTCREATORBUG-29587](https://bugreports.qt.io/browse/QTCREATORBUG-29587))
* Fixed that `Stage for installation` was enabled by default for iOS Simulator
  and Bare Metal configurations
  ([QTCREATORBUG-29293](https://bugreports.qt.io/browse/QTCREATORBUG-29293),
   [QTCREATORBUG-29475](https://bugreports.qt.io/browse/QTCREATORBUG-29475))
* Fixed adding and removing files from modified CMake files
  ([QTCREATORBUG-29550](https://bugreports.qt.io/browse/QTCREATORBUG-29550))

### qmake

* Fixed the ABI setting in the qmake build step
  ([QTCREATORBUG-29552](https://bugreports.qt.io/browse/QTCREATORBUG-29552))

### Python

* Fixed that `.pyw` files were missing from the target information

Debugging
---------

* Fixed the debugging in terminal
  ([QTCREATORBUG-29463](https://bugreports.qt.io/browse/QTCREATORBUG-29463),
   [QTCREATORBUG-29497](https://bugreports.qt.io/browse/QTCREATORBUG-29497),
   [QTCREATORBUG-29554](https://bugreports.qt.io/browse/QTCREATORBUG-29554))
* Improved the pretty printer of `std::string` for `libc++`
  ([QTCREATORBUG-29526](https://bugreports.qt.io/browse/QTCREATORBUG-29526))

Analyzer
--------

### CTF Visualizer

* Fixed a crash when loading invalid JSON

Terminal
--------

* Fixed the default environment variables
  ([QTCREATORBUG-29515](https://bugreports.qt.io/browse/QTCREATORBUG-29515))
* Fixed `gnome-terminal` and `xdg-terminal` for the external terminal
  ([QTCREATORBUG-29488](https://bugreports.qt.io/browse/QTCREATORBUG-29488))

Test Integration
----------------

### CTest

* Fixed the update of target information after a change in the kit
  ([QTCREATORBUG-29477](https://bugreports.qt.io/browse/QTCREATORBUG-29477))

Platforms
---------

### Remote Linux

* Fixed that SFTP was used (and failed) for deployment when the source is remote
  ([QTCREATORBUG-29524](https://bugreports.qt.io/browse/QTCREATORBUG-29524))
* Fixed deployment to the device root directory
  ([QTCREATORBUG-29597](https://bugreports.qt.io/browse/QTCREATORBUG-29597))

### Docker

* Fixed the registration of sysroots by the installer
  ([QTCREATORBUG-29523](https://bugreports.qt.io/browse/QTCREATORBUG-29523))

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Alexandre Laurent  
André Pönitz  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Marcus Tillmanns  
Miikka Heikkinen  
Semih Yavuz  
