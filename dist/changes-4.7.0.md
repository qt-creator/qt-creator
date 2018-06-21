Qt Creator version 4.7 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.6..v4.7.0

General

* Added option for enabling and disabling HiDPI scaling on Windows and Linux
  (QTCREATORBUG-20232)
* Added `Properties` item to context menu on files (QTCREATORBUG-19588)
* Added `New Search` button to search results pane (QTCREATORBUG-17870)
* Added option to show only icons in mode selector (QTCREATORBUG-18845)
* File System View
    * Added `New Folder` (QTCREATORBUG-17358)
    * Added `Collapse All` (QTCREATORBUG-19212)
    * Added option to show folders on top (QTCREATORBUG-7818)
    * Made synchronization of root directory with current document optional
      (QTCREATORBUG-19322)

Editing

* Made replacement with regular expression search more perl-like (`$<number>`
  and `$&` are supported, whereas `&` is no longer used for captures)
  (QTCREATORBUG-9602, QTCREATORBUG-15175)
* Added `Context Help` to editor context menu (QTCREATORBUG-55)
* Added previous and next buttons to bookmarks view, and polished their
  behavior (QTCREATORBUG-9859, QTCREATORBUG-20061)
* Fixed that extra editor windows were not restored when opening session
  (QTCREATORBUG-13840)
* FakeVim
    * Added `:<range>sor[t][!]`

Help

* Improved performance impact on start up

All Projects

* Moved kit settings to separate options category
* Made it easier to abort builds by changing build button to stop button while
  building (QTCREATORBUG-20155)
* Added project type specific warnings and errors for kits, and made them
  visible in `Projects` mode
* Added shortcut for showing current document in project tree
  (QTCREATORBUG-19625)
* Added global option for `Add linker library search paths to run environment`
  (QTCREATORBUG-20240)

QMake Projects

* Added support for `-isystem` in `QMAKE_CXXFLAGS`
* Added deployment rules for devices to widget and console application wizards
  (QTCREATORBUG-20358)

C++ Support

* Improved resize behavior of editor tool bar
  (QTCREATORBUG-15218, QTCREATORBUG-19386)
* Fixed auto-insertion of closing brace and semicolon after classes
  (QTCREATORBUG-19726)
* Fixed location information of macros (QTCREATORBUG-19905)
* Clang Code Model
    * Enabled by default
    * Switched to Clang 6.0
    * Implemented outline pane, outline dropdown and
      `C++ Symbols in Current Document` locator filter
    * Implemented `Follow Symbol` for single translation unit
    * Added type highlighting for Objective-C/C++
    * Added errors and warnings of current editor to Issues pane
      (category `Clang Code Model`)
    * Added highlighting style for overloaded operators (QTCREATORBUG-19659)
    * Added option to use `.clang-tidy` configuration file or checks string
    * Added link to the documentation in tooltip for Clang-Tidy and Clazy
      diagnostics
    * Improved reparse performance and memory usage
    * Improved selecting and deselecting specific Clang-Tidy checks
    * Fixed slow completion in case Clang-Tidy or Clazy checks were enabled
    * Fixed crashes when closing documents fast
* Built-in Code Model
    * Added support for nested namespaces (QTCREATORBUG-16774)

QML Support

* Updated parser to Qt 5.10, adding support for user-defined enums
* Fixed that linter warning `M127` was shown as error (QTCREATORBUG-19534)
* Fixed that reformatting incorrectly removed quotes (QTCREATORBUG-17455)
* Fixed that `.pragma` and `.import` were removed when reformatting
  (QTCREATORBUG-13038)

Python Support

* Added stack traces in application output to Issues pane (category `Python`)

Clang Static Analyzer

* Renamed plugin to `ClangTools`
* Replaced Clang static analyzer by tool that runs Clang-Tidy and Clazy over
  whole project or a subset of the project's files

QML Profiler

* Improved performance of timeline
* Added zooming into flame graph items

Version Control Systems

* Git
    * Added `-git-show <ref>` command line parameter
* Gerrit
    * Added warning when pushing to wrong branch (QTCREATORBUG-20062)

Image Viewer

* Added option to export SVGs in multiple resolutions simultaneously

Test Integration

* Added `Run Test Under Cursor` to C++ editor
* Added editor marks for failed test locations (QTCREATORBUG-20328)
* Google Test
    * Added support for filters
    * Fixed issue with jumping to file and line of failing test
      (QTCREATORBUG-18725)
* Qt Quick
    * Fixed parsing issue with non-ASCII characters (QTCREATORBUG-20105)

Platform Specific

Windows

* Improved parsing of MSVC error messages (QTCREATORBUG-20297)
* Fixed that querying MSVC tool chains at startup could block Qt Creator
* Fixed issue with Clang and Qt 5.8 and later (QTCREATORBUG-20021)

Android

* Improved behavior when emulator cannot be started (QTCREATORBUG-20160)

Credits for these changes go to:  
Aleix Pol  
Alessandro Portale  
Alexander Drozdov  
Andre Hartmann  
André Pönitz  
Antonio Di Monaco  
Arnold Dumas  
Benjamin Balga  
Christian Kandeler  
Christian Stenger  
Claus Steuer  
Colin Duquesnoy  
David Schulz  
Eike Ziller  
Friedemann Kleint  
Hugo Holgersson  
Ivan Donchevskii  
Jaroslaw Kobus  
Jay Gupta  
Jörg Bornemann  
Kari Oikarinen  
Leena Miettinen  
Marco Benelli  
Marco Bubke  
Mitch Curtis  
Nikita Baryshnikov  
Nikolai Kosjar  
Orgad Shaneh  
Pawel Rutka  
Przemyslaw Gorszkowski  
Razi Alavizadeh  
Robert Löhning  
Rune Espeseth  
Sergey Morozov  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Tobias Hunger  
Ulf Hermann  
Vikas Pachdha  
