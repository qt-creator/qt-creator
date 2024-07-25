Qt Creator 14
=============

Qt Creator version 14 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=13.0..v14.0.0>

General
-------

* Started work on supporting Lua based plugins (registering language servers,
  actions, preferences, and wizards)
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-extending/lua-extensions.html))
* Added a mode for managing extensions
* Added `Clear` and `Save Contents` to context menus of all output views
* Fixed the tab focus order in the search toolbar
  ([QTCREATORBUG-30791](https://bugreports.qt.io/browse/QTCREATORBUG-30791))
* Locator
    * Added the option to show results relative to project root
      ([QTCREATORBUG-29462](https://bugreports.qt.io/browse/QTCREATORBUG-29462))

Editing
-------

* Changed the default behavior when files change on disk to
  `Reload All Unchanged Editors`
* Made the search options session-specific
  ([QTCREATORBUG-793](https://bugreports.qt.io/browse/QTCREATORBUG-793))
* Added menus with the navigation history to the back and forward buttons
  ([QTCREATORBUG-347](https://bugreports.qt.io/browse/QTCREATORBUG-347))
* Added a highlight for the current view in case of multiple views
  ([QTCREATORBUG-23654](https://bugreports.qt.io/browse/QTCREATORBUG-23654))
* Added `Window > Reopen Last Closed Document`
* Added the option to open files in a text editor that were detected as binary
  ([QTCREATORBUG-31116](https://bugreports.qt.io/browse/QTCREATORBUG-31116))
* Fixed that changing a document's MIME type by renaming did not re-open it in
  the new editor type when needed
  ([QTCREATORBUG-30317](https://bugreports.qt.io/browse/QTCREATORBUG-30317))
* Fixed that after hiding the editor in `Debug` mode, `Edit` mode always opened
  when opening documents, even if an external editor window was available
  ([QTCREATORBUG-30408](https://bugreports.qt.io/browse/QTCREATORBUG-30408))
* Fixed that it wasn't possible to open a file in the text editor if it was
  classified as a binary file format by the MIME database
  ([QTCREATORBUG-31116](https://bugreports.qt.io/browse/QTCREATORBUG-31116))
* Fixed the unindenting with backspace when multiple cursors are active
  ([QTCREATORBUG-31158](https://bugreports.qt.io/browse/QTCREATORBUG-31158))
* Fixed the `Save` shortcut after closing a document while the document's
  drop-down menu is shown
  ([QTCREATORBUG-31205](https://bugreports.qt.io/browse/QTCREATORBUG-31205))
* Fixed the highlighting of matches for `Whole Words Only` searches
  ([QTCREATORBUG-31020](https://bugreports.qt.io/browse/QTCREATORBUG-31020))
* Fixed that the documents chooser did not close when releasing `Ctrl` in
  some setups
  ([QTCREATORBUG-31228](https://bugreports.qt.io/browse/QTCREATORBUG-31228))

### C++

* Made C++ code model settings configurable per project
* Added a setting for the naming of include guards
  ([QTCREATORBUG-25117](https://bugreports.qt.io/browse/QTCREATORBUG-25117))
* Fixed indentation after function calls with subscript operator
  ([QTCREATORBUG-29225](https://bugreports.qt.io/browse/QTCREATORBUG-29225))
* Refactoring
    * Added `Convert Function Call to Qt Meta-Method Invocation`
      ([QTCREATORBUG-15972](https://bugreports.qt.io/browse/QTCREATORBUG-15972))
    * Added `Move Class to a Dedicated Set of Source Files`
      ([QTCREATORBUG-12190](https://bugreports.qt.io/browse/QTCREATORBUG-12190))
    * Added `Re-order Member Function Definitions According to Declaration Order`
      ([QTCREATORBUG-6199](https://bugreports.qt.io/browse/QTCREATORBUG-6199))
    * Added `Add Curly Braces` for do, while, and for loops
    * Fixed issues with macros
      ([QTCREATORBUG-10279](https://bugreports.qt.io/browse/QTCREATORBUG-10279))

    [Documentation](https://doc.qt.io/qtcreator/creator-reference-cpp-quick-fixes.html)

* Clangd
    * Updated the prebuilt binaries to LLVM 18.1.7
    * Increased the minimum version to LLVM 17
    * Added the `Per-project index location` and `Per-session index location`
      options in `Preferences` > `C++` > `Clangd` for setting the index location
      for a project or session
      ([QTCREATORBUG-27346](https://bugreports.qt.io/browse/QTCREATORBUG-27346))
    * Added the `Update dependent sources` option to make re-parsing source files
      while editing header files optional
      ([QTCREATORBUG-29943](https://bugreports.qt.io/browse/QTCREATORBUG-29943))
    * Fixed the handling of system headers
      ([QTCREATORBUG-30474](https://bugreports.qt.io/browse/QTCREATORBUG-30474))
    * Fixed a warning when adding an empty session to the list of sessions with
      a single Clangd instance
      ([QTCREATORBUG-31186](https://bugreports.qt.io/browse/QTCREATORBUG-31186))
* Built-in
    * Added the `Enable indexing` option in `Preferences` > `C++` > `Code Model`
      to turn off the built-in indexer
      ([QTCREATORBUG-29147](https://bugreports.qt.io/browse/QTCREATORBUG-29147))
    * Added the `Statement Macros` field in `Preferences` > `C++` > `Code Style`
      for macros that the indenter interprets as complete statements that don't
      require a semicolon at the end
      ([QTCREATORBUG-13640](https://bugreports.qt.io/browse/QTCREATORBUG-13640),
       [QTCREATORBUG-15069](https://bugreports.qt.io/browse/QTCREATORBUG-15069),
       [QTCREATORBUG-18789](https://bugreports.qt.io/browse/QTCREATORBUG-18789))
      ([Clang Format Documentation](https://clang.llvm.org/docs/ClangFormatStyleOptions.html#statementmacros))
    * Added indentation support for `try-catch` statements
      ([QTCREATORBUG-29452](https://bugreports.qt.io/browse/QTCREATORBUG-29452))

### QML

* Improved support for enums
  ([QTCREATORBUG-19226](https://bugreports.qt.io/browse/QTCREATORBUG-19226))
* Added `Qt Design Studio` to `Open With` for `.ui.qml` files
  ([Documentation](https://doc.qt.io/qtcreator/creator-quick-ui-forms.html))
* Fixed that the color preview did not work on named colors
  ([QTCREATORBUG-30594](https://bugreports.qt.io/browse/QTCREATORBUG-30594))
* Fixed the default path to `qmlformat` on Windows
  ([QTCREATORBUG-31257](https://bugreports.qt.io/browse/QTCREATORBUG-31257))
* Language Server
    * Switched on by default for Qt 6.8 and later
    * Added an option for generating `qmlls.ini` files for CMake projects in
      `Preferences` > `Qt Quick`> `QML/JS Editing`
      ([QTCREATORBUG-30394](https://bugreports.qt.io/browse/QTCREATORBUG-30394))
      ([Qt Documentation](https://doc.qt.io/qt-6/cmake-variable-qt-qml-generate-qmlls-ini.html))
    * Fixed that tool tips from the built-in model were shown instead of tool tips
      from the server

      [Documentation](https://doc.qt.io/qtcreator/creator-how-to-use-qml-language-server.html)

### Python

* Added options for updating Python Language Server
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-14.0/creator-language-servers.html))

### Language Server Protocol

* Added support for `SymbolTag`
  ([Protocol Documentation](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#symbolTag))
* Added support for type hierarchy
  ([QTCREATORBUG-28116](https://bugreports.qt.io/browse/QTCREATORBUG-28116))

### Compiler Explorer

* Added a wizard template for code that uses Qt to `File`> `New File`>
  `Compiler Explorer`
  [Documentation](https://doc.qt.io/qtcreator/creator-how-to-create-compiler-explorer-sessions.html)

### Markdown

* Fixed the navigation history

### Models

* Added more visual attributes for relations
* Added support for linked files in model element properties
* Added support for custom images in model element properties

([Documentation](https://doc-snapshots.qt.io/qtcreator-14.0/creator-how-to-create-models.html))

### SCXML

* Added visualization of conditions on transitions by using square brackets: `[]`
  ([QTCREATORBUG-21946](https://bugreports.qt.io/browse/QTCREATORBUG-21946))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-14.0/creator-scxml.html))

Projects
--------

* Added the `Hide Inactive Kits`/`Show All Kits` button to hide inactive kits
  from the list in the `Projects` mode
* Added support for user comments in the environment editor
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-14.0/creator-how-to-edit-environment-settings.html))
* Added the setting `Time to wait before force-stopping applications`
  ([QTCREATORBUG-31025](https://bugreports.qt.io/browse/QTCREATORBUG-31025))
* Fixed the parsing of file links when color was used for the output
  ([QTCREATORBUG-30774](https://bugreports.qt.io/browse/QTCREATORBUG-30774))
* Fixed that the column information was not used when opening files from links
  in issues
* Fixed changing the case of file names on case-insensitive file systems
  ([QTCREATORBUG-30846](https://bugreports.qt.io/browse/QTCREATORBUG-30846))
* Fixed that Qt Creator variables were not expanded for the `Copy File`
  deploy step
  ([QTCREATORBUG-30821](https://bugreports.qt.io/browse/QTCREATORBUG-30821))
* Fixed duplicate entries in the variable chooser for the build directory
  ([QTCREATORBUG-31183](https://bugreports.qt.io/browse/QTCREATORBUG-31183))

### CMake

* Made CMake settings configurable per project
* Implemented `Open Online Documentation` for CMake documentation
* Added `Clear CMake Configuration` to the context menu in the `Projects` view
  ([QTCREATORBUG-24658](https://bugreports.qt.io/browse/QTCREATORBUG-24658))
* Added support for the `CROSSCOMPILING_EMULATOR` target property
  ([QTCREATORBUG-29880](https://bugreports.qt.io/browse/QTCREATORBUG-29880))
  ([CMake Documentation](https://cmake.org/cmake/help/latest/prop_tgt/CROSSCOMPILING_EMULATOR.html#crosscompiling-emulator))
* Fixed that the package manager auto-setup files were not removed with
  `Clear CMake Configuration`
  ([QTCREATORBUG-30771](https://bugreports.qt.io/browse/QTCREATORBUG-30771))
* Fixed that files generated by the Qt QML CMake API were not filtered as
  generated files
  ([QTCREATORBUG-29631](https://bugreports.qt.io/browse/QTCREATORBUG-29631))
* Fixed a crash when triggering `Follow Symbol` in a CMake file that does not
  belong to a project
  ([QTCREATORBUG-31077](https://bugreports.qt.io/browse/QTCREATORBUG-31077))
* Fixed that multiple build configurations of the same type used the same
  build directory
  ([QTCREATORBUG-26066](https://bugreports.qt.io/browse/QTCREATORBUG-26066))
* Fixed an issue with adding new files when file globs are used in the CMake
  files
  ([QTCREATORBUG-30445](https://bugreports.qt.io/browse/QTCREATORBUG-30445))
* Fixed the default CMake generator for kits if Ninja is not installed
  ([QTCREATORBUG-31129](https://bugreports.qt.io/browse/QTCREATORBUG-31129))
* Fixed that duplicate `CMAKE_CXX_COMPILER` entries could be used
  ([QTCREATORBUG-27005](https://bugreports.qt.io/browse/QTCREATORBUG-27005))
* Presets
    * Made CMake settings configurable
      ([QTCREATORBUG-25972](https://bugreports.qt.io/browse/QTCREATORBUG-25972),
       [QTCREATORBUG-29559](https://bugreports.qt.io/browse/QTCREATORBUG-29559),
       [QTCREATORBUG-30385](https://bugreports.qt.io/browse/QTCREATORBUG-30385))
    * Made it possible to register debuggers
      ([QTCREATORBUG-30836](https://bugreports.qt.io/browse/QTCREATORBUG-30836))
    * Added support for custom build types
      ([QTCREATORBUG-30014](https://bugreports.qt.io/browse/QTCREATORBUG-30014))
    * Fixed issues with cross-compilation setups
      ([QTCREATORBUG-31249](https://bugreports.qt.io/browse/QTCREATORBUG-31249))

    ([Documentation](https://doc-snapshots.qt.io/qtcreator-14.0/creator-build-settings-cmake-presets.html))

### Workspace

* Added `File > Open Workspace` for opening a directory as a project. A project
  file `.qtcreator/project.json` in the directory is used to set a name and
  file exclusion filters.

Debugging
---------

### C++

* Improved performance
* GDB
    * Added a setting for `debuginfod`
      ([QTCREATORBUG-28868](https://bugreports.qt.io/browse/QTCREATORBUG-28868))
* CDB
    * Fixed the display type of `HRESULT`
      ([QTCREATORBUG-30574](https://bugreports.qt.io/browse/QTCREATORBUG-30574))

Analyzer
--------

### Clang

* Added the option to `Suppress Diagnostics Inline`
  ([QTCREATORBUG-24847](https://bugreports.qt.io/browse/QTCREATORBUG-24847))
  ([Clazy Documentation](https://github.com/KDE/clazy?tab=readme-ov-file#reducing-warning-noise))
  ([clang-tidy Documentation](https://clang.llvm.org/extra/clang-tidy/#suppressing-undesired-diagnostics))

### Axivion

* Added the `Add` and `Remove` buttons to `Preferences` > `Axivion` for
  registering multiple servers
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-14.0/creator-preferences-axivion.html))

### Cppcheck

* Fixed that Cppcheck was not working until selecting `Apply` in the settings
  ([QTCREATORBUG-28951](https://bugreports.qt.io/browse/QTCREATORBUG-28951),
   [QTCREATORBUG-30615](https://bugreports.qt.io/browse/QTCREATORBUG-30615))

Terminal
--------

* Fixed resizing on Windows
  ([QTCREATORBUG-30558](https://bugreports.qt.io/browse/QTCREATORBUG-30558))

Version Control Systems
-----------------------

### Git

* Fixed that email and author mapping was not used for logs and showing changes

Test Integration
----------------

* Made the test timeout optional in `Preferences` > `Testing` > `General`
  ([QTCREATORBUG-30668](https://bugreports.qt.io/browse/QTCREATORBUG-30668))
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-14.0/creator-preferences-testing.html))
* Added the project specific `Limit Files to Path Patterns` option in `Projects` >
  `Project Settings` > `Testing` for restricting the search for tests
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-14.0/creator-how-to-limit-files-to-path-patterns.html))

### Qt Test

* Fixed the order of test execution
  ([QTCREATORBUG-30670](https://bugreports.qt.io/browse/QTCREATORBUG-30670))

Platforms
---------

### Linux

* Adapted the default theme to the system theme
* Fixed issues with light themes on dark system themes
  ([QTCREATORBUG-18281](https://bugreports.qt.io/browse/QTCREATORBUG-18281),
   [QTCREATORBUG-20889](https://bugreports.qt.io/browse/QTCREATORBUG-20889),
   [QTCREATORBUG-26817](https://bugreports.qt.io/browse/QTCREATORBUG-26817),
   [QTCREATORBUG-28589](https://bugreports.qt.io/browse/QTCREATORBUG-28589),
   [QTCREATORBUG-30138](https://bugreports.qt.io/browse/QTCREATORBUG-30138))
* Fixed that recent projects on unavailable remote devices regularly
  froze Qt Creator
  ([QTCREATORBUG-30681](https://bugreports.qt.io/browse/QTCREATORBUG-30681))

### Android

* Added support for creating `android-desktop` devices in `Preferences` >
  `Devices` > `Add` > `Android Device` > `Device definition`
* Added support for `namespace` in `build.gradle`
  ([QTBUG-106907](https://bugreports.qt.io/browse/QTBUG-106907))
* Fixed that errors when creating AVDs were not visible to the user
  ([QTCREATORBUG-30852](https://bugreports.qt.io/browse/QTCREATORBUG-30852))

### iOS

* Removed Simulator management from the preferences. Use the
  `Devices and Simulators` window in Xcode instead.
* Fixed that starting the debugger could be slow for iOS < 17
  ([QTCREATORBUG-31044](https://bugreports.qt.io/browse/QTCREATORBUG-31044))

### Remote Linux

* Added the `Use SSH port forwarding for debugging` option in `Preferences` >
  `Devices` for a `Remote Linux Device`
* Improved the performance of the generic deployment method
* Fixed that the file size check that is performed before parsing C++ files
  could freeze Qt Creator until finished for remote projects
* Fixed that the deployment of a directory deployed only its contents instead
  of the directory itself
  ([QTCREATORBUG-31136](https://bugreports.qt.io/browse/QTCREATORBUG-31136))

### Qt Application Manager

* Added support for the `perf` profiler

### Bare Metal

* Fixed issues with Qbs and the IAR toolchain
  ([QTCREATORBUG-24040](https://bugreports.qt.io/browse/QTCREATORBUG-24040))

Credits for these changes go to:
--------------------------------
Ahmad Samir  
Aleksei German  
Alessandro Portale  
Alexander Drozdov  
Alexandre Laurent  
Ali Kianian  
André Hartmann  
André Pönitz  
Artem Sokolovskii  
Assam Boudjelthia  
BogDan Vatra  
Brook Cronin  
Burak Hancerli  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Dominik Holland  
Eike Ziller  
Esa Törmänen  
Henning Gruendl  
Jaroslaw Kobus  
Jiajie Chen  
Jochen Becher  
Johanna Vanhatapio  
Jussi Witick  
Knud Dollereder  
Leena Miettinen  
Mahmoud Badri  
Marco Bubke  
Marcus Tillmanns  
Mathias Hasselmann  
Mats Honkamaa  
Michael Weghorn  
Miikka Heikkinen  
Orgad Shaneh  
Pino Toscano  
Pranta Dastider  
Ralf Habacker  
Robert Löhning  
Sami Shalayel  
Semih Yavuz  
Sergey Silin  
Shrief Gabr  
Teea Poldsam  
Thiago Macieira  
Thomas Hartmann  
Tim Jenßen  
Vikas Pachdha  
Xavier Besson  
