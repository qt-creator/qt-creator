Qt Creator 15
=============

Qt Creator version 15 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=14.0..v15.0.0>

General
-------

* Changed the default MIME types to the Apache Tika MIME type database
* Added the option to hide modes (`View > Modes` and the context menu on the
  mode selector)
* Added the option to exclude binary files in `Search Results`
  ([QTCREATORBUG-1756](https://bugreports.qt.io/browse/QTCREATORBUG-1756))
* Added new dark and light themes
  ([Blog Post](https://www.qt.io/blog/review-new-themes-for-qt-creator))
* Plugins
    * Added `DocumentationUrl` and new mandatory fields `Id` and `VendorId`
      to the plugin meta data
    * Changed the plugin descriptions to Markdown in
      `Help > About Plugins > Details` and `Extensions` mode
    * Extended the API available to Lua plugins
      ([Documentation](https://doc-snapshots.qt.io/qtcreator-extending/lua-extensions.html))
* Prebuilt binaries
    * Added opt-in crash reporting to
      `Edit > Preferences > Environment > System`

Editing
-------

* Added actions for opening the next and previous documents from the
  `Open Documents` view
  ([QTCREATORBUG-1208](https://bugreports.qt.io/browse/QTCREATORBUG-1208))
* Added actions for moving and sorting bookmarks in the `Bookmarks` view
  ([QTCREATORBUG-30286](https://bugreports.qt.io/browse/QTCREATORBUG-30286))
* Added the folding actions to the context menu on the folding marks
  ([QTCREATORBUG-7461](https://bugreports.qt.io/browse/QTCREATORBUG-7461))
* Added `Fold Recursively` and `Unfold Recursively`
* Fixed the display of multi-line annotations
  ([QTCREATORBUG-29951](https://bugreports.qt.io/browse/QTCREATORBUG-29951))

### C++

* Added a syntax highlighting style for C++ attributes
* Merged the `C` and `C++` compiler categories
  ([QTCREATORBUG-31132](https://bugreports.qt.io/browse/QTCREATORBUG-31132),
   [QTCREATORBUG-30630](https://bugreports.qt.io/browse/QTCREATORBUG-30630))
* Fixed that `Clang` was preferred over `GCC` on Linux when automatically
  assigning toolchains to kits
  ([QTCREATORBUG-29913](https://bugreports.qt.io/browse/QTCREATORBUG-29913))
* Fixed that include statements could be added before `#pragma once`
  ([QTCREATORBUG-30808](https://bugreports.qt.io/browse/QTCREATORBUG-30808))
* Fixed that symbol locations could be mixed up between different open projects
  ([QTCREATORBUG-19636](https://bugreports.qt.io/browse/QTCREATORBUG-19636))
* Fixed issues with code folding and `ifdef` statements
  ([QTCREATORBUG-21064](https://bugreports.qt.io/browse/QTCREATORBUG-21064))
* Fixed the indentation after multi-line comments
  ([QTCREATORBUG-31256](https://bugreports.qt.io/browse/QTCREATORBUG-31256))
* Fixed the display of annotations with array operators
  ([QTCREATORBUG-31670](https://bugreports.qt.io/browse/QTCREATORBUG-31670))
* ClangFormat
    * Implemented `Export` and `Import` for the settings

### QML

* Moved the option for creating Qt Design Studio compatible projects from the
  `Qt Quick Application` to the `Qt Quick UI Prototype` wizard
  ([QTCREATORBUG-31355](https://bugreports.qt.io/browse/QTCREATORBUG-31355),
   [QTCREATORBUG-31657](https://bugreports.qt.io/browse/QTCREATORBUG-31657))
* Fixed the indentation of files created by `Move Component into Separate File`
  ([QTCREATORBUG-31084](https://bugreports.qt.io/browse/QTCREATORBUG-31084))

### Language Server Protocol

* Fixed that global environment changes were not applied to language servers

### Copilot

* Fixed the application of multi-line suggestions
  ([QTCREATORBUG-31418](https://bugreports.qt.io/browse/QTCREATORBUG-31418))

### Compiler Explorer

* Added the option to change the server URL
  ([QTCREATORBUG-31261](https://bugreports.qt.io/browse/QTCREATORBUG-31261))
  ([Documentation](https://doc.qt.io/qtcreator/creator-how-to-explore-compiler-code.html))

### Lua

* Added an interactive shell (REPL) as an output view

### SCXML

* Made names and conditions movable
  ([QTCREATORBUG-31397](https://bugreports.qt.io/browse/QTCREATORBUG-31397))
* Fixed that the colors didn't follow the theme
  ([QTCREATORBUG-29701](https://bugreports.qt.io/browse/QTCREATORBUG-29701))

### Binary Files

* Fixed searching text when a codec is set
  ([QTCREATORBUG-30589](https://bugreports.qt.io/browse/QTCREATORBUG-30589))

Projects
--------

* Removed the Qt Linguist related external tool items, which did not work for
  CMake
  ([QTCREATORBUG-28467](https://bugreports.qt.io/browse/QTCREATORBUG-28467))
* Improved the performance of compile and application output parsing and added
  the option to `Discard excessive output`
  ([QTCREATORBUG-30135](https://bugreports.qt.io/browse/QTCREATORBUG-30135),
   [QTCREATORBUG-31449](https://bugreports.qt.io/browse/QTCREATORBUG-31449))
* Improved the display of toolchains in the kit options by sorting and showing
  issue icons
* Improved the warning against editing files that are not part of the project
  ([QTCREATORBUG-31542](https://bugreports.qt.io/browse/QTCREATORBUG-31542))
* Added `Create Header File` and `Create Source File` to the context menu
  for source files and header files in the project tree respectively
  ([QTCREATORBUG-24575](https://bugreports.qt.io/browse/QTCREATORBUG-24575))
* Added the option to deploy dependent projects
  ([QTCREATORBUG-27406](https://bugreports.qt.io/browse/QTCREATORBUG-27406))
* Added the option to run build and deploy steps as root user
  ([QTCREATORBUG-31012](https://bugreports.qt.io/browse/QTCREATORBUG-31012))
* Added `Copy Contents to Scratch Buffer` to the context menu of output views
  ([QTCREATORBUG-31144](https://bugreports.qt.io/browse/QTCREATORBUG-31144))
* Added the list of open files to the session overview, for sessions that
  do not have any projects
  ([QTCREATORBUG-7660](https://bugreports.qt.io/browse/QTCREATORBUG-7660))
* Added the project names to the `Recent Projects` menu
  ([QTCREATORBUG-31753](https://bugreports.qt.io/browse/QTCREATORBUG-31753))
* Fixed issues with creating subprojects for non-active projects
* Fixed that the wizards were asking for a build system type when creating
  subprojects
  ([QTCREATORBUG-30281](https://bugreports.qt.io/browse/QTCREATORBUG-30281))
* Fixed that new build configurations could use the wrong build directory
  ([QTCREATORBUG-31470](https://bugreports.qt.io/browse/QTCREATORBUG-31470))
* Fixed that temporary kits could stay around
  ([QTCREATORBUG-31461](https://bugreports.qt.io/browse/QTCREATORBUG-31461))
* Fixed that Qt Creator could freeze while trying to stop a user application
  ([QTCREATORBUG-31319](https://bugreports.qt.io/browse/QTCREATORBUG-31319))
* Fixed that the application exit code was not shown after grazefully
  terminating it
  ([QTCREATORBUG-31141](https://bugreports.qt.io/browse/QTCREATORBUG-31141))
* Fixed a focus issue when renaming files
  ([QTCREATORBUG-30926](https://bugreports.qt.io/browse/QTCREATORBUG-30926))

### CMake

* Implemented `New Subproject` for CMake projects
  ([QTCREATORBUG-30471](https://bugreports.qt.io/browse/QTCREATORBUG-30471),
   [QTCREATORBUG-30818](https://bugreports.qt.io/browse/QTCREATORBUG-30818))
* Added support for the `FOLDER` property of targets
  ([QTCREATORBUG-28873](https://bugreports.qt.io/browse/QTCREATORBUG-28873))
* Added groups for resources when `Package manager auto setup` is enabled
  (opt-out)
  ([QTCREATORBUG-31308](https://bugreports.qt.io/browse/QTCREATORBUG-31308),
   [QTCREATORBUG-31312](https://bugreports.qt.io/browse/QTCREATORBUG-31312))
* Added `CMakeLists.txt` items to targets in the `Projects` tree, to the point
  where they are defined
  ([QTCREATORBUG-31362](https://bugreports.qt.io/browse/QTCREATORBUG-31362))
* Added `Open...` to CMake folders in the `Projects` tree
  ([QTCREATORBUG-31362](https://bugreports.qt.io/browse/QTCREATORBUG-31362))
* Added `Build`, `Rebuild`, and `Clean` operations for subprojects to the
  `Build` menu and `Projects` tree
  ([QTCREATORBUG-27588](https://bugreports.qt.io/browse/QTCREATORBUG-27588))
* Added the parsing of `AUTOMOC` and `AUTOUIC` warnings and errors
  ([QTCREATORBUG-29345](https://bugreports.qt.io/browse/QTCREATORBUG-29345),
   [QTCREATORBUG-31597](https://bugreports.qt.io/browse/QTCREATORBUG-31597))
* Added the option of opening `CMakeCache.txt` to open the project
  ([QTCREATORBUG-24439](https://bugreports.qt.io/browse/QTCREATORBUG-24439),
   [QTCREATORBUG-30507](https://bugreports.qt.io/browse/QTCREATORBUG-30507))
* Fixed the option `Build Only the Application to Be Run` for the
  `Build before deploying` preferences
  ([QTCREATORBUG-31416](https://bugreports.qt.io/browse/QTCREATORBUG-31416))

### Workspace

* Added the option to add build configurations
* Added automatic updating of the project tree
* Fixed that cloned run configurations were not editable

### vcpkg

* Fixed the detection of `VCPKG_TARGET_TRIPLET` on ARM macOS

Debugging
---------

* Added `Disable All Breakpoints` to the debugger toolbar
* Fixed that the port range for debugging was not customizable for the
  desktop
  ([QTCREATORBUG-31406](https://bugreports.qt.io/browse/QTCREATORBUG-31406))
* Fixed the unfolding of items in the debugger tooltip
  ([QTCREATORBUG-31250](https://bugreports.qt.io/browse/QTCREATORBUG-31250))
* Pretty printers
    * Added pretty printers for `std:dequeue` and `std::forward_list`
      ([QTCREATORBUG-29994](https://bugreports.qt.io/browse/QTCREATORBUG-29994))
    * Fixed various standard library types
    * Fixed issues with `_GLIBCXX_DEBUG` enabled
      ([QTCREATORBUG-20476](https://bugreports.qt.io/browse/QTCREATORBUG-20476))

### C++

* Added `Load Last Core File`
  ([QTCREATORBUG-29256](https://bugreports.qt.io/browse/QTCREATORBUG-29256))
* Fixed the display of 64-bit `QFlags`

Analyzer
--------

### Clang

* Fixed issues with the compiler options by using the compilation database
  ([QTCREATORBUG-29529](https://bugreports.qt.io/browse/QTCREATORBUG-29529))

### Axivion

* Moved the Axivion views to a Debug perspective
* Improved the handling of invalid filters
* Removed the linking between Dashboard and a project. Now, the Dashboard
  has to be selected on the Axivion issues view.
* Added the option to define path mappings
  (`Preferences > Axivion > Path Mapping`)
* Added a tool button for `Show issue markers inline`
* Added column sorting to the list of issues
* Added a `Reload` button

Version Control Systems
-----------------------

* Improved the error dialog when adding files fails
  ([QTCREATORBUG-31161](https://bugreports.qt.io/browse/QTCREATORBUG-31161))
* Added `Log Directory` to the context menu of the `Projects` tree

### Git

* Added actions for blame at the revision, blame of the parent, the file
  from the revision, and the log for the line to the tooltip for `Instant Blame`
  ([Documentation](https://doc-snapshots.qt.io/qtcreator-15.0/creator-vcs-git.html#using-instant-blame))
* Added visual indications that files are modified to the `Projects` view
  ([QTCREATORBUG-8857](https://bugreports.qt.io/browse/QTCREATORBUG-8857))
* Added the option to include all local branches in the log
* Gerrit
    * Fixed the support for pushing patches with topics
      ([QTCREATORBUG-31411](https://bugreports.qt.io/browse/QTCREATORBUG-31411))

Test Integration
----------------

* Added test duration information for test frameworks that support it
  ([QTCREATORBUG-31242](https://bugreports.qt.io/browse/QTCREATORBUG-31242))

Extension Manager
-----------------

* Added filters and sorting
  ([QTCREATORBUG-31179](https://bugreports.qt.io/browse/QTCREATORBUG-31179))
* Added a button for opening the preferences

Platforms
---------

### macOS

* Added support for back and forward gestures
  ([QTCREATORBUG-7387](https://bugreports.qt.io/browse/QTCREATORBUG-7387))

### Android

* Improved the responsiveness of Qt Creator during Android related operations
* Fixed that the setup wizard could use the wrong NDK and build tools version
  ([QTCREATORBUG-31311](https://bugreports.qt.io/browse/QTCREATORBUG-31311))

### iOS

* Improved the error messages when installation on the Simulator fails
  ([QTCREATORBUG-25833](https://bugreports.qt.io/browse/QTCREATORBUG-25833))
* Fixed issues when multiple devices are open in the Simulator

### Docker

* Improved the performance of operations on the device by deploying
  a helper application (GoCmdBridge)

### WebAssembly

* Improved error messages on registration failure
  ([QTCREATORBUG-30057](https://bugreports.qt.io/browse/QTCREATORBUG-30057))

### VxWorks

* Added support for VxWorks 24.03

Credits for these changes go to:
--------------------------------
Aleksei German  
Alessandro Portale  
Alexandru Croitor  
Ali Kianian  
Andre Hartmann  
André Pönitz  
Andrii Semkiv  
Artem Sokolovskii  
Artur Twardy  
Assam Boudjelthia  
Audun Sutterud  
Burak Hancerli  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Dominik Holland  
Eike Ziller  
Francisco Boni  
Friedemann Kleint  
Henning Gruendl  
Jaroslaw Kobus  
Jussi Witick  
Justyna Hudziak  
Karim Pinter  
Knud Dollereder  
Kwangsub Kim  
Leena Miettinen  
Liu Zhangjian  
Lukasz Papierkowski  
Mahmoud Badri  
Marc Mutz  
Marco Bubke  
Marcus Tillmanns  
Mariusz Szczepanik  
Mathias Hasselmann  
Mats Honkamaa  
Mehdi Salem  
Miikka Heikkinen  
Orgad Shaneh  
Pino Toscano  
Pranta Dastider  
Przemyslaw Lewandowski  
Rauno Pennanen  
Renaud Guezennec  
Robert Löhning  
Sami Shalayel  
Semih Yavuz  
Shrief Gabr  
Sivert Krøvel  
styxer  
Teea Poldsam  
Thiago Macieira  
Thomas Hartmann  
Tim Jenßen  
Ulf Hermann  
Vikas Pachdha  
Xavier Besson  
Zoltan Gera  
