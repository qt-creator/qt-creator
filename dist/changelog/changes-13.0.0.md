Qt Creator 13
=============

Qt Creator version 13 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/12.0..v13.0.0

What's new?
-----------

* Added Qt Application Manager support

### Qt Application Manager

Adds support for Qt 6 based applications with CMake for creating, building,
deploying, running, and debugging on devices that use the
[Qt Application Manager](https://doc.qt.io/QtApplicationManager/).

([Documentation](https://doc.qt.io/qtcreator/creator-overview-qtasam.html))

General
-------

* Improved docking (Debug mode, Widget Designer)
    * Fixed the style of titles and changed them to always be visible
      (removed `View > Views > Automatically Hide View Titlebars`)
    * Added an option to collapse panels
    * Changed `Hide/Show Left/Right Sidebar` to hide and show the corresponding
      dock area
* Added the option to show file paths relative to the active project to the
  search results view
  (QTCREATORBUG-29462)
* Added a `Current` button for selecting the directory of the current document
  for searching in `Files in File System`
* Added `Copy to Clipboard` to the `About Qt Creator` dialog
  (QTCREATORBUG-29886)

Editing
-------

* Made syntax highlighting asynchronous
* Fixed that `Shift+Tab` did not always unindent
  (QTCREATORBUG-29742)
* Fixed that `Surround text selection with brackets` did nothing for `<`
* Fixed following links without a file name in documents without a file name

### C++

* Added the `Move Definition Here` refactoring action that moves an existing
  function definition to its declaration
  (QTCREATORBUG-9515)
* Enabled the completion inside comments and strings by falling back to the
  built-in code model
  (QTCREATORBUG-20828)
* Improved the position of headers inserted by refactoring operations
  (QTCREATORBUG-21826)
* Improved the coding style settings by separating Clang Format and other coding
  style settings, and using a plain text editor for custom Clang Format settings
* Fixed that the class wizards used the class name for the include guard
  instead of the file name
  (QTCREATORBUG-30140)
* Fixed that renaming classes did not change the include directive for the
  renamed header in the source file
  (QTCREATORBUG-30154)
* Fixed issues with refactoring template functions
  (QTCREATORBUG-29408)
* Clangd
    * Fixed that `Follow Symbol Under Cursor` only worked for exact matches
      (QTCREATORBUG-29814)

### QML

* Added navigation from QML components to the C++ code in the project
  (QTCREATORBUG-28086)
* Added a button for launching the QML Preview on the current document to
  the editor tool bar
* Added color previews when hovering Qt color functions
  (QTCREATORBUG-29966)

### Python

* Fixed that global and virtual environments were polluted with `pylsp` and
  `debugpy` installations

### Language Server Protocol

* Added automatic setup up of language servers for `YAML`, `JSON`, and `Bash`
  (requires `npm`)

### Widget Designer

* Fixed the indentation of the code that is inserted by `Go to slot`
  (QTCREATORBUG-11730)

### Compiler Explorer

* Added highlighting of the matching source lines when hovering over the
  assembly

### Markdown

* Added the common text editor tools (line and column, encoding, and line
endings) to the tool bar
* Added support for following links to the text editor

Projects
--------

* Added a section `Vanished Targets` to `Projects` mode in case the project
  was configured for kits that have vanished, as a replacement for the automatic
  creation of "Replacement" kits
* Added the status of devices to the device lists
  (QTCREATORBUG-20941)
* Added the `Preferences > Build & Run > General > Application environment`
  option for globally modifying the environment for all run configurations
  (QTCREATORBUG-29530)
* Added a file wizard for Qt translation (`.ts`) files
  (QTCREATORBUG-29775)
* Increased the maximum width of the target selector
  (QTCREATORBUG-30038)
* Fixed that the `Left` cursor key did not always collapse the current item
  (QTBUG-118515)
* Fixed inconsistent folder hierarchies in the project tree
  (QTCREATORBUG-29923)

### CMake

* Added support for custom output parsers for the configuration of projects
  (QTCREATORBUG-29992)
* Made cache variables available even if project configuration failed

### Python

* Added `Generate Kit` to the Python interpreter preferences for generating a
  Python kit with this interpreter
* Added the target setup page when loading unconfigured Python projects
* Fixed that the same Python interpreter could be auto-detected multiple times
  under different names

Debugging
---------

### C++

* Fixed that breakpoints were not hit while the message dialog about missing
  debug information was shown
  (QTCREATORBUG-30168)

### Debug Adapter Protocol

* Added support for function breakpoints

Analyzer
--------

### Clang

* Added `Edit Checks as Strings` for Clazy
  (QTCREATORBUG-24846)

### Axivion

* Added fetching and showing issues

Terminal
--------

* Added `Select All` to the context menu
  (QTCREATORBUG-29922)
* Fixed the startup performance on Windows
  (QTCREATORBUG-29840)
* Fixed the integration of the `fish` shell
* Fixed that `Ctrl+W` closed the terminal even when shortcuts were blocked
  (QTCREATORBUG-30070)

Version Control Systems
-----------------------

### Git

* Added the upstream status for untracked branches to `Branches` view

Test Integration
----------------

### Qt Test

* Added a locator filter for Qt Test data tags (`qdt`)

Platforms
---------

### Android

* Add support for target-based android-build directories (??? is that ready? Qt 6.8+ ???)
  (QTBUG-117443)

### iOS

* Fixed the detection of iOS 17 devices
* Fixed deployment and running applications for iOS 17 devices
  (application output, debugging, and profiling are not supported)
  (QTCREATORBUG-29682)

### Remote Linux

* Fixed that debugging unnecessarily downloaded files from the remote system
  (QTCREATORBUG-29614)

Credits for these changes go to:
--------------------------------
Aaron McCarthy  
Aleksei German  
Alessandro Portale  
Alexey Edelev  
Ali Kianian  
Amr Essam  
Andre Hartmann  
André Pönitz  
Andreas Loth  
Artem Sokolovskii  
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
Miikka Heikkinen  
Mitch Curtis  
Olivier De Cannière  
Orgad Shaneh  
Pranta Dastider  
Robert Löhning  
Sami Shalayel  
Samuel Jose Raposo Vieira Mira  
Serg Kryvonos  
Shrief Gabr  
Sivert Krøvel  
Tasuku Suzuki  
Thomas Hartmann  
Tim Jenßen  
Vikas Pachdha  
Volodymyr Zibarov  
Xavier Besson  
Yasser Grimes  
Yuri Vilmanis  
