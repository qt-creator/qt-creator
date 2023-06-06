Qt Creator 11
=============

Qt Creator version 11 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/10.0..v11.0.0

General
-------

* Added a `Terminal` view (QTCREATORBUG-8511)
    * Opt-out via `Preferences` > `Terminal` preferences
    * Added support for
        * different shells, colors, fonts, and multiple tabs
        * opening file paths in Qt Creator with `Ctrl+click` (`Cmd+click` on
          macOS)
* Added a more spacious "relaxed" toolbar style `Environment > Interface`
* Added a pin button to progress details instead of automatically resetting
  their position (QTCREATORBUG-28829)
* Improved the selection and navigation in the `Issues` view
  (QTCREATORBUG-26128, QTCREATORBUG-27006, QTCREATORBUG-27506)
* Locator
    * Improved performance
    * Added the creation of directories to the `Files in File System` filter
    * Added device roots and browsing remote file systems to the
      `Files in File System` filter

Editing
-------

* Improved the performance of the multi-cursor support
* Fixed the saving of hardlinked files (QTCREATORBUG-19651)
* Fixed an issue of copy and paste with multiple cursors (QTCREATORBUG-29117)

### C++

* Improved the style of forward declarations in the outline (QTCREATORBUG-312)
* Added highlighting for typed string literals and user-defined literals
  (QTCREATORBUG-28869)
* Added the option to create class members from assignments (QTCREATORBUG-1918)
* Fixed that locator showed both the declaration and the definition of symbols
  (QTCREATORBUG-13894)
* Fixed the handling of C++20 keywords and concepts
* Built-in
     * Fixed support for `if`-statements with initializer (QTCREATORBUG-29182)

### Language Server Protocol

* Added experimental support for GitHub Copilot
  ([GitHub documentation](https://github.com/features/copilot))
* Added missing actions for opening the `Call Hierarchy` (QTCREATORBUG-28839,
  QTCREATORBUG-28842)

### QML

* Fixed the reformatting in the presence of JavaScript directives and function
  return type annotations (QTCREATORBUG-29001, QTCREATORBUG-29046)
* Fixed that reformatting changed `of` to `in` (QTCREATORBUG-29123)
* Fixed the completion for Qt Quick Controls (QTCREATORBUG-28648)

### Python

* Added the option to create a virtual environment (`venv`) to the Python
  interpreter selector and the wizard (PYSIDE-2152)

### Markdown

* Added a Markdown editor with preview (QTCREATORBUG-27883)
* Added a wizard for Markdown files (QTCREATORBUG-29056)

Projects
--------

* Made it possible to add devices without going through the wizard
* Added support for moving files to a different directory when renaming
  (QTCREATORBUG-15981)

### CMake

* Implemented adding files to the project (QTCREATORBUG-25922,
  QTCREATORBUG-26006, QTCREATORBUG-27213, QTCREATORBUG-27538,
  QTCREATORBUG-28493, QTCREATORBUG-28904, QTCREATORBUG-28985,
  QTCREATORBUG-29006)
* Fixed issues with detecting a configured Qt version when importing a build
  (QTCREATORBUG-29075)

### Python

* Added an option for the interpreter to the wizards

### vcpkg

* Added experimental support for `vcpkg`
  ([vcpgk documentation](https://vcpkg.io/en/))
* Added an option for the `vcpkg` installation location
* Added a search dialog for packages
* Added a wizard and an editor for `vcpkg.json` files

Debugging
---------

* Improved the UI for enabling and disabling debuggers (QTCREATORBUG-28627)

### C++

* Added an option for the default number of array elements to show
  (`Preferences > Debugger > Locals & Expressions > Default array size`)
* CDB
    * Added automatic source file mapping for Qt packages
    * Fixed the variables view on remote Windows devices (QTCREATORBUG-29000)
* LLDB
    * Fixed that long lines in the application output were broken into multiple
      lines (QTCREATORBUG-29098)

### Qt Quick

* Improved the auto-detection if QML debugging is required (QTCREATORBUG-28627)
* Added an option for disabling static analyzer messages to
  `Qt Quick > QML/JS Editing` (QTCREATORBUG-29095)

Analyzer
--------

### Clang

* Fixed that a `.clang-tidy` file in the project directory was not used by
  default (QTCREATORBUG-28852)

### Axivion

* Added experimental support

Version Control Systems
-----------------------

### Git

* Instant Blame
    * Improved the performance (QTCREATORBUG-29151)
    * Fixed that it did not show at the end of the document

Platforms
---------

### Android

* Fixed an issue with building library targets (QTCREATORBUG-26980)

### Remote Linux

* Removed the automatic sourcing of target-side shell profiles

### Docker

* Added support for `qmake` based projects (QTCREATORBUG-29140)
* Fixed issues after deleting the Docker image for a registered Docker device
  (QTCREATORBUG-28880)

### QNX

* Added `slog2info` as a requirement for devices
* Fixed the support for remote working directories (QTCREATORBUG-28900)

Credits for these changes go to:
--------------------------------
Aleksei German  
Alessandro Portale  
Alexander Drozdov  
Alexander Pershin  
Ali Kianian  
Alibek Omarov  
Amr Essam  
Andre Hartmann  
André Pönitz  
Artem Mukhin  
Artem Sokolovskii  
Assam Boudjelthia  
Björn Schäpers  
Brook Cronin  
Burak Hancerli  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Esa Törmänen  
Fabian Kosmale  
Filippo Gentile  
Friedemann Kleint  
Henning Gruendl  
Jaroslaw Kobus  
Jussi Witick  
Kai Köhne  
Knud Dollereder  
Knut Petter Svendsen  
Leena Miettinen  
Mahmoud Badri  
Marco Bubke  
Marcus Tillmanns  
Martin Delille  
Mats Honkamaa  
Miikka Heikkinen  
Mitch Curtis  
Niels Weber  
Orgad Shaneh  
Pranta Dastider  
Robert Löhning  
Samuel Ghinet  
Semih Yavuz  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Tim Jenßen  
Ulf Hermann  
Vikas Pachdha  
Yasser Grimes  
Yixue Wang  
