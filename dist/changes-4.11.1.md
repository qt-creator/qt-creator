# Qt Creator 4.11.1

Qt Creator version 4.11.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.11.0..v4.11.1

## Editing

* Fixed `Visualize Whitespace` for editors without specialized highlighter definition
  (QTCREATORBUG-23040)

### C++

* Fixed wrong warnings about C++98 incompatibility with MSVC (QTCREATORBUG-23118)
* Fixed accidentally added internal include paths from GCC (QTCREATORBUG-23330)

### FakeVim

* Fixed goto next and previous split (QTCREATORBUG-22397)
* Fixed indentation of continuation lines (QTCREATORBUG-20876)

## Projects

* Fixed crash when closing application output

### CMake

* Fixed subdirectory structure in project tree (QTCREATORBUG-23372)

## Qt Quick Designer

* Fixed removing single signals from Connection (QDS-1333)

## Test Integration

* Fixed stopping tests when debugging (QTCREATORBUG-23298)

## Platforms

### Windows

* Worked around issue with HiDPI in Qt (QTBUG-80934)

### WebAssembly

* Fixed missing device in kit (QTCREATORBUG-23360)

### QNX

* Fixed deployment of Qt examples (QTCREATORBUG-22592)

## Credits for these changes go to:
