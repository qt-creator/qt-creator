Qt Creator 16
=============

Qt Creator version 16 contains bug fixes and new features.
It is a free upgrade for commercial license holders.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=15.0..v16.0.0>

Editing
-------

* Removed the confusing `Mixed` indentation setting
  ([QTCREATORBUG-11575](https://bugreports.qt.io/browse/QTCREATORBUG-11575),
   [QTCREATORBUG-11675](https://bugreports.qt.io/browse/QTCREATORBUG-11675),
   [QTCREATORBUG-19576](https://bugreports.qt.io/browse/QTCREATORBUG-19576),
   [QTCREATORBUG-25628](https://bugreports.qt.io/browse/QTCREATORBUG-25628))
* Added auto-detection for the indentation setting
  `Preferences > Text Editor > Behavior`
* Added a button for indentation settings to the editor toolbar
* Fixed that `File > Close` / `Ctrl + W` closed pinned files
  ([QTCREATORBUG-25964](https://bugreports.qt.io/browse/QTCREATORBUG-25964))
* Fixed the behavior of `Shift + Backspace` for multi-cursor editing
  ([QTCREATORBUG-32374](https://bugreports.qt.io/browse/QTCREATORBUG-32374))

### C++

* Changed
  `Preferences > C++ > Quick Fixes > Getter Setter Generation Properties`
  to use JavaScript expressions for transformation of the contents
  ([QTCREATORBUG-32302](https://bugreports.qt.io/browse/QTCREATORBUG-32302))
* Improved the dialog for `Create implementations for member functions`
  ([QTCREATORBUG-32193](https://bugreports.qt.io/browse/QTCREATORBUG-32193))
* Fixed a formatting issue when applying method signature changes
  ([QTCREATORBUG-31931](https://bugreports.qt.io/browse/QTCREATORBUG-31931))
* Fixed the generation of getters for local enum types
  ([QTCREATORBUG-32473](https://bugreports.qt.io/browse/QTCREATORBUG-32473))
* Fixed the header guard creation for file names with special characters
  ([QTCREATORBUG-32539](https://bugreports.qt.io/browse/QTCREATORBUG-32539))
* Built-in
    * Added support for init-statements in range-based `for` loops
      ([QTCREATORBUG-31961](https://bugreports.qt.io/browse/QTCREATORBUG-31961))
    * Added support for refactoring in the presence of concepts
      ([QTCREATORBUG-31214](https://bugreports.qt.io/browse/QTCREATORBUG-31214))

### QML

* qmlls
    * Added the value of the `QML_IMPORT_PATH` CMake variable to the imports
      passed to `qmlls`
    * Changed `Go to Definition` to be a non-advanced feature
      ([QTBUG-131920](https://bugreports.qt.io/browse/QTBUG-131920))
    * Changed the outline to be an advanced feature
      ([QTCREATORBUG-31767](https://bugreports.qt.io/browse/QTCREATORBUG-31767))
    * Fixed that the language server was not restarted after changing the Qt
      version in the kit
      ([QTCREATORBUG-32044](https://bugreports.qt.io/browse/QTCREATORBUG-32044))
    * Fixed that toolbars where created over and over again
      ([QTCREATORBUG-32356](https://bugreports.qt.io/browse/QTCREATORBUG-32356))

### Language Server Protocol

* Added support for `Diagnostic.CodeDescription`
  ([LSP Documentation](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#codeDescription))
* Added the option to enable or disable language servers per project
  ([QTCREATORBUG-31987](https://bugreports.qt.io/browse/QTCREATORBUG-31987))
* Fixed that information messages were shown in the same style as warnings in
  the editor
  ([QTCREATORBUG-31878](https://bugreports.qt.io/browse/QTCREATORBUG-31878),
   [QTCREATORBUG-32163](https://bugreports.qt.io/browse/QTCREATORBUG-32163))

### Copilot

* Fixed issues with newer versions of the language server
  ([QTCREATORBUG-32536](https://bugreports.qt.io/browse/QTCREATORBUG-32536))

### SCXML

* Fixed the colors of items
  ([QTCREATORBUG-32477](https://bugreports.qt.io/browse/QTCREATORBUG-32477))

Projects
--------

* Added `SDKs` settings category
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-master/creator-how-tos.html#sdks))
    * Added settings for the Windows App SDK
      ([QTBUG-124800](https://bugreports.qt.io/browse/QTBUG-124800))
    * Moved the Android and QNX SDK settings to the new category
* Added support for [LoongArch](https://en.wikipedia.org/wiki/Loongson#LoongArch)
  architecture
* Added an option for the run environment to the kit settings
  ([QTCREATORBUG-31906](https://bugreports.qt.io/browse/QTCREATORBUG-31906))
* Merged various related kit settings to the same row in
  `Preferences > Kits > Kits`
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-master/creator-preferences-kits.html))
* Made build configuration specific variables like `Device:HostAddress` and
  `Device:KeyFiles` available to deploy configurations
* Improved the target setup page in Projects mode
    * Made the `Configure` button always visible
      ([QTCREATORBUG-31314](https://bugreports.qt.io/browse/QTCREATORBUG-31314))
    * Made the initially selected kit visible without scrolling
* Fixed some inconsistencies between build device and run device settings
  ([QTCREATORBUG-32124](https://bugreports.qt.io/browse/QTCREATORBUG-32124))
* Fixed that the application output doesn't show the last line when filtering
  is active
  ([QTCREATORBUG-31959](https://bugreports.qt.io/browse/QTCREATORBUG-31959))
* Fixed that the project specific editor settings were not applied to the
  Markdown editor
  ([QTCREATORBUG-31875](https://bugreports.qt.io/browse/QTCREATORBUG-31875))
* Fixed the title of `Build for Run Configuration <name>`
  ([QTCREATORBUG-32350](https://bugreports.qt.io/browse/QTCREATORBUG-32350))
* Fixed a crash when an application outputs lots of lines
  ([QTCREATORBUG-32371](https://bugreports.qt.io/browse/QTCREATORBUG-32371))
* Environment Editor
    * Fixed the `Disable` button for the first item
      ([QTCREATORBUG-32495](https://bugreports.qt.io/browse/QTCREATORBUG-32495))
    * Fixed the `Edit` button for disabled items
      ([QTCREATORBUG-32495](https://bugreports.qt.io/browse/QTCREATORBUG-32495))
* Qt
    * Improved performance of Qt ABI detection when module `.json` files are
      available
      ([QTCREATORBUG-31943](https://bugreports.qt.io/browse/QTCREATORBUG-31943))
    * Improved performance of collecting Qt version information
    * Removed unnecessary warning when the Qt version in a kit does not
      have a QML utility
      ([QTCREATORBUG-32052](https://bugreports.qt.io/browse/QTCREATORBUG-32052))
* Qt Creator Plugin Wizard
    * Added support for building for Windows and Linux on ARM on GitHub
    * Added a run configuration that runs the Qt Creator that the plugin was
      built with

### CMake

* Simplified the project tree hierarchy for empty subdirectories
  ([QTCREATORBUG-32217](https://bugreports.qt.io/browse/QTCREATORBUG-32217))
* Added support for creating run configurations for custom CMake targets
  with the `qtc_runnable` `FOLDER` property
  ([QTCREATORBUG-32324](https://bugreports.qt.io/browse/QTCREATORBUG-32324))
* Improved the performance when CMake reply files change on disk
* Fixed that manually created run configurations could be removed if
  `Create suitable run configurations automatically` was turned off
  ([QTCREATORBUG-32289](https://bugreports.qt.io/browse/QTCREATORBUG-32289))
* Fixed issues with Objective-C/C++ files if `OBJCXX` is added to the
  list of languages in the project file
  ([QTCREATORBUG-32282](https://bugreports.qt.io/browse/QTCREATORBUG-32282))
* Fixed that Ninja was not detected even when `CMAKE_MAKE_PROGRAM` was set
  to the `ninja` executable
  ([QTCREATORBUG-32436](https://bugreports.qt.io/browse/QTCREATORBUG-32436))
* Fixed the import of multi-config CMake presets
  ([QTCREATORBUG-31554](https://bugreports.qt.io/browse/QTCREATORBUG-31554))
* Package Manager Auto Setup
    * Changed the default installation directory to `/tmp` to ensure that the
      directory is writable
      ([QTCREATORBUG-31570](https://bugreports.qt.io/browse/QTCREATORBUG-31570),
       [QTCREATORBUG-32430](https://bugreports.qt.io/browse/QTCREATORBUG-32430))

### Qmake

* Fixed that `.pri` files that are included but unused were not marked as
  inactive in the project tree
  ([QTCREATORBUG-32405](https://bugreports.qt.io/browse/QTCREATORBUG-32405))

### Meson

* Replaced explicit calls to `ninja` by their equivalent `meson` calls
  ([QTCREATORBUG-31407](https://bugreports.qt.io/browse/QTCREATORBUG-31407))
* Improved the layout of the project tree

Debugging
---------

* Changed that clicking disabled breakpoints enables them instead of removing
  them
* Fixed the movement of pinned debugger tooltips with extra editor windows
  ([QTCREATORBUG-24109](https://bugreports.qt.io/browse/QTCREATORBUG-24109))

### C++

* Pretty printers
    * Added `QMultiHash`
      ([QTCREATORBUG-32313](https://bugreports.qt.io/browse/QTCREATORBUG-32313))
    * Fixed issues with debuggers that use an older Python version
      ([QTCREATORBUG-32475](https://bugreports.qt.io/browse/QTCREATORBUG-32475))
* CDB
    * Disabled heap debugging by default and added the option
      `Enable heap debugging`
      ([QTCREATORBUG-32102](https://bugreports.qt.io/browse/QTCREATORBUG-32102))

Analyzer
--------

### Clang

* Fixed a crash when the tool is run in parallel
  ([QTCREATORBUG-32411](https://bugreports.qt.io/browse/QTCREATORBUG-32411))

### QML Profiler

* Fixed issues with restarting the profiler on remote Linux devices
  ([QTCREATORBUG-31372](https://bugreports.qt.io/browse/QTCREATORBUG-31372))
* Fixed that profiling could fail to start
  ([QTCREATORBUG-32062](https://bugreports.qt.io/browse/QTCREATORBUG-32062))
* Fixed the sorting of statistics
  ([QTCREATORBUG-32398](https://bugreports.qt.io/browse/QTCREATORBUG-32398))

### Axivion

* Added support for images in the issue details
* Moved Axivion preferences to `Preferences > Analyzer > Axivion`
* Fixed that the display of data in the issues table did not adapt to the
  column's data type
  ([QTCREATORBUG-32023](https://bugreports.qt.io/browse/QTCREATORBUG-32023))
* Fixed that filters were shown even for issue types that do not suppor them
* Fixed that the Filter menus opened at the wrong position
  ([QTCREATORBUG-32506](https://bugreports.qt.io/browse/QTCREATORBUG-32506))

### Coco

* Added support for configuring CMake and qmake projects for code coverage
  in `Projects > Project Settings > Coco Code Coverage`
  ([Documentation]https://doc-snapshots.qt.io/qtcreator-16.0/creator-coco.html)

Terminal
--------

* Fixed that the view didn't jump to the end on input
  ([QTCREATORBUG-32407](https://bugreports.qt.io/browse/QTCREATORBUG-32407))
* Fixed the title of tabs
  ([QTCREATORBUG-32197](https://bugreports.qt.io/browse/QTCREATORBUG-32197))
* Fixed killing the shell process
  ([QTCREATORBUG-32509](https://bugreports.qt.io/browse/QTCREATORBUG-32509))
* Fixed the scrolling behavior
  ([QTCREATORBUG-32167](https://bugreports.qt.io/browse/QTCREATORBUG-32167),
   [QTCREATORBUG-32546](https://bugreports.qt.io/browse/QTCREATORBUG-32546))
* Fixed the title of tabs
  ([QTCREATORBUG-32197](https://bugreports.qt.io/browse/QTCREATORBUG-32197))
* Fixed the handling of `Home` and `End` keys
  ([QTCREATORBUG-32545](https://bugreports.qt.io/browse/QTCREATORBUG-32545))

Version Control Systems
-----------------------

* Added automatic detection of files under version control even when the
  corresponding plugin is disabled
* Added a notification that suggests enabling a version control plugin when
  files under that version control are detected
* Disabled the `Bazaar`, `Fossil`, `Mercurial`, and `Subversion` support
  plugins by default

### Git

* Increased minimum Git version to 2.13.0
* Added `Create Branch From` to the context menu on commits in `Git Log`
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-16.0/creator-how-to-git-log.html))
* Added the option to show the log of the branch in the `Git Commit` and `Amend`
  views by clicking the branch name
* Added the action `Status (Including All Untracked)`, and changed the standard
  `Status` action to exclude untracked files
  ([QTCREATORBUG-32301](https://bugreports.qt.io/browse/QTCREATORBUG-32301))

Platforms
---------

### Windows

* Fixed that temporary files were created when saving files on the FAT file
  system and removed only at Qt Creator shutdown
  ([QTCREATORBUG-29942](https://bugreports.qt.io/browse/QTCREATORBUG-29942))

### Linux

* Added support for the `terminator` terminal emulator
  ([QTCREATORBUG-32111](https://bugreports.qt.io/browse/QTCREATORBUG-32111))

### macOS

* Fixed a crash when MinGW toolchains are detected on macOS hosts
  ([QTCREATORBUG-32127](https://bugreports.qt.io/browse/QTCREATORBUG-32127))

### Android

* Fixed a performance problem when detecting the Android ABI
  ([QTCREATORBUG-31068](https://bugreports.qt.io/browse/QTCREATORBUG-31068))
* Fixed that the wrong `lldb-server` could be used
  ([QTCREATORBUG-32494](https://bugreports.qt.io/browse/QTCREATORBUG-32494))

### iOS

* Added support for application output and C++ debugging for devices with iOS 17
  and later
* Fixed a crash when stopping applications on devices with iOS 16 and earlier
* Fixed QML profiling on devices with iOS 16 and earlier
  ([QTCREATORBUG-32403](https://bugreports.qt.io/browse/QTCREATORBUG-32403))
* Fixed that the development teams could not be determined with Xcode 16.2
  and later
  ([QTCREATORBUG-32447](https://bugreports.qt.io/browse/QTCREATORBUG-32447))

### Remote Linux

* Added support for `GoCmdBridge` for performance improvements

### Docker

* Fixed an issue with running `pkg-config` in the container
  ([QTCREATORBUG-32325](https://bugreports.qt.io/browse/QTCREATORBUG-32325))
* Fixed an issue with shutting down the device access
* Fixed soft asserts during container setup

### QNX

* Fixed issues with Clangd 19
  ([QTCREATORBUG-32529](https://bugreports.qt.io/browse/QTCREATORBUG-32529))

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Alexander Drozdov  
Alexander Pershin  
Alexandre Laurent  
Alexis Jeandet  
Ali Kianian  
Andre Hartmann  
André Pönitz  
Andrii Batyiev  
Andrii Semkiv  
Artur Twardy  
Brook Cronin  
Burak Hancerli  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Faure  
David Schulz  
Eike Ziller  
Esa Törmänen  
Henning Gruendl  
Jaime Resano  
Jaroslaw Kobus  
Johanna Vanhatapio  
Jörg Bornemann  
Kai Köhne  
Knud Dollereder  
Leena Miettinen  
Liu Zhangjian  
Lukasz Papierkowski  
Mahmoud Badri  
Marco Bubke  
Marcus Tillmanns  
Markus Redeker  
Masoud Jami  
Mats Honkamaa  
Miikka Heikkinen  
Mitch Curtis  
Morteza Jamshidi  
Nicholas Bennett  
Nikolaus Demmel  
Olivier De Cannière  
Orgad Shaneh  
Patryk Stachniak  
Pranta Dastider  
Przemyslaw Lewandowski  
Rafal Stawarski  
Ralf Habacker  
Robert Löhning  
Sami Shalayel  
Semih Yavuz  
Shrief Gabr  
Shyamnath Premnadh  
Tasuku Suzuki  
Teea Poldsam  
Thiago Macieira  
Thomas Hartmann  
Tim Jenßen  
Vikas Pachdha  
Ville Lavonius  
Xu Jin  
