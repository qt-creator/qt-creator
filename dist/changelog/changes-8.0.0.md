Qt Creator 8
============

Qt Creator version 8 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/7.0..v8.0.0

General
-------

* Moved `Tools > Options` to `Edit > Preferences`

Help
----

* Added support for mouse forward and backward buttons (QTCREATORBUG-25168)

Editing
-------

* Added shortcut for adding next search match to multi-selection
* Added warning when editing generated file (QTCREATORBUG-27173)
* Added option for hiding line ending information `Text Editor > Display >
  Display file line ending`
* Fixed updating of annotations (QTCREATORBUG-26812)
* Fixed that whitespace was not selected on double-click (QTCREATORBUG-24607)
* Fixed `Rewrap Paragraph` when indenting with tabs (QTCREATORBUG-27602)
* Fixed code folding marker visibility after `Open in New Window`
  (QTCREATORBUG-27748)

### C++

* Removed `libclang` based code model
* Fixed that `Generate Setter and Getter` generated non-static methods for
  static pointer types (QTCREATORBUG-27547)
* Fixed `Add Local Declaration` for class templates (QTCREATORBUG-26004)
* Fixed that `Follow Symbol Under Cursor` could jump to new location even after
  the user started doing other things (QTCREATORBUG-20878)
* Fixed that macros were highlighted as preprocessor statements
  (QTCREATORBUG-23548)
* Fixed initialization of pointer variables when generating member for
  `Q_PROPERTY` (QTCREATORBUG-27770)
* Clangd
    * Increased minimum `Clangd` version to 14
    * Added warning and disabling of `Clangd` by default on machines with low
      memory (QTCREATORBUG-19297)
    * Improved performance of `compile_commands.json` creation
    * Replaced some refactoring actions by the ones from `Clangd`
    * Added desugaring of types and warning about unused includes
    * Added option for ignoring big files
    * Added information on parent function to `Find References With Access Type`
      (QTCREATORBUG-27550)
    * Added warning icon and deprecation tag to completion items for deprecated
      API (QTCREATORBUG-2325)
    * Worked around `Clangd` highlighting issue (QTCREATORBUG-27601)
    * Fixed `Follow Symbol Under Cursor` for template types (QTCREATORBUG-27524)
    * Fixed that issues from other files could be shown in `Issues`
      (QTCREATORBUG-27260)
    * Fixed position of diagnostics lightbulb
    * Fixed function return types in `Outline` (QTCREATORBUG-27587)
    * Fixed that UI files with same name could confuse code model
      (QTCREATORBUG-27584)
* clang-format
    * Fixed cursor position when undoing formatting (QTCREATORBUG-27608)

### QML

* Added option for maximum line length to code style options
  (QTCREATORBUG-23411)
* Fixed handling of JavaScript string templates (QTCREATORBUG-21869)
* Fixed formatting issue with nullish coalescing operator (QTCREATORBUG-27344)
* Fixed that `Follow Symbol Under Cursor` could open file from build directory
  instead of source directory (QTCREATORBUG-27173)
* Fixed member lookup for items with same name in different modules
  (QTCREATORBUG-26714)

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
* Added support for dragging in `Outline` (QTCREATORBUG-27502)
* Fixed order of outline items (QTCREATORBUG-4346)
* Fixed semantic highlighting after server reset
* Fixed that semantic update was delayed by `Document update threshold` even
  after saving
* Fixed that tooltips could appear while Qt Creator is not in the foreground
* Fixed synchronization of outline view (QTCREATORBUG-27595)

### Image Viewer

* Added button for copying image as data URL

### FakeVim

* Partially implemented multi repeat command `:g, :v`

Projects
--------

* Added locator filter for starting run configurations
* Added `BuildSystem:Name` variable for default build directory
  (QTCREATORBUG-26147)

### CMake

* Added `Profile` build configuration type that is `RelWithDebInfo` with `QML
  debugging and profiling`
* Added `install` command to wizard generated projects
* Turned `QML debugging and profiling` option on by default for `Debug`
  configurations
* Removed hardcoded `QT_QML_DEBUG` from wizard created project files
* Fixed issue when reconfiguring with `QML debugging and profiling` option
  enabled
* Fixed missing path to `ninja` for `ExternalProject_Add` (QTCREATORBUG-27495)
* Fixed that headers were wrongly classified as `C` code if `qt_add_qml_module`
  is used (QTCREATORBUG-27117)

### Compilation Database

* Fixed wrong removal of command line options (QTCREATORBUG-22949)

Debugging
---------

* Switched fallback Qt version for debug information to Qt 6.2
* Added pretty printer for `QAnyStringView`
* Added workaround for LLDB on Ubuntu 22.04

Analyzer
--------

### Coco

* Added experimental `Coco` integration
* Added visualization of code coverage in code editor

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
* Fixed ABI detection of static Qt (QTCREATORBUG-27735)
* Fixed interrupting remote CDB processes (QTCREATORBUG-21657)

### macOS

* Fixed import of existing builds of CMake projects that were done on the
  command line (QTCREATORBUG-27591)

### Android

* Added option to connect physical device over WiFi
* Moved SDK manager to separate dialog
* Aligned platform names with Android Studio (QTCREATORBUG-27161)
* Fixed issues with newer SDK tools (QTCREATORBUG-27174)

### iOS

* Improved consecutive deployment speed (QTCREATORBUG-24371)

### Remote Linux

* Switched to `echo` for testing connection
* Fixed deployment of multiple directories with `rsync`

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
Aleksei German  
Alesandro Portale  
Alessandro Portale  
Alexander Akulich  
Alexander Drozdov  
Alexandru Croitor  
Andre Hartmann  
André Pönitz  
Artem Sokolovskii  
Assam Boudjelthia  
Bartlomiej Moskal  
BogDan Vatra  
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
Evgeny Shtanov  
Fawzi Mohamed  
Henning Gruendl  
Ihor Ivlev  
Ippei Sugita  
Jaroslaw Kobus  
Knud Dollereder  
Leena Miettinen  
Mahmoud Badri  
Marcus Tillmanns  
Mats Honkamaa  
Maximilian Goldstein  
Miikka Heikkinen  
Orgad Shaneh  
Piotr Mućko  
Rafael Roquetto  
Robert Löhning  
Samuel Ghinet  
Sergey Morozov  
Tapani Mattila  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Xavier Besson  
