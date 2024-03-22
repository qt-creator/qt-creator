Qt Creator 13
=============

Qt Creator version 13 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/12.0..v13.0.0

New plugins
-----------

### Qt Application Manager

Adds support for Qt 6 based applications with CMake for creating, building,
deploying, running, and debugging for devices that use the
[Qt Application Manager](https://doc.qt.io/QtApplicationManager/).

([Documentation](https://doc.qt.io/qtcreator/creator-how-to-run-in-app-manager.html))

General
-------

* Updated the visual style of Welcome mode
* Improved docking (Debug mode, Widget Designer)
    * Fixed the style of titles and changed them to always be visible
      (removed `View > Views > Automatically Hide View Titlebars`)
    * Added an option to collapse panels
    * Changed `Hide/Show Left/Right Sidebar` to hide and show the corresponding
      dock area
* Added the option to show file paths relative to the active project to the
  search results view
  ([QTCREATORBUG-29462](https://bugreports.qt.io/browse/QTCREATORBUG-29462))
* Added a `Current` button for selecting the directory of the current document
  for searching in `Files in File System`
* Added `Copy to Clipboard` to the `About Qt Creator` dialog
  ([QTCREATORBUG-29886](https://bugreports.qt.io/browse/QTCREATORBUG-29886))
* Fixed issues with the window actions
  ([QTCREATORBUG-30381](https://bugreports.qt.io/browse/QTCREATORBUG-30381))
* Fixed drag & drop for external tools
  ([QTCREATORBUG-30469](https://bugreports.qt.io/browse/QTCREATORBUG-30469))
* Known Issues
    * Installers provided by The Qt Company mostly display English text, not
      the system's language
      ([QTIFW-3310](https://bugreports.qt.io/browse/QTIFW-3310))

Help
----

* Fixed that non-Qt related help files could automatically be registered
* Fixed that the option `Highest Version Only` for automatically registering Qt
  documentation did not work for the Qt Online Installer

Editing
-------

* Fixed that `Shift+Tab` did not always unindent
  ([QTCREATORBUG-29742](https://bugreports.qt.io/browse/QTCREATORBUG-29742))
* Fixed that `Surround text selection with brackets` did nothing for `<`
* Fixed following links without a file name in documents without a file name
* Fixed that device tree source files (`.dts`) were opened in the binary editor
  ([QTCREATORBUG-19029](https://bugreports.qt.io/browse/QTCREATORBUG-19029))

### C++

* Added the `Move Definition Here` refactoring action that moves an existing
  function definition to its declaration
  ([QTCREATORBUG-9515](https://bugreports.qt.io/browse/QTCREATORBUG-9515))
* Added the `Enclose in QByteArrayLiteral` refactoring action
  ([QTCREATORBUG-12995](https://bugreports.qt.io/browse/QTCREATORBUG-12995))
* Enabled the completion inside comments and strings by falling back to the
  built-in code model
  ([QTCREATORBUG-20828](https://bugreports.qt.io/browse/QTCREATORBUG-20828))
* Improved the position of headers inserted by refactoring operations
  ([QTCREATORBUG-21826](https://bugreports.qt.io/browse/QTCREATORBUG-21826))
* Improved the coding style settings by separating Clang Format and other coding
  style settings, and using a plain text editor for custom Clang Format settings
  ([Documentation](https://doc.qt.io/qtcreator/creator-preferences-cpp-code-style.html))
* Fixed that the class wizards used the class name for the include guard
  instead of the file name
  ([QTCREATORBUG-30140](https://bugreports.qt.io/browse/QTCREATORBUG-30140))
* Fixed that renaming classes did not change the include directive for the
  renamed header in the source file
  ([QTCREATORBUG-30154](https://bugreports.qt.io/browse/QTCREATORBUG-30154))
* Fixed issues with refactoring template functions
  ([QTCREATORBUG-29408](https://bugreports.qt.io/browse/QTCREATORBUG-29408))
* Fixed the `Add Definition` refactoring action for member functions of a
  template class in a namespace
  ([QTCREATORBUG-22076](https://bugreports.qt.io/browse/QTCREATORBUG-22076))
* Clangd
    * Improved the function hint tool tip
      ([QTCREATORBUG-26346](https://bugreports.qt.io/browse/QTCREATORBUG-26346),
       [QTCREATORBUG-30489](https://bugreports.qt.io/browse/QTCREATORBUG-30489))
    * Fixed that `Follow Symbol Under Cursor` only worked for exact matches
      ([QTCREATORBUG-29814](https://bugreports.qt.io/browse/QTCREATORBUG-29814))
    * Fixed the version check for remote `clangd` executables
      ([QTCREATORBUG-30374](https://bugreports.qt.io/browse/QTCREATORBUG-30374))

### QML

* Added navigation from QML components to the C++ code in the project
  ([QTCREATORBUG-28086](https://bugreports.qt.io/browse/QTCREATORBUG-28086))
* Added a button for launching the QML Preview on the current document to
  the editor tool bar
* Added color previews when hovering Qt color functions
  ([QTCREATORBUG-29966](https://bugreports.qt.io/browse/QTCREATORBUG-29966))

### Python

* Fixed that global and virtual environments were polluted with `pylsp` and
  `debugpy` installations

### Language Server Protocol

* Added automatic setup up of language servers for `YAML`, `JSON`, and `Bash`
  (requires `npm`)
  ([Documentation](https://doc.qt.io/qtcreator/creator-language-servers.html#adding-language-servers))

### Widget Designer

* Fixed the indentation of the code that is inserted by `Go to slot`
  ([QTCREATORBUG-11730](https://bugreports.qt.io/browse/QTCREATORBUG-11730))

### Compiler Explorer

* Added highlighting of the matching source lines when hovering over the
  assembly

### Markdown

* Added the common text editor tools (line and column, encoding, and line
endings) to the tool bar
* Added support for following links to the text editor

### Binary Files

* Fixed issues with large addresses

### Models

* Fixed a crash when selecting items
  ([QTCREATORBUG-30413](https://bugreports.qt.io/browse/QTCREATORBUG-30413))

Projects
--------

* Added a section `Vanished Targets` to `Projects` mode in case the project
  was configured for kits that have vanished, as a replacement for the automatic
  creation of "Replacement" kits
  ([Documentation](https://doc.qt.io/qtcreator/creator-how-to-activate-kits.html#copy-custom-settings-from-vanished-targets))
* Added the status of devices to the device lists
  ([QTCREATORBUG-20941](https://bugreports.qt.io/browse/QTCREATORBUG-20941))
* Added the `Preferences > Build & Run > General > Application environment`
  option for globally modifying the environment for all run configurations
  ([QTCREATORBUG-29530](https://bugreports.qt.io/browse/QTCREATORBUG-29530))
* Added a file wizard for Qt translation (`.ts`) files
  ([QTCREATORBUG-29775](https://bugreports.qt.io/browse/QTCREATORBUG-29775))
* Added an optional warning for special characters in build directories
  ([QTCREATORBUG-20834](https://bugreports.qt.io/browse/QTCREATORBUG-20834))
* Improved the environment settings by making the changes explicit in a
  separate, text-based editor
* Increased the maximum width of the target selector
  ([QTCREATORBUG-30038](https://bugreports.qt.io/browse/QTCREATORBUG-30038))
* Fixed that the `Left` cursor key did not always collapse the current item
  ([QTBUG-118515](https://bugreports.qt.io/browse/QTBUG-118515))
* Fixed inconsistent folder hierarchies in the project tree
  ([QTCREATORBUG-29923](https://bugreports.qt.io/browse/QTCREATORBUG-29923))

### CMake

* Added support for custom output parsers for the configuration of projects
  ([QTCREATORBUG-29992](https://bugreports.qt.io/browse/QTCREATORBUG-29992))
* Made cache variables available even if project configuration failed
* Fixed that too many paths were added to the build library search path
  ([QTCREATORBUG-29662](https://bugreports.qt.io/browse/QTCREATORBUG-29662))
* Fixed that searching in the project included results from module files
  not in the project
  ([QTCREATORBUG-30372](https://bugreports.qt.io/browse/QTCREATORBUG-30372))
* Fixed that `Follow Symbol` on `add_subdirectory` could jump to a target of
  the same name
  ([QTCREATORBUG-30510](https://bugreports.qt.io/browse/QTCREATORBUG-30510))
* CMake Presets
    * Fixed `Reload CMake Presets` if the project was not configured yet
      ([QTCREATORBUG-30238](https://bugreports.qt.io/browse/QTCREATORBUG-30238))
    * Fixed that kits were accumulating on the project setup page
      ([QTCREATORBUG-29535](https://bugreports.qt.io/browse/QTCREATORBUG-29535))
    * Fixed that `binaryDir` was not handled for all presets
      ([QTCREATORBUG-30236](https://bugreports.qt.io/browse/QTCREATORBUG-30236))
    * Fixed a freeze with nested presets
      ([QTCREATORBUG-30288](https://bugreports.qt.io/browse/QTCREATORBUG-30288))
    * Fixed a wrong error message
      ([QTCREATORBUG-30373](https://bugreports.qt.io/browse/QTCREATORBUG-30373))
    * Fixed a crash when no CMake tool is found
      ([QTCREATORBUG-30505](https://bugreports.qt.io/browse/QTCREATORBUG-30505))
* Conan
    * Fixed that backslashes were wrongly used for paths on Windows
      ([QTCREATORBUG-30326](https://bugreports.qt.io/browse/QTCREATORBUG-30326))

### Qbs

* Added support for code completion with the Qbs language server
  (QBS-395)

### Python

* Added `Generate Kit` to the Python interpreter preferences for generating a
  Python kit with this interpreter
* Added the `Kit Selection` page for creating and opening Python projects
* Added a `requirements.txt` file to the application wizard
* Fixed that the same Python interpreter could be auto-detected multiple times
  under different names

  ([Documentation](https://doc.qt.io/qtcreator/creator-python-development.html))

Debugging
---------

* Added a `dr` locator filter for debugging a project

### C++

* Added a pretty printer for `std::tuple`
* Improved the display of size information for the pretty printer of
  `QByteArray`
  ([QTCREATORBUG-30065](https://bugreports.qt.io/browse/QTCREATORBUG-30065))
* Fixed that breakpoints were not hit while the message dialog about missing
  debug information was shown
  ([QTCREATORBUG-30168](https://bugreports.qt.io/browse/QTCREATORBUG-30168))
* LLDB
    * Fixed setting breakpoints in assembler
    * Fixed `Run as root`
      ([QTCREATORBUG-30516](https://bugreports.qt.io/browse/QTCREATORBUG-30516))
* CDB
    * Fixed a missing debugger tool tip
      ([QTCREATORBUG-13413](https://bugreports.qt.io/browse/QTCREATORBUG-13413))

### Debug Adapter Protocol

* Added support for function breakpoints

Analyzer
--------

### Clang

* Added `Edit Checks as String` for Clazy
  ([QTCREATORBUG-24846](https://bugreports.qt.io/browse/QTCREATORBUG-24846))

### Axivion

* Added a view for listing and searching issues
* Added other issue types than `Style Violations` to the editor annotations

Terminal
--------

* Added `Select All` to the context menu
  ([QTCREATORBUG-29922](https://bugreports.qt.io/browse/QTCREATORBUG-29922))
* Fixed the startup performance on Windows
  ([QTCREATORBUG-29840](https://bugreports.qt.io/browse/QTCREATORBUG-29840))
* Fixed the integration of the `fish` shell
* Fixed that `Ctrl+W` closed the terminal even when shortcuts were blocked
  ([QTCREATORBUG-30070](https://bugreports.qt.io/browse/QTCREATORBUG-30070))
* Fixed issues with Windows Powershell
* Fixed issues with copying Japanese text

Version Control Systems
-----------------------

* Added support for remote version control operations

### Git

* Added the upstream status for untracked branches to `Branches` view

Test Integration
----------------

### Qt Test

* Added a locator filter for Qt Test data tags (`qdt`)

### Catch2

* Added support for namespaced fixtures
* Added support for `SCENARIO_METHOD`

Platforms
---------

### Windows

* Fixed Clang compiler ABI detection for WOA64
  ([QTCREATORBUG-30060](https://bugreports.qt.io/browse/QTCREATORBUG-30060))

### Android

* Add support for target-based android-build directories
  ([QTBUG-117443](https://bugreports.qt.io/browse/QTBUG-117443))
* Fixed issues with debugging
  ([QTCREATORBUG-29928](https://bugreports.qt.io/browse/QTCREATORBUG-29928),
   [QTCREATORBUG-30405](https://bugreports.qt.io/browse/QTCREATORBUG-30405))
* Fixed a crash when removing Android Qt versions
  ([QTCREATORBUG-30347](https://bugreports.qt.io/browse/QTCREATORBUG-30347))

### iOS

* Fixed the detection of iOS 17 devices
* Fixed deployment and running applications for iOS 17 devices
  (application output, debugging, and profiling are not supported)
  ([QTCREATORBUG-29682](https://bugreports.qt.io/browse/QTCREATORBUG-29682))
  ([Documentation](https://doc.qt.io/qtcreator/creator-developing-ios.html))

### Remote Linux

* Fixed that debugging unnecessarily downloaded files from the remote system
  ([QTCREATORBUG-29614](https://bugreports.qt.io/browse/QTCREATORBUG-29614))

### MCU

* Fixed a crash when fixing errors in MCU kits
  ([QTCREATORBUG-30360](https://bugreports.qt.io/browse/QTCREATORBUG-30360))

Credits for these changes go to:
--------------------------------
Aaron McCarthy  
Aleksei German  
Alessandro Portale  
Alexandre Laurent  
Alexey Edelev  
Ali Kianian  
Amr Essam  
Andre Hartmann  
André Pönitz  
Andreas Loth  
Artem Sokolovskii  
Assam Boudjelthia  
Aurele Faure  
Brook Cronin  
Burak Hancerli  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
Daniel Trevitz  
David Faure  
David Schulz  
Dominik Holland  
Eike Ziller  
Esa Törmänen  
Fabian Kosmale  
Friedemann Kleint  
Henning Gruendl  
Ilya Kulakov  
Jaroslaw Kobus  
Johanna Vanhatapio  
Karim Abdelrahman  
Knud Dollereder  
Leena Miettinen  
Mahmoud Badri  
Marco Bubke  
Marcus Tillmanns  
Mathias Hasselmann  
Mats Honkamaa  
Mehdi Salem  
Michael Weghorn  
Miikka Heikkinen  
Mitch Curtis  
Olivier De Cannière  
Orgad Shaneh  
Pranta Dastider  
Robert Löhning  
Sami Shalayel  
Samuel Jose Raposo Vieira Mira  
Samuel Mira  
Semih Yavuz  
Serg Kryvonos  
Shrief Gabr  
Sivert Krøvel  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Tim Jenßen  
Vikas Pachdha  
Volodymyr Zibarov  
Xavier Besson  
Yasser Grimes  
Yuri Vilmanis  
