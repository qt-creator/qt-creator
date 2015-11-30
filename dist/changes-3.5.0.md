Qt Creator version 3.5 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/3.4..origin/3.5

General

* Increased minimum requirements for compilation of Qt Creator to
  MSVC 2013 and GCC 4.7, and Qt 5.4.0
* Added variants with native separators to Qt Creator variables that
  represent file paths
* Changed the way inconsistent enabled states were handled by the
  plugin manager. Disabling plugins is now only a hint; if another
  (enabled) plugin needs it, it is implicitly enabled. Before, the
  other plugin was implicitly disabled.
* Improved keyboard shortcut settings. Made shortcut input field
  freely editable and added separate `record` button.
* Added support for `~` as shortcut for user's home directory to
  path input fields
* Added filtering to About Plugins
* Added `-load all` and `-noload all` command line options that
  enable and disable all plugins respectively
* Made `-load` command line option implicitly enable all required
  plugins, and `-noload` disable all plugins requiring the
  disabled plugin. Multiple `-load` and `-noload` options are
  interpreted in the order given on the command line.
* Fixed issues with raising the Qt Creator window on Gnome desktop
  (QTCREATORBUG-13845)
* Fixed appearance on high DPI displays on Windows
  (QTCREATORBUG-11179)
* Added locator filter for running external tools

Editing

* Added highlighting of search results to editor scroll bar
* Added option to jump directly to specific column in addition to
  line number when opening files through locator or command line
* Added *Remove missing files* action to QRC editor
  (QTCREATORBUG-13941)
* Made global file search use multiple threads (QTCREATORBUG-10298)
* Fixed highlighting of current line in read-only text editors
  (QTCREATORBUG-10104)
* Fixed issues with completion while inserting snippet (QTCREATORBUG-14633)

Project Management

* Fixed issues with restoring project tree state (QTCREATORBUG-14304)
* Fixed crash when application output contains incomplete control
  sequence (QTCREATORBUG-14720)

CMake Projects

* Made it possible to register multiple CMake executables
* Added support for file targets when explicitly specified in the
  generated CodeBlocks file
* Fixed default shadow build directory name

Generic Projects

* Fixed that resource links were removed from UI files
  (QTCREATORBUG-14275)

QML-Only Projects (.qmlproject)

* Re-enabled the plugin by default

Autotools Projects

* Improved parsing of `CPPFLAGS`
* Added support for line continuations
* Added support for `top_srcdir`, `abs_top_srcdir`, `top_builddir` and
  `abs_top_builddir`

Debugging

* Added dumper for `QJsonValue`, `QJsonObject`, `QJsonArray`, `QUuid`
* Added basic support for GDB's fork-follows-child
* Improved support for GDB 7.9 and LLDB 3.7
* Fixed display of `QHash` keys with value 0 (QTCREATORBUG-14451)
* Fixed variable expansion state in QML debugger
* Fixed display of members of returned values
* Fixed that items in Locals and Expressions did not expand on first
  click for QML (QTCREATORBUG-14210)

QML Profiler

* Removed support for V8
* Made saving and loading trace data asynchronous to avoid
  locking up UI (QTCREATORBUG-11822)

C++ Support

* Added separate icon for structs
* Added support for setting the access specifier of an extracted function (QTCREATORBUG-12127)
* Moved Clang code model backend out-of-process
* Fixed *Convert to Stack Variable* refactoring action for empty
  initializer lists (QTCREATORBUG-14279)
* Fixed misplaced newlines of refactoring actions (QTCREATORBUG-13872)
* Fixed expanding items in class view with double-click
  (QTCREATORBUG-2536)
* Fixed code folding issues after missing closing braces
* Fixed resolving of decltype (QTCREATORBUG-14483)
* Fixed resolving of template using alias
  For example: `template<class T> using U = Temp<T>` (QTCREATORBUG-14480)
* Fixed some issues related to template lookup (QTCREATORBUG-14141,
  QTCREATORBUG-14218, QTCREATORBUG-14237)
* Fixed resolving of partial and full template specialization (QTCREATORBUG-14034)
* Partially fixed STL containers (QTCREATORBUG-8937, QTCREATORBUG-8922)
    * GCC implementation of `std::map`, `std::unique_ptr` (and other pointer wrappers)
      and `std::vector` are known to work
    * MSVC implementation is not supported
* Fixed that highlighting vanished after text zoom (QTCREATORBUG-14579)
* Fixed issues with completion while renaming local variable (QTCREATORBUG-14633)

QML Support

* Removed Qt Quick 1 wizards
* Fixed missing auto-completion for `QtQuick` and `QtQuick.Controls`
  (QTCREATORBUG-14563)

Qt Quick Designer

* Removed Qt Quick 1 support

Version Control Systems

* Perforce
    * Added support for P4CONFIG (QTCREATORBUG-14378)

FakeVim

* Added support for `C-r{register}`
* Added support for remapping shortcuts

Todo

* Added option to exclude file patterns from parsing

Beautifier

* Added option to format only selected lines with Uncrustify (`--frag`)

Platform Specific

Windows

* Fixed that Qt Creator could freeze while user application is running
  (QTCREATORBUG-14676)

OS X

* Added locator filter that uses Spotlight for locating files

Linux

* Fixed performance issue with journal support (QTCREATORBUG-14356)

Android

* Made it possible to create AVD without SD card (QTCREATORBUG-13590)
* Improved handling of invalid names when creating AVD
  (QTCREATORBUG-13589)
* Added 5.1 to known versions
* Added warning if emulator is not OpenGL enabled
  (QTCREATORBUG-13615)
* Added input field for activity name in Android manifest editor
  (QTCREATORBUG-13958)
* Fixed issues with Android M (QTCREATORBUG-14537, QTCREATORBUG-14534)
* Fixed issues with 64 bit
* Fixed handling of external file changes in Android manifest editor
* Fixed listing of Google AVDs (QTCREATORBUG-13980)
* Fixed that kits were removed from projects when changing NDK path
  (QTCREATORBUG-14243)
* Fixed copying application data with spaces in path
  (QTCREATORBUG-13868)
* Fixed that sometimes the wrong AVD was deployed to
  (QTCREATORBUG-13095)

BlackBerry

* Removed support for BlackBerry 10 development

Remote Linux

* Added support for ECDH key exchange for SSH connections
  (QTCREATORBUG-14025)

BareMetal

* Fixed processing of additional OpenOCD arguments

Credits for these changes go to:  
Alessandro Portale  
André Pönitz  
Alexander Drozdov  
Alexander Izmailov  
Arnold Dumas  
Benjamin Zeller  
BogDan Vatra  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
Daniel Teske  
David Schulz  
Denis Kormalev  
Eike Ziller  
Erik Verbruggen  
Finn Brudal  
Friedemann Kleint  
Hugues Delorme  
Jack Andersen  
Jarek Kobus  
Jochen Becher  
Jörg Bornemann  
Johannes Lorenz  
Kai Köhne  
Knut Petter Svendsen  
Kudryavtsev Alexander  
Leena Miettinen  
Libor Tomsik  
Lorenz Haas  
Lukas Holecek  
Marcel Krems  
Marco Benelli  
Marco Bubke  
Montel Laurent  
Nikita Baryshnikov  
Nikita Kniazev  
Nikolai Kosjar  
Olivier Goffart  
Orgad Shaneh  
Ray Donnelly  
Robert Löhning  
Stanislav Ionascu  
Sune Vuorela  
Takumi ASAKI  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Thorben Kroeger  
Tim Jenssen  
Tobias Hunger  
Ulf Hermann  
