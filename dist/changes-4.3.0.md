Qt Creator version 4.3 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.2..v4.3.0

General

* Added option to search `Files in File System` with Silver Searcher (`ag`)
  (experimental `SilverSearcher` plugin)
* Added exclusion patterns to `Advanced Find` and custom locator filters
* Added navigation pane on right side of edit mode
* Removed dependency of Welcome mode on OpenGL, improving experience in
  virtual machine environments and certain setups (QTCREATORBUG-15727)
* Fixed wrong UI colors after suspend (QTCREATORBUG-14929)
* Fixed crash with invalid themes (QTCREATORBUG-17517)

Help

* Fixed that help bookmarks got lost (QTCREATORBUG-17537)

Editing

* Added optional shortcut for duplicating current selection
* Adapted to changes of code pasting services
  (QTCREATORBUG-17942, QTCREATORBUG-18192)
* Fixed freeze when highlighting `Kconfig` file (QTCREATORBUG-14611)

All Projects

* Added support for `.qrc` files in project tree for all projects
* Added Qt Creator variable `CurrentRun:Executable` (QTCREATORBUG-12201,
  QTCREATORBUG-16830)
* Added choice of build system to most project wizards (QTCREATORBUG-17308)

QMake Projects

* Fixed wrong warning when specifying absolute path to mkspec
  (QTCREATORBUG-17237)
* Fixed deployment of symlinks for versioned shared libraries

CMake Projects

* Added support for `server-mode` with CMake 3.7 or later
    * Added products and targets to project tree
    * Added option to build individual products and targets
    * Removed the need for `CodeBlocks` extra generator
* Added header files to project tree, even if not listed explicitly in
  project files
* Added import of configuration of existing builds
* Fixed `Build > Clean`

Generic Projects

* Added expansion of Qt Creator variables in project files

C++ Support

* Added support for `clang-query` in `Advanced Find` to experimental
  `ClangRefactoring` plugin
* Added switching project and language context for parsing files to editor
  toolbar
* Added support for `--gcctoolchain` option
* Improved performance of first completion in file on Windows
* Fixed handling of Objective-C/C++
* Fixed cursor position after correcting `.` to `->` (QTCREATORBUG-17697)
* Fixed that quotes were added when splitting raw string literals
  (QTCREATORBUG-17717)

QML Support

* Added option to automatically format QML files on save
* Added menu item for adding expression evaluators from QML code editor
  (QTCREATORBUG-17754)
* Fixed reformatting of signals (QTCREATORBUG-17886)
* Fixed issues with jumping text cursor while editing
  (QTCREATORBUG-15680, QTCREATORBUG-17413)

Nim Support

* Added automatic reparsing when files are added to or removed from project
* Added Nim compiler setting to kits
* Fixed that loading projects blocked Qt Creator
* Fixed crash when opening empty projects

Debugging

* Added pretty printing of `unordered_multi(set|map)`, `boost::variant` and
  `QLazilyAllocated`
* Fixed that expression evaluators were not evaluated when added
  (QTCREATORBUG-17763)
* QML
    * Fixed accessing items by `id` in `Debugger Console` (QTCREATORBUG-17177)
* GDB
    * Fixed issue with templated types that are pretty printed differently
      depending on argument type (`std::vector<bool>` versus `std::vector<t>`)
* CDB
    * Changed to Python based pretty printing backend, resulting in faster
      startup and more, faster, and unified pretty printers

QML Profiler

* Added performance information to QML code editor (QTCREATORBUG-17757)
* Improved performance of rendering timeline and loading trace files
* Improved error and progress reporting for loading and saving trace files
* Fixed pixmap cache size information when loading profile
  (QTCREATORBUG-17424)
* Fixed UI issues (QTCREATORBUG-17939, QTCREATORBUG-17937)

Qt Quick Designer

