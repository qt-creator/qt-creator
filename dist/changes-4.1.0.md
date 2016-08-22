Qt Creator version 4.1 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.0..v4.1.0

General

* Added `Flat Dark` and `Flat Light` themes
* Added `Ctrl+N` and `Ctrl+P` shortcuts for navigating in locator
* Added experimental support for `Nim` programming language

Editing

* Added automatic release of resources for unmodified documents that have
  not been visible recently (QTCREATORBUG-10016)
* Added separate options for automatically inserted characters
* Added highlighting of automatically inserted characters
* Added option for skipping automatically inserted character, and changed
  it to be done only if the cursor stays before the character
* Added `Modnokai Night Shift v2`, `Qt Creator Dark`,
  `Solarized Dark` and `Solarized Light` editor schemes
* Fixed that replacing could change selection (QTCREATORBUG-15623)
* Fixed opening bookmarks in external editor window (QTCREATORBUG-16524)

Help

* Fixed default fallback font on Linux
* Fixed crash when removing multiple documentation sets (QTCREATORBUG-16747)

All Projects

* Improved feedback when building results in errors, because of issues with
  kits (QTCREATORBUG-16079)
* Fixed issue with building in paths with non-ASCII characters
  (QTCREATORBUG-15855)
* Fixed that `%{buildDir}` and `%{sourceDir}` stopped working in
  run configurations (QTCREATORBUG-16338)
* Fixed that `CurrentProject:` variables were sometimes not resolved from the
  appropriate project (QTCREATORBUG-16724)

QMake Projects

* Added `Duplicate File` to context menu in project tree (QTCREATORBUG-15952)
* Added `QtWebEngine` to modules list
* Changed `Run Qmake` from `qmake -r` to `qmake && make qmake_all` for Qt 5
* Fixed renaming files used in QRC files (QTCREATORBUG-15786)

CMake Projects

* Improved parsing of errors
* Added workaround for CMake issue that include paths are in random order
  (QTCREATORBUG-16432)
* Added option for disabling automatic running of CMake to `Build & Run` >
  `CMake` (QTCREATORBUG-15934)
* Fixed that CMake was automatically run even if Qt Creator application
  is not in foreground (QTCREATORBUG-16354)
* QML_IMPORT_PATH can now be set in CMakeLists.txt files. This information
  will be passed on to QmlJS code model (QTCREATORBUG-11328)
  Example CMakeLists.txt code:  
  `set(QML_IMPORT_PATH ${CMAKE_SOURCE_DIR}/qml ${CMAKE_BINARY_DIR}/imports CACHE string "" FORCE)`
* Fixed crash when re-opening project
* Fixed that `CMakeLists.txt` file was not shown for projects with errors

Qbs Projects

* Added support for `qtcRunnable` property, similar to `qtc_runnable` for
  Qmake projects

C++ Support

* Added separate highlighting for function declarations and usages
  (QTCREATORBUG-15564)
* Added highlighting option for global variables
* Added coding style option for preferring getters with `get`
* Fixed parsing of `-std=gnu++XX` option (QTCREATORBUG-16290)
* Fixed refactoring of methods with ref-qualifier

QML Support

* Added formal parameters of JavaScript functions to outline and locator

Debugging

* Added support for copying selected values from `Locals and Expressions`
  (QTCREATORBUG-14956)
* Fixed jumping to address in binary editor (QTCREATORBUG-11064)
* Fixed environment for `Start and Debug External Application`
  (QTCREATORBUG-16746)
* GDB
    * Fixed that `qint8` values where shown as unsigned values
      (QTCREATORBUG-16353)

QML Profiler

* Improved progress information
* Improved performance when many events are involved

Clang Static Analyzer

* Fixed that built-in headers were not found

Qt Quick Designer

* Added support for Qt Quick Controls 2 styles
* Added `Move to Component` action
* Added `Add New Signal Handler` action
* Added support of Qt Creator themes in the Designer UI
* Improved performance
* Improved error dialog (QTCREATORBUG-15772)
* Fixed crumble bar for component navigation
* Fixed that `Connections` was not allowed in `.ui.qml` files
* Fixed crashes with spaces in properties (QTCREATORBUG-16059)
* Fixed that child items of `State` were rendered (QTCREATORBUG-13003)

Version Control Systems

* Git
    * Added date and time information to branch dialog
    * Added support for running `git blame` only on selected lines
      (QTCREATORBUG-16055)
    * Fixed that branch dialog suggested existing branch name for new branch
      (QTCREATORBUG-16264)
* SVN
    * Added conflicted files to file list in submit editor

Test Integration

* Fixed that test case summary stayed visible even if no entries matched
  the applied filter
* Fixed parsing of failure location for Google Test on Windows

FakeVim

* Added expansion of `~` in file names (QTCREATORBUG-11160)

Model Editor

* Added zooming of diagrams

Beautifier

* Added option to automatically format files on save
* Uncrustify
    * Fixed issues with non-C++ files (QTCREATORBUG-15575)

Platform Specific

Windows

* Added detection of MSVC amd64_x86 toolchain

macOS

* Fixed include search order with frameworks (QTCREATORBUG-11599)

Remote Linux

* Added support for TCP/IP forward tunneling with SSH

iOS

* Added human readable error messages (QTCREATORBUG-16328)
* Fixed that deployment could fail if device and host are in same WiFi network
  (QTCREATORBUG-16061)

Credits for these changes go to:  
Albert Astals Cid  
Alessandro Portale  
Alexander Drozdov  
Alexandru Croitor  
Andre Hartmann  
André Pönitz  
Antoine Poliakov  
Anton Kudryavtsev  
Arnold Dumas  
BogDan Vatra  
Brett Stottlemyer  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Dmitry Ashkadov  
Eike Ziller  
Erik Verbrüggen  
Filippo Cucchetto  
Finn Brudal  
Georger Araújo  
Jake Petroules  
Jaroslaw Kobus  
Jean Gressmann  
Jochen Becher  
Lars Knoll  
Leena Miettinen  
Lorenz Haas  
Lukas Holecek  
Marc Mutz  
Marc Reilly  
Marco Benelli  
Marco Bubke  
Mashrab Kuvatov  
Mat Sutcliffe  
Maurice Kalinowski  
Nazar Gerasymchuk  
Nikita Baryshnikov  
Nikolai Kosjar  
Orgad Shaneh  
Oswald Buddenhagen  
Philip Lorenz  
Robert Löhning  
Serhii Moroz  
Shinnok  
Takumi ASAKI  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Tobias Hunger  
Ulf Hermann  
Unai IRIGOYEN  
Victor Heng  
Vikas Pachdha  
Vlad Seryakov  
Wolfgang Bremer  
