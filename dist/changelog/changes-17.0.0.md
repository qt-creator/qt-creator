Qt Creator 17
=============

Qt Creator version 17 contains bug fixes and new features.
It is a free upgrade for commercial license holders.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=16.0..v17.0.0>

General
-------

* Made the "2024" theme variants the default in
  `Preferences > Environment > Interface`
  ([QTCREATORBUG-32400](https://bugreports.qt.io/browse/QTCREATORBUG-32400))
* Updated icons
* Improved support for extracting archives
  ([QTAIASSIST-169](https://bugreports.qt.io/browse/QTAIASSIST-169))
* Added a `Courses` tab to `Welcome` mode
* Added tab completion to the locator
* Extensions
    * Moved the default extension registry to
      https://github.com/qt-creator/extension-registry
      (submissions not open to the public yet)
    * Added the option to configure multiple extension registries in
      `Preferences > Extensions > Browser`
    * Added the dependencies and supported platforms of extensions that are not
      installed to their details
    * Added version selectors for extensions that are not installed
    * Added support for dropping extension archives onto `Extensions` mode
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-17.0/creator-how-to-install-extensions.html))

Editing
-------

* Enabled smooth per pixel scrolling

### C++

* Updated prebuilt binaries to LLVM 20.1.3
* Added refactoring actions for adding string literal operators to literals
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-17.0/creator-reference-cpp-quick-fixes.html))
* Fixed the indentation for function-like built-ins
  ([QTCREATORBUG-32667](https://bugreports.qt.io/browse/QTCREATORBUG-32667))
* Fixed issues with function-like macros
  ([QTCREATORBUG-32598](https://bugreports.qt.io/browse/QTCREATORBUG-32598))
* Built-in
    * Fixed the highlighting after 2D array initialization
      ([QTCREATORBUG-32848](https://bugreports.qt.io/browse/QTCREATORBUG-32848))

### QML

* Integrated `qmlformat` more tightly
  ([QTCREATORBUG-26602](https://bugreports.qt.io/browse/QTCREATORBUG-26602))
* Added a button for opening `.ui.qml` files in Qt Design Studio to the editor
  tool bar and a setting for the location of Qt Design Studio when `QmlDesigner`
  is not enabled
  ([QTCREATORBUG-31005](https://bugreports.qt.io/browse/QTCREATORBUG-31005))
* Fixed the highlighting of `of` in `for`-loops
  ([QTCREATORBUG-32843](https://bugreports.qt.io/browse/QTCREATORBUG-32843))
* Fixed an issue with `Move Component into Separate File`
  ([QTCREATORBUG-32033](https://bugreports.qt.io/browse/QTCREATORBUG-32033))

### Language Server Protocol

* Fixed that the `detail` field of `Document Symbols` was ignored
  ([QTCREATORBUG-31766](https://bugreports.qt.io/browse/QTCREATORBUG-31766))

### SCXML

* Improved adaptation to Qt Creator theme
  ([QTCREATORBUG-29701](https://bugreports.qt.io/browse/QTCREATORBUG-29701))

Projects
--------

* Removed the explicit Haskell project support (use a Workspace project instead)
* Changed run configurations to be configured per build configuration
  ([QTCREATORBUG-20986](https://bugreports.qt.io/browse/QTCREATORBUG-20986),
   [QTCREATORBUG-32380](https://bugreports.qt.io/browse/QTCREATORBUG-32380))
* Changed the project configuration page to only select `Debug` configurations
  by default
* Improved the behavior of `Next Item` and `Previous Item` in the `Issues` view
  ([QTCREATORBUG-32503](https://bugreports.qt.io/browse/QTCREATORBUG-32503))
* Added `Clone into This` for copying the data of a different run configuration
  into the current run configuration
  ([QTCREATORBUG-26825](https://bugreports.qt.io/browse/QTCREATORBUG-26825))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-17.0/creator-run-settings.html))
* Added the `Add Project Dependency` refactoring action for missing included
  Qt files to add the missing package dependency to the project file
* Added the `Add #include and Project Dependency` refactoring action for unknown
  Qt classes to include the corresponding header and add the missing package
  dependency to the project file
* Added the option to use custom output parsers for all build or run
  configurations by default in `Preferences > Build & Run > Custom Output Parsers`
  ([QTCREATORBUG-32342](https://bugreports.qt.io/browse/QTCREATORBUG-32342))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-17.0/creator-custom-output-parsers.html))
* Added the option to select `qtpaths` instead of `qmake` when registering
  Qt versions
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-17.0/creator-project-qmake.html))
  ([QTCREATORBUG-32213](https://bugreports.qt.io/browse/QTCREATORBUG-32213))
* Fixed `Open Terminal Here` in run settings
  ([QTCREATORBUG-32841](https://bugreports.qt.io/browse/QTCREATORBUG-32841))
* Fixed that cloning a build configuration did not re-apply the build directory
  template
  ([QTCREATORBUG-31062](https://bugreports.qt.io/browse/QTCREATORBUG-31062))
* Fixed removing devices with `sdktool`
  ([QTCREATORBUG-32872](https://bugreports.qt.io/browse/QTCREATORBUG-32872))

### CMake

* Added the option to install missing Qt components with the Qt Online Installer
  when the CMake configuration fails with missing Qt packages
  ([QTCREATORBUG-32323](https://bugreports.qt.io/browse/QTCREATORBUG-32323))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-17.0/creator-how-to-edit-cmake-config-files.html))
* Presets
    * Fixed the run device type for non-desktop targets
      ([QTCREATORBUG-32943](https://bugreports.qt.io/browse/QTCREATORBUG-32943))
* vcpkg
    * Fixed that project local `vcpkg` installations were not prioritized
      ([QTCREATORBUG-32947](https://bugreports.qt.io/browse/QTCREATORBUG-32947))

### qmake

* Fixed that `QMAKE_PROJECT_NAME` was not used for run configuration names
  ([QTCREATORBUG-32465](https://bugreports.qt.io/browse/QTCREATORBUG-32465))

### Python

* Added support for `pyproject.toml` projects
  ([QTCREATORBUG-22492](https://bugreports.qt.io/browse/QTCREATORBUG-22492),
   [PYSIDE-2714](https://bugreports.qt.io/browse/PYSIDE-2714))

Debugging
---------

### C++

* LLDB
    * Fixed the pretty printer for `QMap` on ARM Macs
      ([QTCREATORBUG-32309](https://bugreports.qt.io/browse/QTCREATORBUG-32309))
    * Fixed the pretty printer for `QImage`
      ([QTCREATORBUG-32390](https://bugreports.qt.io/browse/QTCREATORBUG-32390))

### QML

* Fixed QML debugging with `Run in Terminal` enabled
  ([QTCREATORBUG-32871](https://bugreports.qt.io/browse/QTCREATORBUG-32871))

Analyzer
--------

### Axivion

* Added settings for
  `Axivion Suite path`,
  `Save all open files before starting an analysis`,
  `BAUHAUS_PYTHON`, and
  `JAVA_HOME`
  in `Preferences > Analyze > Axivion`.
* Added tool buttons for `Local Build` and `Local Dashboard` to the `Issues`
  view in the `Debug > Axivion` mode
  ([QTCREATORBUG-32385](https://bugreports.qt.io/browse/QTCREATORBUG-32385))

### Coco

* Fixed that the highlighting via CoverageBrowser was not started automatically
  ([QTCREATORBUG-32645](https://bugreports.qt.io/browse/QTCREATORBUG-32645))

Terminal
--------

* Added the option to reflow the text when resizing the terminal window
  in `Preferences > Terminal > Enable live reflow (Experimental)`

Version Control Systems
-----------------------

* Added `Log Directory` to directories in the `File System` view
  ([QTCREATORBUG-32540](https://bugreports.qt.io/browse/QTCREATORBUG-32540))

### Git

* Added the `%{Git:Config:<key>}` Qt Creator variable for Git configuration
  values
* Added actions for staged changes
  ([QTCREATORBUG-32361](https://bugreports.qt.io/browse/QTCREATORBUG-32361))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-17.0/creator-how-to-git-diff.html))
* Added `Revert` to the actions in the `Instant Blame` tooltip
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-17.0/creator-how-to-git-reset.html))
* Added the option to create annotated tags to the `Create Branch` dialog
* Added a `Diff & Cancel` option to the `Uncommitted Changes Found` dialog
  ([QTCREATORBUG-25795](https://bugreports.qt.io/browse/QTCREATORBUG-25795))
* Added a `.gitignore` file when creating a repository in an existing directory
  ([QTCREATORBUG-29776](https://bugreports.qt.io/browse/QTCREATORBUG-29776))
* Fixed that numbers in file names were interpreted as commit IDs
  ([QTCREATORBUG-32740](https://bugreports.qt.io/browse/QTCREATORBUG-32740))

Test Integration
----------------

* Added wizards for Qt 6 only CMake projects
  ([QTCREATORBUG-32578](https://bugreports.qt.io/browse/QTCREATORBUG-32578))
* Fixed test output parsing if that does not end in a newline
  ([QTCREATORBUG-32768](https://bugreports.qt.io/browse/QTCREATORBUG-32768))

Platforms
---------

### Windows

* Re-enabled all functionality of the debugger that calls functions on the
  debugged items when using GDB from MinGW
  ([QTCREATORBUG-30661](https://bugreports.qt.io/browse/QTCREATORBUG-30661))

### macOS

* Fixed an issue with deploying files to remote machines
  ([QTCREATORBUG-32613](https://bugreports.qt.io/browse/QTCREATORBUG-32613))

### Android

* Dropped support for GDB (LLDB is available for Qt 5.15.9 and later and Qt 6.2
  and later)
* Worked around issues with LLDB from NDK 27 and later on macOS
* Fixed that Valgrind actions were enabled
  ([QTCREATORBUG-32336](https://bugreports.qt.io/browse/QTCREATORBUG-32336))
* Fixed more occurrences of the debugger breaking in unrelated code
  ([QTCREATORBUG-32937](https://bugreports.qt.io/browse/QTCREATORBUG-32937))

### Docker

* Added the `Port Mappings` device setting
* Fixed that the environment from the container entrypoint was not used even
  with `Do not modify entry point` checked
  ([QTCREATORBUG-32135](https://bugreports.qt.io/browse/QTCREATORBUG-32135))
* Fixed that the `cmdbridge` process kept running on the device
  ([QTCREATORBUG-32450](https://bugreports.qt.io/browse/QTCREATORBUG-32450))

Credits for these changes go to:
--------------------------------
Aleksei German  
Alessandro Portale  
Alexander Drozdov  
Alexandru Croitor  
Ali Kianian  
Amr Essam  
Andre Hartmann  
André Pönitz  
Andrii Semkiv  
Andrzej Biniek  
BogDan Vatra  
Brook Cronin  
Burak Hancerli  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Dmitrii Akshintsev  
Eike Ziller  
faust747  
Friedemann Kleint  
Henning Gruendl  
Jaime Resano  
Jaroslaw Kobus  
Johanna Vanhatapio  
Kai Köhne  
Karim Pinter  
Karol Herda  
Knud Dollereder  
Krzysztof Chrusciel  
Leena Miettinen  
Liu Zhangjian  
Lukasz Papierkowski  
Mahmoud Badri  
Marco Bubke  
Marcus Tillmanns  
Markus Redeker  
Martin Leutelt  
Mats Honkamaa  
Michael Weghorn  
Miikka Heikkinen  
Nikita Baryshnikov  
Orgad Shaneh  
Orkun Tokdemir  
Pranta Dastider  
Przemyslaw Lewandowski  
Rafal Andrusieczko  
Rafal Stawarski  
Ralf Habacker  
Robert Löhning  
Sami Shalayel  
Samuli Piippo  
Semih Yavuz  
Shrief Gabr  
Sivert Krøvel  
Teea Poldsam  
Thiago Macieira  
Thiemo van Engelen  
Thomas Hartmann  
Tian Shilin  
Tim Jenßen  
Vikas Pachdha  
Zoltan Gera  