* Added support for HiDPI
* Added text editor view
* Added tool bar for common actions
* Added changing type of item by changing type name in property editor
* Added support for `qsTranslate` (QTCREATORBUG-17714)
* Added actions for adding items, selecting visible item, and adding tab bar
  to stacked containers
* Fixed that `Dialog` was not allowed in `.ui.qml` files
* Fixed that error messages could be shown twice
* Fixed handling of escaped unicode characters (QTCREATORBUG-12616)
* Fixed that document needed to be manually re-opened after type information
  became available
* Fixed crash when root item is layout
* Fixed that expressions were not shown in URL input field (QTCREATORBUG-13328)

Version Control Systems

* Git
    * Added option to only show the first parent of merge commits in log
    * Added action to skip a commit during rebase (QTCREATORBUG-17350)
    * Added option to sign-off commits
    * Fixed handling of already merged files in merge tool
* Gerrit
    * Added detection of Gerrit remotes
    * Added support for accessing Gerrit via REST API over HTTP(S)

Test Integration

* Removed `experimental` state
* Improved display of test results (QTCREATORBUG-17104)
* Added option to limit searching for tests to folders matching pattern
  (QTCREATORBUG-16705)
* Fixed detection of inherited test methods (QTCREATORBUG-17522)
* Fixed missing update of test list when QML files are added or removed
  (QTCREATORBUG-17805)

SCXML Editor

* Fixed adding elements to `else` case (QTCREATORBUG-17674)
* Fixed that copying and pasting state created invalid name

Beautifier

* Uncrustify
    * Added option to select config file to use

Platform Specific

Windows

* Fixed that it was not possible to save files with arbitrary extension
  (QTCREATORBUG-15862)
* Fixed ABI detection for Clang
* Fixed that ABI of MSVC2017 was considered different from ABI of MSVC2015
  (QTCREATORBUG-17740)

Linux

* Worked around issue that Unity menu bar vanished after editing main window in
  Design mode (QTCREATORBUG-17519)

Android

* Improved package signing (QTCREATORBUG-17545, QTCREATORBUG-17304)
* Fixed issues with new Android SDK (25.3.1)
  (QTCREATORBUG-17814, QTCREATORBUG-18013)
* Fixed debugging of release builds

iOS

* Added option to select developer team and provisioning profile used for
  signing (QTCREATORBUG-16936)
* Fixed that starting simulator blocked Qt Creator
* Fixed `Run Without Deployment` on Simulator (QTCREATORBUG-18107)

Remote Linux

* Added incremental deployment to `tar` package deployment

QNX

* Added support for 64bit platforms

Credits for these changes go to:  
Alessandro Portale  
Alexander Drozdov  
Alexandru Croitor  
Andre Hartmann  
Andreas Pakulat  
André Pönitz  
Arnold Dumas  
BogDan Vatra  
Christian Gagneraud  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
Daniel Kamil Kozar  
Daniel Trevitz  
David Schulz  
Eike Ziller  
Filippo Cucchetto  
Florian Apolloner  
Francois Ferrand  
Frank Meerkötter  
Friedemann Kleint  
Hugo Holgersson  
Jake Petroules  
James McDonnell  
Jaroslaw Kobus  
Jesus Fernandez  
Juhapekka Piiroinen  
Jörg Bornemann  
Kari Oikarinen  
Kavindra Palaraja  
Konstantin Podsvirov  
Leena Miettinen  
Lorenz Haas  
Lukas Holecek  
Marco Benelli  
Marco Bubke  
Mathias Hasselmann  
Max Blagay  
Michael Dönnebrink  
Michal Steller  
Montel Laurent  
Nikita Baryshnikov  
Nikolai Kosjar  
Oleg Yadrov  
Orgad Shaneh  
Oswald Buddenhagen  
Przemyslaw Gorszkowski  
Robert Löhning  
Serhii Moroz  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Tobias Hunger  
Ulf Hermann  
Vikas Pachdha
