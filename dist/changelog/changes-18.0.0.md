Qt Creator 18
=============

Qt Creator version 18 contains bug fixes and new features.
It is a free upgrade for commercial license holders.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=17.0..v18.0.0>

New plugins
-----------

### Development Container Support

The development container support detects a `devcontainer.json` in your project
directory and creates a docker container for it.
It supports Qt Creator specific `customizations` in the `devcontainer.json` that
let you auto detect or specify custom kits for the container and control other
aspects like the command bridge.

([Development Container Documentation](https://containers.dev/))

General
-------

* Moved some messages from the `General Messages` to `Issues`
* Changed the notifications to popups with the option to opt-out with
  `Environment > Interface > Prefer banner style info bars over pop-ups`
* Added the `HostOs:DocumentsLocation`, `HostOs:GenericDataLocation`,
  `HostOs:HomeLocation`, and `HostOs:TempLocation` Qt Creator variables that
  map to the corresponding `QStandardPaths`
* Added the `:DirName` postfix for Qt Creator variables that map to file paths
  ([QTCREATORBUG-33205](https://bugreports.qt.io/browse/QTCREATORBUG-33205))
* Fixed a freeze when installing large plugins
  ([QTCREATORBUG-33069](https://bugreports.qt.io/browse/QTCREATORBUG-33069))
* Welcome
    * Added an `Overview` tab
* Locator
    * Made `Use Tab Completion` (when pressing the `Tab` key) optional
      ([QTCREATORBUG-33193](https://bugreports.qt.io/browse/QTCREATORBUG-33193))

Editing
-------

* Added the option `Environment > Interface > Use tabbed editors`
  ([QTCREATORBUG-31644](https://bugreports.qt.io/browse/QTCREATORBUG-31644))
    * Added `Window > Previous Tab` and `Window > Next Tab`
* Added `File > Save Without Formatting`
* Added `Open With > Cycle to Next Editor`
  ([QTCREATORBUG-32482](https://bugreports.qt.io/browse/QTCREATORBUG-32482),
   [QTCREATORBUG-32610](https://bugreports.qt.io/browse/QTCREATORBUG-32610))
* Fixed that error markers were not removed when fixing the error
  ([QTCREATORBUG-33108](https://bugreports.qt.io/browse/QTCREATORBUG-33108))

### C++

* Added automatic insertion of the closing part of a raw string literal prefix
  ([QTCREATORBUG-31901](https://bugreports.qt.io/browse/QTCREATORBUG-31901))
* Fixed that trailing white space was removed from raw string literals
  ([QTCREATORBUG-30003](https://bugreports.qt.io/browse/QTCREATORBUG-30003))
* Fixed that `Re-order Member Function Definitions According to Declaration Order`
  did not move comments accordingly
  ([QTCREATORBUG-33070](https://bugreports.qt.io/browse/QTCREATORBUG-33070))
* Fixed the generation of `compile_commands.json` for remote projects
* Quick fixes
    * Added `Remove Curly Braces`
    * Added `Add definition` for static data members
      ([QTCREATORBUG-20961](https://bugreports.qt.io/browse/QTCREATORBUG-20961))
    * Fixed issues with templates and nested classes
      ([QTCREATORBUG-9727](https://bugreports.qt.io/browse/QTCREATORBUG-9727))
    * Fixed issues with nested template parameters
      ([QTCREATORBUG-17695](https://bugreports.qt.io/browse/QTCREATORBUG-17695))
* Built-in
    * Added support for template deduction guides
    * Added support for fold expressions
    * Added support for `decltype(auto)` as function return types
    * Added support for `if consteval` and `if not consteval`
    * Added support for __int128
    * Fixed the preprocessor expansion in include directives
      ([QTCREATORBUG-27473](https://bugreports.qt.io/browse/QTCREATORBUG-27473))
    * Fixed issues with defaulted and deleted functions
      ([QTCREATORBUG-26090](https://bugreports.qt.io/browse/QTCREATORBUG-26090))
    * Fixed the parsing of friend declarations with a simple type specifier
    * Fixed the parsing of attributes after an operator name

### QML

* Added the option to install Qt Design Studio via the Qt Online Installer
  ([QTCREATORBUG-30787](https://bugreports.qt.io/browse/QTCREATORBUG-30787))
* Added the option to override the path to `qmlls`
  ([QTCREATORBUG-32749](https://bugreports.qt.io/browse/QTCREATORBUG-32749))

### Copilot

* Added support for GitHub Enterprise environments
  ([QTCREATORBUG-33220](https://bugreports.qt.io/browse/QTCREATORBUG-33220))
* Fixed configuration issues with Copilot >= v1.49.0

### Markdown

* Improved table rendering
* Fixed the scaling of images
  ([QTCREATORBUG-33325](https://bugreports.qt.io/browse/QTCREATORBUG-33325))

### SCXML

* Fixed the positioning of the transition arrow
  ([QTCREATORBUG-32654](https://bugreports.qt.io/browse/QTCREATORBUG-32654))

### GLSL

* Fixed the handling of interface blocks
  ([QTCREATORBUG-12784](https://bugreports.qt.io/browse/QTCREATORBUG-12784),
   [QTCREATORBUG-27068](https://bugreports.qt.io/browse/QTCREATORBUG-27068))

Projects
--------

* Moved the project settings to a `.qtcreator` subdirectory in the project
  directory. The `.user` file at the old location in the project directory is
  kept up to date in addition, for old projects
  ([QTCREATORBUG-28610](https://bugreports.qt.io/browse/QTCREATORBUG-28610))
* Changed the `Build` and `Run` subitems to tabs in `Projects` mode and
  separated `Deploy Settings` from `Run Settings`
* Changed the `Current Project` advanced search to `Single Project` with
  an explicit choice of the project to search
  ([QTCREATORBUG-29790](https://bugreports.qt.io/browse/QTCREATORBUG-29790))
* Removed the `Code Snippet` wizard from `File > New Project > Other Project`.
  Use `Plain C++` instead
* Made options from the global `Build & Run` settings available as project
  specific options
* Made `Copy Steps From Another Kit` available without first enabling the kit
  ([QTCREATORBUG-24123](https://bugreports.qt.io/browse/QTCREATORBUG-24123))
* Made the default deploy configuration available for all target devices
* Added a configuration for various tools on devices, like GDB server, CMake,
  clangd, rsync, qmake, and more, and the option to auto-detect them
* Added the setting `Build & Run > General > Keep run configurations in sync`
  with the option to synchronize run configurations within one or all kits
  ([QTCREATORBUG-33172](https://bugreports.qt.io/browse/QTCREATORBUG-33172))
* Added the tool button `Create Issues From External Build Output` to the
  `Issues` view
  ([QTCREATORBUG-30776](https://bugreports.qt.io/browse/QTCREATORBUG-30776))
* Added the
  `Preferences > Build & Run > Default Build Properties > Default working directory`
  setting for run configurations
* Added keyboard shortcuts for editing the active build and run configurations
  ([QTCREATORBUG-27887](https://bugreports.qt.io/browse/QTCREATORBUG-27887))
* Added the option to add a file to a project directly from the
  `This file is not part of any project` warning
  ([QTCREATORBUG-25834](https://bugreports.qt.io/browse/QTCREATORBUG-25834))
* Added the `Project` Qt Creator variable for the build configuration settings
  that maps to the project file path
* Added a Qt Interface Framework project wizard
  ([QTBUG-99070](https://bugreports.qt.io/browse/QTBUG-99070))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-18.0/creator-how-to-create-qtif.html))
* Added the `Enable logging category filtering` option for desktop run
  configurations with Qt 6.11 and later
  ([QTCREATORBUG-33169](https://bugreports.qt.io/browse/QTCREATORBUG-33169))
* Fixed `Duplicate File` for remote projects

### CMake

* Added more detailed information to the build progress tool tip
  ([QTCREATORBUG-33356](https://bugreports.qt.io/browse/QTCREATORBUG-33356))
* Added the `ct` locator filter for running CTest tests
* Fixed `Build for All Configurations`
  ([QTCREATORBUG-33178](https://bugreports.qt.io/browse/QTCREATORBUG-33178))
* Fixed issues with rewriting `CMakeLists.txt` files with the UTF-8 BOM set
  ([QTCREATORBUG-33363](https://bugreports.qt.io/browse/QTCREATORBUG-33363))
* vcpkg

### qmake

* Fixed various issues with opening remote projects
  TODO: what state is that exactly in now?

### Python

* Removed PySide2 from the project wizard options
  ([QTCREATORBUG-33030](https://bugreports.qt.io/browse/QTCREATORBUG-33030))

### Workspace

* Changed projects to be automatically configured for the default kit on first
  use
* Added minimal support for Cargo build projects (Rust)

Debugging
---------

### C++

* Fixed `Load QML Stack`
  ([QTCREATORBUG-33244](https://bugreports.qt.io/browse/QTCREATORBUG-33244))

Analyzer
--------

### Clang

* Added Clang-Tidy and Clazy issues from the current document to the `Issues`
  view
  ([QTCREATORBUG-29789](https://bugreports.qt.io/browse/QTCREATORBUG-29789))
* Improved the performance of loading diagnostics from a file
* Fixed freezes when applying multiple fix-its
  ([QTCREATORBUG-25394](https://bugreports.qt.io/browse/QTCREATORBUG-25394))

### Axivion

* Added a request for the user to add a path mapping when opening files from
  the issues table and none exist

### Coco

* Fixed issues with MinGW
  ([QTCREATORBUG-33287](https://bugreports.qt.io/browse/QTCREATORBUG-33287))

Version Control Systems
-----------------------

### Git

* Added `Git > Local Repository > Patch > Apply from Clipboard`
* Added `Git > Local Repository > Patch > Create from Commits`
* Commit editor
    * Added `Recover File`, `Revert All Changes to File`, and
      `Revert Unstaged Changes to File` to the context menu on files
    * Added `Stage`, `Unstage`, and `Add to .gitignore` to the context menu on
      untracked files
    * Added actions for resolving conflicts
* Added an error indicator and error messages to the `Add Branch` dialog
* Added `Diff & Cancel` to the `Checkout Branch` dialog
* Added a visualization of the version control state to the `File System` view
* Improved performance of file modification status updates
  ([QTCREATORBUG-32002](https://bugreports.qt.io/browse/QTCREATORBUG-32002))
* Fixed updating the `Branch` view after changes
  ([QTCREATORBUG-29918](https://bugreports.qt.io/browse/QTCREATORBUG-29918))

Platforms
---------

### macOS

* Removed the auto-detection of 32-bit compilers
* Made it clearer which auto-detected toolchains are only for iOS
* Fixed that the automatically set toolchain for desktop kits could be an iOS
  toolchain

### Android

* Fixed the qmake project path set when creating APK templates
  ([QTCREATORBUG-33215](https://bugreports.qt.io/browse/QTCREATORBUG-33215))

### Remote Linux

* Added the `Auto-connect on startup` option and removed automatic connection
  to devices if it is turned off (the default)
* Added support for deployment with `rsync` with remote build devices
* Improved the error message when device tests fail
  ([QTCREATORBUG-32933](https://bugreports.qt.io/browse/QTCREATORBUG-32933))

### Docker

* Added the option `Mount Command Bridge` to the docker device configuration
  ([QTCREATORBUG-33006](https://bugreports.qt.io/browse/QTCREATORBUG-33006))

Credits for these changes go to:
--------------------------------
Aaron McCarthy  
Alessandro Portale  
Alexandru Croitor  
Alexis Jeandet  
Ali Kianian  
Amr Essam  
Andre Hartmann  
Andrzej Biniek  
André Pönitz  
Assam Boudjelthia  
Björn Schäpers  
Burak Hancerli  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Dheerendra Purohit  
Eike Ziller  
Eren Bursali  
faust747  
Friedemann Kleint  
Jaroslaw Kobus  
Johanna Vanhatapio  
Kai Köhne  
Leena Miettinen  
Mahmoud Badri  
Marco Bubke  
Marcus Tillmanns  
Markus Redeker  
Miikka Heikkinen  
Mitch Curtis  
Nicholas Bennett  
Nikita Baryshnikov  
Olivier De Cannière  
Orgad Shaneh  
Philip Van Hoof  
Renaud Guezennec  
Sami Shalayel  
Samuli Piippo  
Semih Yavuz  
Sheree Morphett  
Stanislav Polukhanov  
Teea Poldsam  
Thiago Macieira  
Tian Shilin  
Tim Jenßen  
Volodymyr Zibarov  
Xavier Besson  
Zoltan Gera  
