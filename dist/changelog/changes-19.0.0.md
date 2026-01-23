Qt Creator 19
=============

Qt Creator version 19 contains bug fixes and new features.
It is a free upgrade for all users.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=18.0..v19.0.0>

New plugins
-----------

### MCP Server

Implements a basic
[MCP server](https://modelcontextprotocol.io/docs/getting-started/intro)
for Qt Creator that allows opening files and projects, as well as building,
running, and debugging, and a few other actions.

### Ant

Adds lightweight support for projects using the
[Ant build system](https://ant.apache.org/).
Open `build.xml` files as workspace projects that use `ant build` for
building.

([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-project-opening.html))

### Cargo

Adds lightweight support for `cargo build` of Cargo, the Rust package manager.
Open `Cargo.toml` files as workspace projects that use `cargo build` for
building, and `cargo run` for running.

([Cargo Documentation](https://doc.rust-lang.org/cargo/commands/cargo-build.html))  
([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-project-opening.html))

### Dotnet

Adds lightweight support for .NET projects. Open `.csproj` files as workspace
projects that use `dotnet build` for building and `dotnet run` for running.
It offers to set up the `csharp-ls` language server, if found.

([.NET CLI Documentation](https://learn.microsoft.com/en-us/dotnet/core/tools/))  
([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-project-opening.html))

### Gradle

Adds lightweight support for projects using the
[Gradle Build Tool](https://gradle.org/). Open `.gradle` or `.gradle.kts` files
as workspace projects that offer to run `gradlew` or `gradle` for building, and
`gradlew run` or `gradle run` for running.

([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-project-opening.html))

### Swift

Adds lightweight support for [Swift](https://www.swift.org/) projects. Open
`Package.swift` files as workspace projects that use `swift build` for building
and `swift run` for running. It offers to set up the `sourcekit-lsp` Swift
language server, if found.

([Swift Package Manager Documentation](https://docs.swift.org/swiftpm/documentation/packagemanagerdocs))  
([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-project-opening.html))

General
-------

* Moved `Preferences` from a dialog to a mode
  ([QTCREATORBUG-33787](https://bugreports.qt.io/browse/QTCREATORBUG-33787))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-switch-between-modes.html))
* Added the root folders of connected devices to the `File System` view
  ([QTCREATORBUG-33577](https://bugreports.qt.io/browse/QTCREATORBUG-33577))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-file-system-view.html))
* Added the option `Ignore generated files` to `Advanced Find` and Locator
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-reference-search-results-view.html))
* Added support for
  [freedesktop.org compatible file managers](https://freedesktop.org/wiki/Specifications/file-manager-interface/)
  to `Show in File Manager`
  ([QTCREATORBUG-29368](https://bugreports.qt.io/browse/QTCREATORBUG-29368))
* Added the option to explicitly remove categories for external tools
  ([QTCREATORBUG-33316](https://bugreports.qt.io/browse/QTCREATORBUG-33316))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-editor-external.html))
* Added the Qt Creator variable `HostOs:BatchFileSuffix`
  ([Documentation](https://doc.qt.io/qtcreator/creator-how-to-use-qtc-variables.html))
* Added the setting `Environment > System > Variable separators` for defining
  custom separators for list-style environment variables
  ([QTCREATORBUG-33790](https://bugreports.qt.io/browse/QTCREATORBUG-33790))
  ([Documentation](https://doc.qt.io/qtcreator/creator-how-to-edit-environment-settings.html))
* Added auto-correction of the pattern separators in `Advanced Find` and
  `Preferences > Environment > MIME Types`
* Added `Copy Expanded Value` to the context menu of path choosers

Editing
-------

* Added the option `Text Editor > Display > Enable minimap` to show a minimap in
  text editors
  ([QTCREATORBUG-29177](https://bugreports.qt.io/browse/QTCREATORBUG-29177))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-coding-navigating.html#using-minimap))
* Added `Close All Tabs` and `Close Other Tabs` for the tabbed editors
  ([Documentation](https://doc.qt.io/qtcreator/creator-coding-navigating.html#using-tabbed-editors))
* Added a pinned indicator that also acts as an unpin button to tabs for
  pinned documents
* Added that tabs for pinned documents are kept at the front
  ([QTCREATORBUG-33702](https://bugreports.qt.io/browse/QTCREATORBUG-33702))
* Fixed code folding with multi-line comments
  ([QTCREATORBUG-33857](https://bugreports.qt.io/browse/QTCREATORBUG-33857))

### C++

* Added a quick fix for adding a class or struct definition from a forward
  declaration
  ([QTCREATORBUG-19929](https://bugreports.qt.io/browse/QTCREATORBUG-19929))
  ([Documentation](https://doc.qt.io/qtcreator/creator-reference-cpp-quick-fixes.html))
* Added an image preview when hovering over references to image resources
  ([QTCREATORBUG-29727](https://bugreports.qt.io/browse/QTCREATORBUG-29727),
   [QTCREATORBUG-29819](https://bugreports.qt.io/browse/QTCREATORBUG-29819))
* Fixed issues with parameter packs when refactoring
  ([QTCREATORBUG-32597](https://bugreports.qt.io/browse/QTCREATORBUG-32597))
* Fixed that `Add Definition` was not available for `friend` functions
  ([QTCREATORBUG-31048](https://bugreports.qt.io/browse/QTCREATORBUG-31048))
* Fixed indenting with the `TAB` key after braced initializer
  ([QTCREATORBUG-31759](https://bugreports.qt.io/browse/QTCREATORBUG-31759))
* Fixed missing `struct` keywords when generating code
  ([QTCREATORBUG-20838](https://bugreports.qt.io/browse/QTCREATORBUG-20838))
* Built-in
    * Improved the memory footprint of the index
    * Fixed indentation within lambda functions
      ([QTCREATORBUG-15156](https://bugreports.qt.io/browse/QTCREATORBUG-15156))
* Clangd
    * Fixed issues with parse contexts for header files
      ([QTCREATORBUG-33642](https://bugreports.qt.io/browse/QTCREATORBUG-33642))

### QML

* Added the option `Set QT_QML_GENERATE_QMLLS_INI to ON in CMake` to the
  `Qt Quick Application` wizard
  ([QTCREATORBUG-33673](https://bugreports.qt.io/browse/QTCREATORBUG-33673))
* Added an image preview when hovering over references to image resources
  ([QTCREATORBUG-29727](https://bugreports.qt.io/browse/QTCREATORBUG-29727),
   [QTCREATORBUG-29819](https://bugreports.qt.io/browse/QTCREATORBUG-29819))
* Added the option to rename all usages of a QML component when renaming the
  file
  ([QTCREATORBUG-33195](https://bugreports.qt.io/browse/QTCREATORBUG-33195))
* Improved the performance of scanning for QML files
* Improved the `qmlformat` settings
  ([QTCREATORBUG-33305](https://bugreports.qt.io/browse/QTCREATORBUG-33305))
* Fixed a wrong warning `Duplicate Id. (M15)`
  ([QTCREATORBUG-32418](https://bugreports.qt.io/browse/QTCREATORBUG-32418))
* Fixed that `Split Initializer` did not remove unneeded semicolons
  ([QTCREATORBUG-16207](https://bugreports.qt.io/browse/QTCREATORBUG-16207))
* qmlls
    * Added the option `Enable qmlls's CMake integration`
    * Added the option to `Deploy INI File to Current Project`
    * Fixed that custom indentation size was not respected when reformatting
      ([QTCREATORBUG-33712](https://bugreports.qt.io/browse/QTCREATORBUG-33712))

### Diff Viewer

* Added `Fold All` and `Unfold All`
  ([QTCREATORBUG-33783](https://bugreports.qt.io/browse/QTCREATORBUG-33783))
* Added `Copy Cleaned Text` that removes leading dashes and plus signs
  ([QTCREATORBUG-23694](https://bugreports.qt.io/browse/QTCREATORBUG-23694))

### SCXML

* Fixed the handling of the implicit initial state
  ([QTCREATORBUG-32603](https://bugreports.qt.io/browse/QTCREATORBUG-32603))

### GLSL

* Updated the parser to GLSL 4.60
  ([QTCREATORBUG-32899](https://bugreports.qt.io/browse/QTCREATORBUG-32899))
* Added support for Vulkan
  ([QTCREATORBUG-26057](https://bugreports.qt.io/browse/QTCREATORBUG-26057))
* Added `.geom`, `.comp`, `.tesc`, and `.tese` to the recognized GLSL file
  extensions
  ([QTCREATORBUG-31779](https://bugreports.qt.io/browse/QTCREATORBUG-31779))

Projects
--------

* Improved startup performance when devices are configured to auto-connect
* Improved performance when opening projects (finding extra compilers for CMake
  projects)
* Consolidated the build device tools
    * Added a filter for the build device to the settings for Qt versions,
      compilers, debuggers, and CMake
    * Added the option to detect tools on build devices from their
      respective settings page
    * Added auto-detection of the tools with individual settings pages when
      auto-detection is triggered in the device settings
      ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-configure-tools-on-devices.html))
  ([QTCREATORBUG-33677](https://bugreports.qt.io/browse/QTCREATORBUG-33677))
* Added `Re-detect` to `Preferences > Kits > Qt Versions`
* Added the option to run applications as a different user
  ([QTCREATORBUG-33655](https://bugreports.qt.io/browse/QTCREATORBUG-33655))
* Added the option to search in generated files with the project related
  `Advanced Find` filters
  ([QTCREATORBUG-33579](https://bugreports.qt.io/browse/QTCREATORBUG-33579))
* Added support for Qt Creator variables in the build directory setting
  ([QTCREATORBUG-24121](https://bugreports.qt.io/browse/QTCREATORBUG-24121))
* Added `Bulk Remove` to run configuration settings
* Added the option to export and import custom output parsers
* Fixed that local paths in the device tool settings were not interpreted as
  paths on the device

### CMake

* Split up the CMake helper file into the package manager auto-setup part and
  a more generic `qtcreator-project.cmake` helper that contains the more
  generic functionality
* Improved the auto-installation of missing Qt components
    * Removed the configure-time dependency on the Qt Maintenance Tool
    * Removed the automatic execution of the Qt Online Installer
    * Added informative CMake output and a task with a link that starts the
      installation process on user request
* Improved the performance of parsing projects
* Improved the `QML debugging and profiling` setting for build configurations
  by not relying on `CMAKE_CXX_FLAGS_INIT`
  ([QTCREATORBUG-25016](https://bugreports.qt.io/browse/QTCREATORBUG-25016),
   [QTBUG-139293](https://bugreports.qt.io/browse/QTBUG-139293))
* Added `Preferences > CMake > General > Clear old CMake output on a new run`
  ([QTCREATORBUG-33838](https://bugreports.qt.io/browse/QTCREATORBUG-33838))
* Removed the special `CMake From Build Device` setting for kits that is no
  longer needed
* Fixed that building a single target always built everything if
  `Install into staging directory` is enabled
  ([QTCREATORBUG-33580](https://bugreports.qt.io/browse/QTCREATORBUG-33580))
* Presets
    * Added support for `graphviz` and `trace`
      ([QTCREATORBUG-33943](https://bugreports.qt.io/browse/QTCREATORBUG-33943))

### qmake

* Fixed `Build single file` if more build steps were added
  ([QTCREATORBUG-29837](https://bugreports.qt.io/browse/QTCREATORBUG-29837))

### Python

* Added the option to configure a C++ debugger for Python kits

### Workspace

* Added environment settings to the run configurations

### Compilation Database

* Fixed issues with remote build devices
  ([QTCREATORBUG-33739](https://bugreports.qt.io/browse/QTCREATORBUG-33739))

Debugging
---------

### C++

* GDB
    * Fixed the connection to a gdbserver that uses SSH port forwarding
      ([QTCREATORBUG-33620](https://bugreports.qt.io/browse/QTCREATORBUG-33620))

Analyzer
--------

### QML Profiler

* Added the option to resize the category column in the timeline view
  ([QTCREATORBUG-33045](https://bugreports.qt.io/browse/QTCREATORBUG-33045))

### Coco

* Fixed the recognition of Coco installations on macOS
  ([QTCREATORBUG-33476](https://bugreports.qt.io/browse/QTCREATORBUG-33476))
* Fixed issues when `CMAKE_AR` is not set in some configurations
  ([QTCREATORBUG-33770](https://bugreports.qt.io/browse/QTCREATORBUG-33770))

### Valgrind

* Added support for protocol versions 5 and 6
  ([QTCREATORBUG-33759](https://bugreports.qt.io/browse/QTCREATORBUG-33759))
* Added the option to show `Size and Alignment Errors` and `Other` issues

Terminal
--------

([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-reference-terminal-view.html))
* Added a `qtc` helper function that passes its arguments to the running
  Qt Creator instance, for example for opening files from the terminal
  ([QTCREATORBUG-33784](https://bugreports.qt.io/browse/QTCREATORBUG-33784))
* Added the option to paste the values of Qt Creator variables into the terminal
  ([QTCREATORBUG-29282](https://bugreports.qt.io/browse/QTCREATORBUG-29282))

Version Control Systems
-----------------------

* Added the version control state colors to items in the `Projects` and
  `File System` views
  ([QTCREATORBUG-33689](https://bugreports.qt.io/browse/QTCREATORBUG-33689))
* Added the version control context menu actions to the `Projects` and
  `File System` views
  ([QTCREATORBUG-32540](https://bugreports.qt.io/browse/QTCREATORBUG-32540),
   [QTCREATORBUG-33687](https://bugreports.qt.io/browse/QTCREATORBUG-33687))
* Added the version control state information to the `Open Documents` view
  and the editor tabs
 ([QTCREATORBUG-33688](https://bugreports.qt.io/browse/QTCREATORBUG-33688))
* Added an explicit `Refresh` button to the submit editor
* Added an update interval setting to
  `Preferences > Version Control > Show VCS file status`

### Git

* Submit Editor
    * Added `Stage`, `Mark Untracked`, and `Remove` to added files
    * Added `Revert Renaming` to renamed files
* Added options for using `--rebase-merges` and `--update-refs` for
  `Interactive Rebase`
  ([QTCREATORBUG-33786](https://bugreports.qt.io/browse/QTCREATORBUG-33786))
* Added `Omit Path` and `Omit Author` to the blame viewer
  ([QTCREATORBUG-33636](https://bugreports.qt.io/browse/QTCREATORBUG-33636))
* Added support for cherry-picking a list of commits from the `Branches` view
* Added `Edit Commit Message` to the context menu on changes

Test Integration
----------------

* Fixed a performance issue when many tests fail
  ([QTCREATORBUG-32895](https://bugreports.qt.io/browse/QTCREATORBUG-32895))

### CTest

* Added skipped tests to the test summary
  ([QTCREATORBUG-33871](https://bugreports.qt.io/browse/QTCREATORBUG-33871))

Platforms
---------

### Windows

* Improved the performance of parsing MSVC output
  ([QTCREATORBUG-33756](https://bugreports.qt.io/browse/QTCREATORBUG-33756))
* Improved the detection performance for MSVC and CDB
* Fixed that `clang-cl` from Visual Studio installations was not detected
  ([QTCREATORBUG-33600](https://bugreports.qt.io/browse/QTCREATORBUG-33600))

### macOS

* Fixed that the cursor down key did not open the completion popup in input
  fields that support completion

### Android

* Added file access to Android devices (hardware and emulator) via all means
  like `File > Open From Device`, the Locator, the `File System` view.
  ([QTCREATORBUG-32697](https://bugreports.qt.io/browse/QTCREATORBUG-32697))
* Added auto-completion to the text based Android Manifest editor
* Removed the graphical `AndroidManifest.xml` editor
  ([QTCREATORBUG-33267](https://bugreports.qt.io/browse/QTCREATORBUG-33267))
* Fixed the insert location of the `target_properties` call when creating
  template files
  ([QTCREATORBUG-33360](https://bugreports.qt.io/browse/QTCREATORBUG-33360))

### iOS

* Fixed the deployment of applications built with a single-platform Qt version

### Remote Linux

* Added support for `Run as root`
* Added fast file access support for 32-bit ARM Linux devices
* Added the option to `Create kits` when auto-detecting device build tools

### Development Container

([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/devcontainer.html))
* Added the configuration files to the project tree

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Ali Kianian  
Andre Hartmann  
Andrii Semkiv  
Andrzej Biniek  
André Pönitz  
Burak Hancerli  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David M. Cotter  
David Schulz  
Eike Ziller  
Friedemann Kleint  
Henning Gruendl  
Jaroslaw Kobus  
Jochen Becher  
Johanna Vanhatapio  
Joker Principal  
Jussi Witick  
Karol Herda  
Knud Dollereder  
Leena Miettinen  
Mahmoud Badri  
Marco Bubke  
Marcus Tillmanns  
Markus Redeker  
Mats Honkamaa  
Miikka Heikkinen  
Mitch Curtis  
Olivier De Cannière  
Orgad Shaneh  
Pranta Dastider  
Rafal Andrusieczko  
Rami Potinkara  
Sami Shalayel  
Semih Yavuz  
Sheree Morphett  
Shrief Gabr  
Stanislav Polukhanov  
Sze Howe Koh  
Teea Poldsam  
Thomas Hartmann  
Tim Jenßen  
Ulf Hermann  
Vikas Pachdha  
Xavier Besson  
Zoltan Gera  
