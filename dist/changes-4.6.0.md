Qt Creator version 4.6 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.5..v4.6.0

General

* Locator
    * Added filter `b` for bookmarks
    * Added filter `t` for triggering items from main menu
    * Added filter `=` for evaluating JavaScript expressions
      (QTCREATORBUG-14380)
* File System View
    * Added bread crumbs for file path (QTCREATORBUG-19203)
    * Added `Add New`, `Rename`, `Remove File`, `Diff Against Current File`
      (QTCREATORBUG-19213, QTCREATORBUG-19209, QTCREATORBUG-19208,
      QTCREATORBUG-19211)
* Added restoration of search flags when choosing search term from history

Editing

* Added option to display annotations between lines (QTCREATORBUG-19181)
* Added shortcut setting for jumping to document start and end
* Fixed that editor could jump to end of file when editing in a different split
  (QTCREATORBUG-19550)
* Fixed order of items in list of recent documents when documents are suspended
  (QTCREATORBUG-19758)
* Fixed crash in generic highlighter (QTCREATORBUG-19916)
* Fixed issue with snippet variables on Gnome (QTCREATORBUG-19571)
* Fixed tool tips in binary editor (QTCREATORBUG-17573)

Help

* Improved startup performance

All Projects

* Added filtering to project kit setup page

CMake Projects

* Fixed that files could be shown multiple times in project tree
  (QTCREATORBUG-19020)

Qbs Projects

* Added option to add library paths to dependencies (QTCREATORBUG-19274)

C++ Support

* Clang Code Model
    * Switched to Clang 5.0, adding support for C++17
    * Implemented information tool tips, which improves type information
      including resolution of `auto` types (QTCREATORBUG-11259), template arguments
      for template types, and the first or `\brief` paragraph of documentation
      comments (QTCREATORBUG-4557)
    * Integrated Clang-Tidy and Clazy.
      Enable checks in Options > C++ > Code Model > Clang Code Model Warnings
    * Added separate highlighting for function definitions (QTCREATORBUG-16625)
    * Fixed issues with non-UTF-8 strings (QTCREATORBUG-16941)

QML Support

* Added inline annotations for issues from code model

Debugging

* Split `Expressions` view from `Locals` view (QTCREATORBUG-19167)
* LLDB
    * Fixed attaching to core file (QTCREATORBUG-18722)
    * Fixed issue when killing LLDB from the outside (QTCREATORBUG-18723)

Qt Quick Designer

* Added font and text properties from Qt 5.10
* Added `Add New Resources` to item library
* Fixed that items blurred when zooming in
* Fixed crash when changing control focus policy (QTCREATORBUG-19563)
* Fixed assert in backend process with Qt 5.9.4 & 5.10.1 and later
  (QTCREATORBUG-19729)

Version Control Systems

* Git
    * Added `Recover Deleted Files`
    * Added `Reload` button to `git log` and `git blame`
* Gerrit
    * Added support for private and work-in-progress changes for
      Gerrit 2.15 and later

Diff Viewer

* Added folding for files and chunks
* Fixed issue with repeated stage and unstage operation

Test Integration

* Added Qt Quick Test to auto test wizard
* Added grouping of test cases (QTCREATORBUG-17979)
* Fixed handling of `qCritical` output (QTCREATORBUG-19795)
* Google Test
    * Fixed detection of crashed tests (QTCREATORBUG-19565)

Model Editor

* Removed experimental state
* Added support for text alignment
* Added support for multi-line object names
* Added support for dragging items onto model editor from more panes
* Added `Export Selected Elements`
* Added `Flat` visual role
* Added `Add Related Elements` to diagram context menu
* Added wizard for scratch models
* Moved export actions to `File` menu
* Moved zoom actions to editor tool bar
* Fixed issue with selecting items (QTCREATORBUG-18368)

Platform Specific

Windows

* Added support for the [heob](https://github.com/ssbssa/heob/releases)
  memory analyzer
* Fixed detection of CDB in non-default installation roots
* Fixed issue with setting `PATH` versus `Path` environment variable

Android

* Fixed issues with GCC include directories in Clang code model

Remote Linux

* Fixed that remote application was not killed before deployment
  (QTCREATORBUG-19326)

Credits for these changes go to:  
Adam Treat  
Alessandro Portale  
Alexandru Croitor  
Andre Hartmann  
André Pönitz  
Christian Gagneraud  
Christian Kandeler  
Christian Stenger  
Daniel Engelke  
David Schulz  
Eike Ziller  
Friedemann Kleint  
Hannes Domani  
Hugo Holgersson  
Ivan Donchevskii  
Jake Petroules  
Jaroslaw Kobus  
Jochen Becher  
Jörg Bornemann  
Marco Benelli  
Marco Bubke  
Mitch Curtis  
Nikita Baryshnikov  
Nikolai Kosjar  
Oliver Wolff  
Orgad Shaneh  
Oswald Buddenhagen  
Przemyslaw Gorszkowski  
Robert Löhning  
Thomas Hartmann  
Tim Jenssen  
Tobias Hunger  
Tomasz Olszak  
Tor Arne Vestbø  
Ulf Hermann  
Vikas Pachdha
