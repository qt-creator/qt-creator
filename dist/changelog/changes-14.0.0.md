Qt Creator 14
=============

Qt Creator version 14 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/13.0..v14.0.0

General
-------

* Started work on supporting Lua based plugins (registering language servers,
  actions, preferences, and wizards)
* Added `Clear` and `Save Contents` to context menus of all output views
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
* Fixed that changing a document's MIME type by renaming did not re-open it in
  the new editor type when needed
  ([QTCREATORBUG-30317](https://bugreports.qt.io/browse/QTCREATORBUG-30317))
* Fixed that after hiding the editor in `Debug` mode, `Edit` mode always opened
  when opening documents, even if an external editor window was available
  ([QTCREATORBUG-30408](https://bugreports.qt.io/browse/QTCREATORBUG-30408))

### C++

* Made C++ code model settings configurable per project
* Fixed indentation after function calls with subscript operator
  ([QTCREATORBUG-29225](https://bugreports.qt.io/browse/QTCREATORBUG-29225))
* Refactoring
    * Added `Convert Function Call to Qt Meta-Method Invocation`
      ([QTCREATORBUG-15972](https://bugreports.qt.io/browse/QTCREATORBUG-15972))
    * Added `Move Class to a Dedicated Set of Source Files`
      ([QTCREATORBUG-12190](https://bugreports.qt.io/browse/QTCREATORBUG-12190))
    * Added `Re-order Member Function Definitions According to Declaration Order`
      ([QTCREATORBUG-6199](https://bugreports.qt.io/browse/QTCREATORBUG-6199))
    * Added triggers for `Add Curly Braces`
    * Fixed issues with macros
      ([QTCREATORBUG-10279](https://bugreports.qt.io/browse/QTCREATORBUG-10279))
* Clangd
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
* Added `Qt Design Studio` to `Open With` for `.ui.qml`-files
* Language Server
    * Switched on by default
    * Added option for generating `qmlls.ini` files for CMake projects
      ([QTCREATORBUG-30394](https://bugreports.qt.io/browse/QTCREATORBUG-30394))
      ([Qt Documentation](https://doc.qt.io/qt-6/cmake-variable-qt-qml-generate-qmlls-ini.html))
    * Fixed that tool tips from the built-in model were shown instead of tool tips
      from the server

### Python

* Added the option to install Python LSP updates

### Language Server Protocol

* Added support for `SymbolTag`
  ([Protocol Documentation](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#symbolTag))
* Added support for type hierarchy
  ([QTCREATORBUG-28116](https://bugreports.qt.io/browse/QTCREATORBUG-28116))

### Compiler Explorer

* Added a wizard template for code that uses Qt

### Models

* Added more visual attributes for relations
* Added support for linked files
* Added support for custom images

### SCXML

* Added visualization of conditions on transitions
  ([QTCREATORBUG-21946](https://bugreports.qt.io/browse/QTCREATORBUG-21946))

Projects
--------

* Added the option to hide all unconfigured kits from the list in `Projects`
  mode
* Added support for user comments in the environment editor
* Fixed the parsing of file links when color was used for the output
  ([QTCREATORBUG-30774](https://bugreports.qt.io/browse/QTCREATORBUG-30774))
* Fixed that the column information was not used when opening files from links
  in issues
* Fixed changing the case of file names on case-insensitive file systems
  ([QTCREATORBUG-30846](https://bugreports.qt.io/browse/QTCREATORBUG-30846))
* Fixed that Qt Creator variables were not expanded for the `Copy File`
  deploy step
  ([QTCREATORBUG-30821](https://bugreports.qt.io/browse/QTCREATORBUG-30821))

### CMake

* Made CMake settings configurable per project
* Implemented `Open Online Documentation` for CMake documentation
* Added `Clear CMake Configuration` to the context menu in the Projects view
  ([QTCREATORBUG-24658](https://bugreports.qt.io/browse/QTCREATORBUG-24658))
* Fixed that the package manager auto-setup files were not removed with
  `Clear CMake Configuration`
  ([QTCREATORBUG-30771](https://bugreports.qt.io/browse/QTCREATORBUG-30771))
* Fixed that files generated by the Qt QML CMake API were not filtered as
  generated files
  ([QTCREATORBUG-29631](https://bugreports.qt.io/browse/QTCREATORBUG-29631))
* Presets
    * Made CMake settings configurable
      ([QTCREATORBUG-25972](https://bugreports.qt.io/browse/QTCREATORBUG-25972),
       [QTCREATORBUG-29559](https://bugreports.qt.io/browse/QTCREATORBUG-29559),
       [QTCREATORBUG-30385](https://bugreports.qt.io/browse/QTCREATORBUG-30385))
    * Made it possible to register debuggers
    * Added support for custom build types

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

* Made it possible to register multiple servers

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

* Made the test timeout optional
  ([QTCREATORBUG-30668](https://bugreports.qt.io/browse/QTCREATORBUG-30668))
* Added a project specific option `Limit Files to Path Patterns` for restricting
  the search for tests

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

* Added support for creating `android-desktop` devices
* Added support for `namespace` in `build.gradle`
  ([QTBUG-106907](https://bugreports.qt.io/browse/QTBUG-106907))

### iOS

* Removed Simulator management from the preferences. Use the
  `Devices and Simulators` window in Xcode instead.

### Remote Linux

* Added the option to use SSH port forwarding for debugging
* Improved the performance of the generic deployment method
* Fixed that the file size check that is performed before parsing C++ files
  could freeze Qt Creator until finished for remote projects

### Qt Application Manager

* Added support for the `perf` profiler

Credits for these changes go to:
--------------------------------
Ahmad Samir  
Aleksei German  
Alessandro Portale  
Alexander Drozdov  
Ali Kianian  
Andre Hartmann  
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
Pranta Dastider  
Robert Löhning  
Sami Shalayel  
Sergey Silin  
Shrief Gabr  
Teea Poldsam  
Thiago Macieira  
Thomas Hartmann  
Tim Jenßen  
Vikas Pachdha  
Xavier Besson  
