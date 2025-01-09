Qt Creator 16
=============

Qt Creator version 16 contains bug fixes and new features.

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

### C++

* Changed
  `Preferences > C++ > Quick Fixes > Getter Setter Generation Properties`
  to use JavaScript expressions for transformation of the contents
  ([QTCREATORBUG-32302](https://bugreports.qt.io/browse/QTCREATORBUG-32302))
* Improved the dialog for `Create implementations for member functions`
  ([QTCREATORBUG-32193](https://bugreports.qt.io/browse/QTCREATORBUG-32193))
* Fixed a formatting issue when applying method signature changes
  ([QTCREATORBUG-31931](https://bugreports.qt.io/browse/QTCREATORBUG-31931))
* Built-in
    * Added support for init-statements in range-based `for` loops
      ([QTCREATORBUG-31961](https://bugreports.qt.io/browse/QTCREATORBUG-31961))
    * Added support for refactoring in the presence of concepts
      ([QTCREATORBUG-31214](https://bugreports.qt.io/browse/QTCREATORBUG-31214))

### QML

* qmlls
    * Added the value of the `QML_IMPORT_PATH` CMake variable to the imports
      passed to `qmlls`
    * Fixed that the language server was not restarted after changing the Qt
      version in the kit
      ([QTCREATORBUG-32044](https://bugreports.qt.io/browse/QTCREATORBUG-32044))

### Language Server Protocol

* Added support for `Diagnostic.CodeDescription`
  ([LSP Documentation](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#codeDescription))
* Added the option to enable or disable language servers per project
  ([QTCREATORBUG-31987](https://bugreports.qt.io/browse/QTCREATORBUG-31987))
* Fixed that information messages were shown in the same style as warnings in
  the editor
  ([QTCREATORBUG-31878](https://bugreports.qt.io/browse/QTCREATORBUG-31878),
   [QTCREATORBUG-32163](https://bugreports.qt.io/browse/QTCREATORBUG-32163))

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
* Qt
    * Improved performance of Qt ABI detection when module `.json` files are
      available
      ([QTCREATORBUG-31943](https://bugreports.qt.io/browse/QTCREATORBUG-31943))
    * Improved performance of collecting Qt version information
    * Removed unnecessary warning when the Qt version in a kit does not
      have a QML utility
      ([QTCREATORBUG-32052](https://bugreports.qt.io/browse/QTCREATORBUG-32052))

### CMake

* Simplified the project tree hierarchy for empty subdirectories
  ([QTCREATORBUG-32217](https://bugreports.qt.io/browse/QTCREATORBUG-32217))
* Fixed that manually created run configurations could be removed if
  `Create suitable run configurations automatically` was turned off
  ([QTCREATORBUG-32289](https://bugreports.qt.io/browse/QTCREATORBUG-32289))

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
* CDB
    * Disabled heap debugging by default and added the option
      `Enable heap debugging`
      ([QTCREATORBUG-32102](https://bugreports.qt.io/browse/QTCREATORBUG-32102))

Analyzer
--------

### QML Profiler

* Fixed issues with restarting the profiler on remote Linux devices
  ([QTCREATORBUG-31372](https://bugreports.qt.io/browse/QTCREATORBUG-31372))
* Fixed that profiling could fail to start
  ([QTCREATORBUG-32062](https://bugreports.qt.io/browse/QTCREATORBUG-32062))

### Axivion

* Added support for images in the issue details

### Coco

* Added support for configuring CMake and qmake projects for code coverage

Version Control Systems
-----------------------

* Added automatic detection of files under version control even when the
  corresponding plugin is disabled
* Added a notification that suggests enabling a version control plugin when
  files under that version control are detected
* Disabled the `Bazaar`, `Fossil`, `Mercurial`, and `Subversion` support
  plugins by default

### Git

* Added `Create Branch From` to the context menu on commits
* Added the option to show the log of the branch in the submit editor by
  clicking on it
* Added the action `Status (Including All Untracked)`, and changed the standard
  `Status` action to exclude untracked files
  ([QTCREATORBUG-32301](https://bugreports.qt.io/browse/QTCREATORBUG-32301))

Platforms
---------

### Windows

* Fixed that temporary files were created when saving files on the FAT file
  system and removed only at Qt Creator shutdown
  ([QTCREATORBUG-29942](https://bugreports.qt.io/browse/QTCREATORBUG-29942))

### Android

* Fixed a performance problem when detecting the Android ABI
  ([QTCREATORBUG-31068](https://bugreports.qt.io/browse/QTCREATORBUG-31068))

### iOS

* Added support for application output and C++ debugging for devices with iOS 17
  and later
* Fixed a crash when stopping applications on devices with iOS 16 and earlier

### Remote Linux

* Added support for `GoCmdBridge` for performance improvements

### Docker

* Fixed an issue with running `pkg-config` in the container
  ([QTCREATORBUG-32325](https://bugreports.qt.io/browse/QTCREATORBUG-32325))

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Alexander Drozdov  
Alexander Pershin  
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
Mahmoud Badri  
Marco Bubke  
Marcus Tillmanns  
Markus Redeker  
Masoud Jami  
Mats Honkamaa  
Miikka Heikkinen  
Mitch Curtis  
Morteza Jamshidi  
Nikolaus Demmel  
Olivier De Cannière  
Orgad Shaneh  
Patryk Stachniak  
Pranta Dastider  
Przemyslaw Lewandowski  
Rafal Stawarski  
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
Xu Jin  
