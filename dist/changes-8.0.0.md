Qt Creator 8
============

Qt Creator version 8 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/7.0..v8.0.0

Editing
-------

* Added shortcut for adding next search match to multi-selection
* Added warning when editing generated file (QTCREATORBUG-27173)
* Fixed updating of annotations (QTCREATORBUG-26812)
* Fixed that whitespace was not selected on double-click (QTCREATORBUG-24607)

### C++

* Removed `libclang` based code model
* Clangd
    * Increased minimum `Clangd` version to 14
    * Improved performance of `compile_commands.json` creation
    * Replaced some refactoring actions by the ones from `Clangd`
    * Added desugaring of types and warning about unused includes
    * Added option for ignoring big files
    * Added information on parent function to `Find References With Access Type`
      (QTCREATORBUG-27550)
    * Worked around `Clangd` highlighting issue (QTCREATORBUG-27601)
    * Fixed `Follow Symbol Under Cursor` for template types (QTCREATORBUG-27524)
    * Fixed that issues from other files could be shown in `Issues`
      (QTCREATORBUG-27260)
    * Fixed position of diagnostics lightbulb
    * Fixed function return types in `Outline` (QTCREATORBUG-27587)
    * Fixed that UI files with same name could confuse code model
      (QTCREATORBUG-27584)
* clang-format
    * Simplified options dialog

### QML

* Added option for maximum line length to code style options
  (QTCREATORBUG-23411)
* Fixed handling of JavaScript string templates (QTCREATORBUG-21869)
* Fixed formatting issue with nullish coalescing operator (QTCREATORBUG-27344)

### Python

* Switched to `python-lsp-server` by default (QTCREATORBUG-26230)
* Added configuration options for `python-lsp-server`
* Added check and installation help for `PySide` (PYSIDE-1742)
* Added clean up of outdated interpreters
* Added UIC based project wizard
* Improved code style in wizard generated files
* Fixed that unsaved and uncompiled UI files where outdated in code model
* Fixed performance issues (QTCREATORBUG-24140, QTCREATORBUG-24704)

### Language Server Protocol

* Improved performance for large server responses
* Fixed semantic highlighting after server reset
* Fixed that semantic update was delayed by `Document update threshold` even
  after saving

### Image Viewer

* Added button for copying image as data URL

Projects
--------

* Added locator filter for starting run configurations

### CMake

* Added `Profile` build configuration type that is `RelWithDebInfo` with `QML
  debugging and profiling`
* Turned `QML debugging and profiling` option on by default for `Debug`
  configurations
* Removed hardcoded `QT_QML_DEBUG` from wizard created project files
* Fixed issue when reconfiguring with `QML debugging and profiling` option
  enabled

Debugging
---------

* Switched fallback Qt version for debug information to Qt 6.2
* Added pretty printer for `QAnyStringView`

Analyzer
--------

### Coco

* Added experimental `Coco` integration

### CppCheck

* Added `Copy to Clipboard` to text marks (QTCREATORBUG-27092)
* Fixed quoting of command line arguments (QTCREATORBUG-27284)

Version Control Systems
-----------------------

* Changed output pane to use text editor font (QTCREATORBUG-27164)

### Git

* Fixed that fetching tags when showing changes blocked UI

### Gerrit

* Fixed that non-Gerrit remote could be selected in `Push to Gerrit`

### GitLab

* Added experimental `GitLab` integration
* Added support for browsing and cloning projects

Platforms
---------

### Windows

* Removed support for Universal Windows Platform (UWP)
* Added auto-detection for MSVC ARM toolchain and debugger
* Fixed ABI detection on ARM Windows

### Android

* Added option to connect physical device over WiFi
* Moved SDK manager to separate dialog
* Aligned platform names with Android Studio (QTCREATORBUG-27161)
* Fixed issues with newer SDK tools (QTCREATORBUG-27174)

### Remote Linux

* Switched to `echo` for testing connection

### Docker

* Added default mount point set to projects directory
* Fixed state detection of docker daemon
* Fixed issues with kit initialization

### MCU

* Added support for QUL 2.0 and removed support for earlier versions

### QNX

* Fixed debugger detection

Credits for these changes go to:
--------------------------------
Aaron Barany  
Adam Treat  
Alesandro Portale  
Alessandro Portale  
Alexander Drozdov  
Alexandru Croitor  
Andre Hartmann  
André Pönitz  
Artem Sokolovskii  
Assam Boudjelthia  
Christiaan Janssen  
Christian Kandeler  
Christian Stenger  
Christian Strømme  
Cristian Adam  
Cristián Maureira-Fredes  
David Schulz  
Dmitry Shachnev  
Eike Ziller  
Erik Verbruggen  
Fawzi Mohamed  
Henning Gruendl  
Ihor Ivlev  
Jaroslaw Kobus  
Knud Dollereder  
Leena Miettinen  
Marcus Tillmanns  
Maximilian Goldstein  
Miikka Heikkinen  
Orgad Shaneh  
Piotr Mućko  
Rafael Roquetto  
Robert Löhning  
Sergey Morozov  
Tapani Mattila  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Xavier Besson  
