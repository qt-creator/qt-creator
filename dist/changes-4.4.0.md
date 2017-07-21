Qt Creator version 4.4 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.3..v4.4.0

General

* Added highlighting of search term in Locator results
* Added larger icons to `New` dialog
* Added locator input to extra editor and help windows (QTCREATORBUG-9696)
* Fixed theming of Debugger Console and TODO pane (QTCREATORBUG-17532)

Editing

* Added inline annotations for errors, warnings and bookmarks
* Added optional smooth scrolling when navigating within the same file
  (for example with Locator or `Follow Symbol Under Cursor`)
* Added overridable `DeleteStartOfLine` and `DeleteEndOfLine` actions
  (QTCREATORBUG-18095)
* Added support for relative path to active project to `Advanced Find` >
  `Files in File System` (QTCREATORBUG-18139)
* Added colors to default text editor scheme (the previous default is
  available as `Default Classic`)
* FakeVim
    * Fixed `gt`/`gT`/`:tabnext`/`:tabprevious`

All Projects

* Improved detection of cross-compilers

CMake Projects

* Added option to filter for CMake variables in build configuration
  (QTCREATORBUG-17973)
* Added warning when detecting `CMakeCache.txt` in source directory even though
  build is configured for out-of-source build (QTCREATORBUG-18381)
* CMake >= 3.7
    * Fixed that headers from top level directory were not shown in project tree
      (QTCREATORBUG-17760)
    * Improved handling of `CMAKE_RUNTIME_OUTPUT_DIRECTORY` (QTCREATORBUG-18158)

Qbs Projects

* Re-added `Qbs install` deploy step (QTCREATORBUG-17958)
* Added `rebuild` and `clean` actions to products and subprojects
  (QTCREATORBUG-15919)

C++ Support

* Added option to rename files when renaming symbol using same name
  (QTCREATORBUG-14696)
* Added auto-insertion of matching curly brace (QTCREATORBUG-15073)
* Fixed that C++ and Qt keywords were considered keywords in C files
  (QTCREATORBUG-2818, QTCREATORBUG-18004)
* Fixed highlighting of raw string literals (QTCREATORBUG-17720)
* Fixed `Add #include` refactoring action for static functions
* Clang Code Model
    * Added highlighting of identifier under cursor, which was still
      delegated to built-in code model
    * Improved order of items in completion list
      (QTCREATORBUG-18319, QTCREATORBUG-15445)
    * Fixed function signature hint when completing constructors and functors
      (QTCREATORBUG-14882)
    * Fixed that completing function pointer was adding parentheses
      (QTCREATORBUG-17578)
    * Fixed completion inside function template (QTCREATORBUG-17222)
    * Fixed wrong column number with non-ASCII characters (QTCREATORBUG-16775)
    * Fixed highlighting of primitive types and operators (QTCREATORBUG-17867)
    * Fixed highlighting of partial template specializations
    * Fixed highlighting of functions in `using` declarations
    * Fixed that keywords were highlighted in preprocessor directives
      (QTCREATORBUG-15516)
* Built-in Code Model
    * Fixed completion of STL containers (QTCREATORBUG-1892)

QML Support

* Updated QML parser to newer QML version (QTCREATORBUG-17842)

Debugging

* Added `Alt+V` + letter shortcuts to open views
* Added pretty printing for `qfloat16`, `std::{optional,byte}`, `gsl::{span,byte}`
  and `boost::variant`
* Improved display of enum bitfields
* Fixed support for `long double` (QTCREATORBUG-18023)
* CDB
    * Added support for extra debugging helpers and debugging helper
      customization
    * Added warning if run configuration uses unsupported shell command

Version Control Systems

* Fixed format of visual whitespace in blame, log and git rebase editors
  (QTCREATORBUG-17735)
* Git
    * Improved branch listing in `Show` (QTCREATORBUG-16949)
* Gerrit
    * Added validation of server certificate when using REST API
    * Fixed that non-Gerrit remotes were shown in `Push to Gerrit` dialog
      (QTCREATORBUG-16367)
* ClearCase
    * Disabled by default

Diff Viewer

* Fixed state of actions in `Edit` menu
* Fixed that context information for chunks was not shown in side-by-side view
  (QTCREATORBUG-18289)
* Fixed that UI blocked when showing very large diffs

Test Integration

* Added view with complete, unprocessed test output
* Made it possible to enable and disable all tests using a specific test
  framework
* QTest
    * Added option to run verbose and with logging of signals and slots
      (`-vb` and `-vs`)

Beautifier

* Added option for using a different AStyle configuration file
* Added option for fallback style for `clang-format`

Platform Specific

Windows

* Removed support for Windows CE

Android

* Added option to run commands before app starts and after app stopped
* Fixed state of actions in `Edit` menu in text based manifest editor

iOS

* Added UI for managing simulator devices (QTCREATORBUG-17602)

Remote Linux

* Added support for `ssh-agent` (QTCREATORBUG-16245)

Credits for these changes go to:  
Alessandro Portale  
Alexander Drozdov  
Andre Hartmann  
André Pönitz  
Christian Kandeler  
Christian Stenger  
Daniel Teske  
David Schulz  
Eike Ziller  
Felix Kälberer  
Florian Apolloner  
Friedemann Kleint  
Ivan Donchevskii  
Jake Petroules  
Jaroslaw Kobus  
Jesus Fernandez  
Jochen Becher  
Jörg Bornemann  
Kai Köhne  
Leandro T. C. Melo  
Leena Miettinen  
Lorenz Haas  
Marco Benelli  
Marco Bubke  
Mitch Curtis  
Montel Laurent  
Nikita Baryshnikov  
Nikolai Kosjar  
Orgad Shaneh  
Przemyslaw Gorszkowski  
Robert Löhning  
Serhii Moroz  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Tobias Hunger  
Tomasz Olszak  
Tor Arne Vestbø  
Ulf Hermann  
Vikas Pachdha
