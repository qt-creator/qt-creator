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

A basic [MCP server](https://modelcontextprotocol.io/docs/getting-started/intro)
for Qt Creator that lets AI assistants control opening files and projects,
as well as building, running, and debugging, and a few other actions.

([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-mcp-server.html))

### Ant

Lightweight support for projects using the
[Ant build system](https://ant.apache.org/manual/).
Open `build.xml` files as workspace projects that use `ant build` for
building.

([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-project-opening.html))

### Cargo

Lightweight support for projects using
[Cargo](https://doc.rust-lang.org/cargo/commands/cargo-build.html), the
Rust package manager. Open `Cargo.toml` files as workspace projects that
use `cargo build` for building, and `cargo run` for running.

([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-project-opening.html))

### Dotnet

Lightweight support for
[.NET projects](https://learn.microsoft.com/en-us/dotnet/core/tools/).
Open `.csproj` files as workspace projects that use `dotnet build` for building
and `dotnet run` for running. It offers to set up the `csharp-ls` language
server, if found.

([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-project-opening.html))

### Gradle

Lightweight support for projects using the
[Gradle Build Tool](https://docs.gradle.org/current/userguide/userguide.html).
Open `.gradle` or `.gradle.kts` files
as workspace projects that offer to run `gradlew` or `gradle` for building, and
`gradlew run` or `gradle run` for running.

([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-project-opening.html))

### Swift

Lightweight support for
[Swift](https://docs.swift.org/swiftpm/documentation/packagemanagerdocs) projects.
Open `Package.swift` files as workspace projects that use `swift build` for building
and `swift run` for running. It offers to set up the `sourcekit-lsp` Swift
language server, if found.

([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-project-opening.html))

General
-------

Added

* The root folders of connected devices to the `File System` view
  ([QTCREATORBUG-33577](https://bugreports.qt.io/browse/QTCREATORBUG-33577))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-file-system-view.html))
* The option to `Ignore generated files` in `Advanced Find` and Locator
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-reference-search-results-view.html))
* Support for
  [freedesktop.org compatible file managers](https://freedesktop.org/wiki/Specifications/file-manager-interface/)
  in `Show in File Manager`
  ([QTCREATORBUG-29368](https://bugreports.qt.io/browse/QTCREATORBUG-29368))
* The option to explicitly remove categories in `External Tools`:
  `Environment > External Tools > Remove`
  ([QTCREATORBUG-33316](https://bugreports.qt.io/browse/QTCREATORBUG-33316))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-editor-external.html))
* The Qt Creator variable `HostOs:BatchFileSuffix`
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-use-qtc-variables.html))
* The option to define custom separators for list-style environment variables:
  `Environment > System > Variable separators`
  ([QTCREATORBUG-33790](https://bugreports.qt.io/browse/QTCREATORBUG-33790))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-edit-environment-settings.html))
* Auto-correction of the pattern separators in `Advanced Find` and
  `Preferences > Environment > MIME Types`
* The option to `Copy Expanded Value` to the context menu of path choosers

Changed

* Moved `Preferences` from a dialog to a mode
  ([QTCREATORBUG-33787](https://bugreports.qt.io/browse/QTCREATORBUG-33787))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-switch-between-modes.html))

Editing
-------

Added

* The option to show a minimap in text editors:
  `Text Editor > Display > Enable minimap`
  ([QTCREATORBUG-29177](https://bugreports.qt.io/browse/QTCREATORBUG-29177))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-coding-navigating.html#using-minimap))
* Tabbed editors:
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-coding-navigating.html#using-tabbed-editors))
  * `Close All Tabs` and `Close Other Tabs` to the context menu
  * A pinned indicator that also acts as an unpin button to tabs for
    pinned documents
  * Tabs for pinned documents are kept at the front
    ([QTCREATORBUG-33702](https://bugreports.qt.io/browse/QTCREATORBUG-33702))

Fixed

* Code folding with multi-line comments
  ([QTCREATORBUG-33857](https://bugreports.qt.io/browse/QTCREATORBUG-33857))

### C++

Added

* A quick fix for adding a class or struct definition from a forward
  declaration
  ([QTCREATORBUG-19929](https://bugreports.qt.io/browse/QTCREATORBUG-19929))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-reference-cpp-quick-fixes.html))
* An image preview when hovering over references to image resources
  ([QTCREATORBUG-29727](https://bugreports.qt.io/browse/QTCREATORBUG-29727),
   [QTCREATORBUG-29819](https://bugreports.qt.io/browse/QTCREATORBUG-29819))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-preview-images.html))

Fixed

* Issues with parameter packs when refactoring
  ([QTCREATORBUG-32597](https://bugreports.qt.io/browse/QTCREATORBUG-32597))
* `Add Definition` was not available for `friend` functions
  ([QTCREATORBUG-31048](https://bugreports.qt.io/browse/QTCREATORBUG-31048))
* Indenting with the `TAB` key after braced initializer
  ([QTCREATORBUG-31759](https://bugreports.qt.io/browse/QTCREATORBUG-31759))
* Missing `struct` keywords when generating code
  ([QTCREATORBUG-20838](https://bugreports.qt.io/browse/QTCREATORBUG-20838))
* Built-in
    * The memory footprint of the index
    * Indentation within lambda functions
      ([QTCREATORBUG-15156](https://bugreports.qt.io/browse/QTCREATORBUG-15156))
* Clangd
    * Issues with parse contexts for header files
      ([QTCREATORBUG-33642](https://bugreports.qt.io/browse/QTCREATORBUG-33642))

### QML

Added

* The option to `Set QT_QML_GENERATE_QMLLS_INI to ON in CMake` to the
  `Qt Quick Application` wizard
  ([QTCREATORBUG-33673](https://bugreports.qt.io/browse/QTCREATORBUG-33673))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/quick-projects.html))
* An image preview when hovering over references to image resources
  ([QTCREATORBUG-29727](https://bugreports.qt.io/browse/QTCREATORBUG-29727),
   [QTCREATORBUG-29819](https://bugreports.qt.io/browse/QTCREATORBUG-29819))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-preview-images.html))
* The option to rename all usages of a QML component when renaming the file
  ([QTCREATORBUG-33195](https://bugreports.qt.io/browse/QTCREATORBUG-33195))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-rename-symbols.html))
* qmlls ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-use-qml-language-server.html))
    * The option to `Enable qmlls's CMake integration`
    * The option to `Deploy INI File to Current Project`

Changed

* Improved the performance of scanning for QML files
* Improved the `qmlformat` settings
  ([QTCREATORBUG-33305](https://bugreports.qt.io/browse/QTCREATORBUG-33305))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-preferences-qtquick-code-style.html))

Fixed

* A wrong warning `Duplicate Id. (M15)`
  ([QTCREATORBUG-32418](https://bugreports.qt.io/browse/QTCREATORBUG-32418))
* `Split Initializer` did not remove unneeded semicolons
  ([QTCREATORBUG-16207](https://bugreports.qt.io/browse/QTCREATORBUG-16207))
* qmlls
    * The handling of custom indentation sizes when reformatting
      ([QTCREATORBUG-33712](https://bugreports.qt.io/browse/QTCREATORBUG-33712))

### Diff Viewer

Added

* The option to `Fold All` and `Unfold All`
  ([QTCREATORBUG-33783](https://bugreports.qt.io/browse/QTCREATORBUG-33783))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-git-diff.html))
* `Copy Cleaned Text` to remove leading dashes and plus signs
  ([QTCREATORBUG-23694](https://bugreports.qt.io/browse/QTCREATORBUG-23694))

### SCXML

Fixed

* The handling of the implicit initial state
  ([QTCREATORBUG-32603](https://bugreports.qt.io/browse/QTCREATORBUG-32603))

### GLSL

Added

* Support for Vulkan
  ([QTCREATORBUG-26057](https://bugreports.qt.io/browse/QTCREATORBUG-26057))
* `.geom`, `.comp`, `.tesc`, and `.tese` to the recognized GLSL file extensions
  ([QTCREATORBUG-31779](https://bugreports.qt.io/browse/QTCREATORBUG-31779))

Changed

* Updated the parser to GLSL 4.60
  ([QTCREATORBUG-32899](https://bugreports.qt.io/browse/QTCREATORBUG-32899))

Projects
--------

Added

* The option to refresh the list of automatically detected Qt Versions:
  `Preferences > Kits > Qt Versions > Re-detect`
* The option to search in generated files with the project related
  `Advanced Find` filters
  ([QTCREATORBUG-33579](https://bugreports.qt.io/browse/QTCREATORBUG-33579))
* Support for Qt Creator variables in the build directory setting
  ([QTCREATORBUG-24121](https://bugreports.qt.io/browse/QTCREATORBUG-24121))
* `Bulk Remove` to run configuration settings
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-run-settings.html))
* The option to export and import custom output parsers:
  `Preferences > Build & Run > Custom Output Parsers`
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-custom-output-parsers.html))
* The automatic import of project specific custom parsers and code styles from
  the project source directory
  (`.qtcreator/customparsers.json` and `.qtcreator/codestyles/`)
  ([QTCREATORBUG-33839](https://bugreports.qt.io/browse/QTCREATORBUG-33839),
   [QTCREATORBUG-28913](https://bugreports.qt.io/browse/QTCREATORBUG-28913))
* Build device tools
    * A filter for the build device to the settings for Qt versions,
      compilers, debuggers, and CMake
    * The option to detect tools on build devices from their
      respective settings page
    * Auto-detection of the tools with individual settings pages when
      auto-detection is triggered in the device settings
      ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-configure-tools-on-devices.html))
      ([QTCREATORBUG-33677](https://bugreports.qt.io/browse/QTCREATORBUG-33677))

Changed

* Improved startup performance when devices are configured to auto-connect
* Improved performance when opening projects (finding extra compilers for CMake
  projects)
* Changed the option to run applications as root to run them as a different user:
  `Projects > Run Settings > Run as user`
  ([QTCREATORBUG-33655](https://bugreports.qt.io/browse/QTCREATORBUG-33655))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-run-settings-desktop-devices.html))

Fixed

* That local paths in the device tool settings were not interpreted as paths
  on the device

### CMake

Added

* The option to clear old output when you run CMake:
  `Preferences > CMake > General > Clear old CMake output on a new run`
  ([QTCREATORBUG-33838](https://bugreports.qt.io/browse/QTCREATORBUG-33838))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-build-settings-cmake.html#viewing-cmake-output))
* Support for `graphviz` and `trace` configure preset options
  ([QTCREATORBUG-33943](https://bugreports.qt.io/browse/QTCREATORBUG-33943))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-build-settings-cmake-presets.html))

Changed

* Split up the CMake helper file into the package manager auto-setup part and
  a more generic `qtcreator-project.cmake` helper that contains the more
  generic functionality
* Improved auto-installation of missing Qt components
    * Removed the configure-time dependency on the Qt Maintenance Tool
    * Removed the automatic execution of the Qt Online Installer
    * Qt Creator shows missing components in the `Issues` view
      with an option to install them
    ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-edit-cmake-config-files.html#install-missing-qt-packages))
* Improved the performance of parsing projects
* Improved the `QML debugging and profiling` setting for build configurations
  by not relying on `CMAKE_CXX_FLAGS_INIT`
  ([QTCREATORBUG-25016](https://bugreports.qt.io/browse/QTCREATORBUG-25016),
   [QTBUG-139293](https://bugreports.qt.io/browse/QTBUG-139293))
* Removed the special `CMake From Build Device` setting for kits that is no
  longer needed

Fixed

* That building a single target always built everything if
  `Install into staging directory` was enabled
  ([QTCREATORBUG-33580](https://bugreports.qt.io/browse/QTCREATORBUG-33580))

### qmake

Fixed

* `Build single file` if more build steps were added
  ([QTCREATORBUG-29837](https://bugreports.qt.io/browse/QTCREATORBUG-29837))

### Python

Added

* The option to configure a C++ debugger for Python kits:
  `Projects > Run Settings > Debugger Settings`

### Workspace

Added

* Environment settings to the run configurations

### Compilation Database

Fixed

* Issues with remote build devices
  ([QTCREATORBUG-33739](https://bugreports.qt.io/browse/QTCREATORBUG-33739))

Debugging
---------

### C++

Fixed

* GDB
    * The connection to a gdbserver that uses SSH port forwarding
      ([QTCREATORBUG-33620](https://bugreports.qt.io/browse/QTCREATORBUG-33620))

Analyzer
--------

### QML Profiler

Added

* The option to resize the category column in
  `Analyze > QML Profiler > Timeline`
  ([QTCREATORBUG-33045](https://bugreports.qt.io/browse/QTCREATORBUG-33045))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-profile-qml.html))

### Coco

Fixed

* The recognition of Coco installations on macOS
  ([QTCREATORBUG-33476](https://bugreports.qt.io/browse/QTCREATORBUG-33476))
* Issues when `CMAKE_AR` is not set in some configurations
  ([QTCREATORBUG-33770](https://bugreports.qt.io/browse/QTCREATORBUG-33770))

### Axivion

Added

* The explicit `Anonymous authentication` option
  ([QTCREATORBUG-33949](https://bugreports.qt.io/browse/QTCREATORBUG-33949))

### Valgrind

Added

* Support for protocol versions 5 and 6
  ([QTCREATORBUG-33759](https://bugreports.qt.io/browse/QTCREATORBUG-33759))
* The option to show `Size and Alignment Errors` and `Other` issues
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-memcheck.html))

Terminal
--------

([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-reference-terminal-view.html))

Added

* A `qtc` helper function that passes its arguments to the running
  Qt Creator instance, for example for opening files from the terminal
  ([QTCREATORBUG-33784](https://bugreports.qt.io/browse/QTCREATORBUG-33784))
* The option to paste the values of Qt Creator variables into the terminal
  ([QTCREATORBUG-29282](https://bugreports.qt.io/browse/QTCREATORBUG-29282))

Version Control Systems
-----------------------

Added

* The version control context menu actions to the `Projects` and
  `File System` views
  ([QTCREATORBUG-32540](https://bugreports.qt.io/browse/QTCREATORBUG-32540),
   [QTCREATORBUG-33687](https://bugreports.qt.io/browse/QTCREATORBUG-33687))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-use-common-vcs-functions.html))
* File state
  * The version control state colors to items in the `Projects` and
    `File System` views
    ([QTCREATORBUG-33689](https://bugreports.qt.io/browse/QTCREATORBUG-33689))
  * The version control state information to the `Open Documents` view
    and the editor tabs
    ([QTCREATORBUG-33688](https://bugreports.qt.io/browse/QTCREATORBUG-33688))
* An explicit `Refresh` button to the submit editor
* The option to set the file status update interval for Git:
  `Preferences > Version Control > Show VCS file status`
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-git-stage-changes.html))

### Git

([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-vcs-git.html))

Added

* Submit Editor ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-git-commit.html))
    * `Stage`, `Mark Untracked`, and `Remove` to added files
    * `Revert Renaming` to renamed files
* Options for using `--rebase-merges` and `--update-refs` for
  `Interactive Rebase`
  ([QTCREATORBUG-33786](https://bugreports.qt.io/browse/QTCREATORBUG-33786))
* `Omit Path` and `Omit Author` to the blame viewer
  ([QTCREATORBUG-33636](https://bugreports.qt.io/browse/QTCREATORBUG-33636))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-git-blame.html))
* Support for cherry-picking a list of commits from the `Branches` view
* `Edit Commit Message` to the context menu on changes

Test Integration
----------------

Fixed

* A performance issue when many tests fail
  ([QTCREATORBUG-32895](https://bugreports.qt.io/browse/QTCREATORBUG-32895))

### CTest

Added

* Skipped tests to the test summary
  ([QTCREATORBUG-33871](https://bugreports.qt.io/browse/QTCREATORBUG-33871))

Platforms
---------

### Windows

Changed

* Improved the performance of parsing MSVC output
  ([QTCREATORBUG-33756](https://bugreports.qt.io/browse/QTCREATORBUG-33756))
* Improved the detection performance for MSVC and CDB

Fixed

* The detection of `clang-cl` from Visual Studio installations
  ([QTCREATORBUG-33600](https://bugreports.qt.io/browse/QTCREATORBUG-33600))

### Linux

Fixed

* Issues with killing tool subprocesses
  ([QTCREATORBUG-27567](https://bugreports.qt.io/browse/QTCREATORBUG-27567),
   [QTCREATORBUG-32125](https://bugreports.qt.io/browse/QTCREATORBUG-32125))

### macOS

Fixed

* Opening the completion popup in input fields that support completion
  with the cursor down key
* The application icon on macOS 26 and later

### Android

Added

* File access to Android devices (hardware and emulator) via all means
  like `File > Open From Device`, the Locator, the `File System` view.
  ([QTCREATORBUG-32697](https://bugreports.qt.io/browse/QTCREATORBUG-32697))
* Auto-completion to the text based Android Manifest editor

Changed

* Removed the graphical `AndroidManifest.xml` editor
  ([QTCREATORBUG-33267](https://bugreports.qt.io/browse/QTCREATORBUG-33267))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-android-manifest.html))

Fixed

* The insert location of the `target_properties` call when creating
  template files
  ([QTCREATORBUG-33360](https://bugreports.qt.io/browse/QTCREATORBUG-33360))

### iOS

Fixed

* The deployment of applications built with a single-platform Qt version

### Remote Linux

Added

* Fast file access support for 32-bit ARM Linux devices
* The option to create kits for auto-detected device build tools:
  `Preferences > Devices > Create kits`
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-how-to-configure-tools-on-devices.html))

Changed

* Changed the option to run applications as root to run them as a different user:
  `Projects > Run Settings > Run as user`
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/creator-run-settings-remote-linux.html))

### Development Container

([Documentation](https://doc-snapshots.qt.io/qtcreator-19.0/devcontainer.html))

Added

* The configuration files to the project tree

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
Mike Jyu  
Mitch Curtis  
Olivier De Cannière  
Orgad Shaneh  
Pino Toscano  
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
