Qt Creator version 4.9 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.8..v4.9.0

General

* Added high-level introduction to Qt Creator's UI for first-time users
  (QTCREATORBUG-21585)
* Added option to run external tools in build or run environment of
  active project (QTCREATORBUG-18394, QTCREATORBUG-19892)
* Improved selection colors in dark themes (QTCREATORBUG-18888)
* Added -temporarycleansettings (alias -tcs) command line option

Editing

* Language Client
    * Added support for document outline (QTCREATORBUG-21573)
    * Added support for `Find References to Symbol Under Cursor`
      (QTCREATORBUG-21577)
    * Added support for code actions
* Highlighter
    * Replaced custom highlighting file parser with `KSyntaxHighlighting`
      (QTCREATORBUG-21029)
* Made it possible to filter bookmarks by line and text content in Locator
  (QTCREATORBUG-21771)
* Fixed document sort order after rename (QTCREATORBUG-21565)

Help

* Improved context help in case of code errors or diagnostics
  (QTCREATORBUG-15959, QTCREATORBUG-21686)
* Improved lookup performance for context help

All Projects

* Added `Expand All` to context menu (QTCREATORBUG-17243)
* Added `Close All Files in Project` action (QTCREATORBUG-15593)
* Added closing of all files of a project when project is closed
  (QTCREATORBUG-15721)
* Added display of command line parameters to `Application Output`
  (QTCREATORBUG-20577)
* Fixed that dragging file from `Projects` view to desktop moved the file
  (QTCREATORBUG-14494)
* Fixed regression with `QTC_EXTENSION` environment variable

QMake Projects

* Fixed that adding files did not respect alphabetic sorting and indentation
  with tabs (QTCREATORBUG-553, QTCREATORBUG-21807)
* Fixed updating of `LD_LIBRARY_PATH` environment variable (QTCREATORBUG-21475)
* Fixed updating of project tree in case of wildcards in corresponding QMake
  variable (QTCREATORBUG-21603)
* Fixed issues with project tree when files are directly added to `RESOURCES`
  (QTCREATORBUG-20103)
* Fixed that importing build unnecessarily created temporary kit
  (QTCREATORBUG-18153)

CMake Projects

* Fixed that default build directory names contained spaces (QTCREATORBUG-18442)
* Fixed that build targets were reset on CMake parse error (QTCREATORBUG-21617)
* Fixed scroll behavior when adding configuration item

Qbs Projects

* Fixed crash when switching kits (QTCREATORBUG-21544)

Generic Projects

* Added deployment via `QtCreatorDeployment.txt` file (QTCREATORBUG-19202)
* Added setting C/C++ flags for the code model via `.cflags` and `.cxxflags`
  files (QTCREATORBUG-19668)
* Fixed `Apply Filter` when editing file list (QTCREATORBUG-16237)

Compilation Database Projects

* Fixed that project tree was not updated when database changes on disk
  (QTCREATORBUG-21733)

C++ Support

* Added code snippet for range-based `for` loops
* Added option to synchronize `Include Hierarchy` with current document
  (QTCREATORBUG-12022)
* Clang Code Model
    * Added buttons for copying and ignoring diagnostics to tooltip
    * Fixed issue with high memory consumption (QTCREATORBUG-19543)
    * Fixed inconsistency between `Follow Symbol` and `Ctrl + Click`
      (QTCREATORBUG-21637)
    * Fixed that global completion was shown after comma (QTCREATORBUG-21624)
* Clang Format
    * Added option to format code instead of only indenting code
    * Added `Open Used .clang-format Configuration File` to editor's
      context menu
    * Fixed indentation issue after empty line (QTCREATORBUG-22238)

QML Support

* Updated to parser from Qt 5.12, adding support for ECMAScript 7
  (QTCREATORBUG-20341, QTCREATORBUG-21301)
* Added Qt 5.13 as option to the wizards
* Improved error handling in Qt Quick Application project template (QTBUG-39469)
* Fixed crash on `Find Usages`

Python

* Added project templates for Qt for Python

Nim Support

* Added code completion based on `NimSuggest`

Debugging

* Added pretty printing of `QSizePolicy`
* Fixed that debugger toolbar could force large minimum window size
  (QTCREATORBUG-21885)
* Fixed restoring of debugger layout (QTCREATORBUG-21083)
* Fixed pretty printing of standard maps and sets from `libc++`
  (QTCREATORBUG-18536)
* GDB
    * Added support for rvalue references in function arguments
    * Fixed `Break on Abort` with GDB > 8.1 (QTBUG-73993)
* LLDB
    * Fixed `Source Paths Mappings` functionality (QTCREATORBUG-17468)
* QML
    * Fixed loading QML stack (QTCREATORBUG-22209)

Clang Analyzer Tools

* Made Clazy configuration options more fine grained (QTCREATORBUG-21120)
* Improved Fix-its handling in case of selecting multiple diagnostics and
  after editing files
* Added diagnostics from header files (QTCREATORBUG-21452)
* Added sorting to result list (QTCREATORBUG-20660)
* Fixed that files were analyzed that are not part of current build
  configuration (QTCREATORBUG-16016)

Perf Profiler

* Made Perf profiler integration opensource

Qt Quick Designer

* Made QML Live Preview integration opensource
* Added support for `Dialog` (QTCREATORBUG-22120)
* Fixed layout icons (QDS-538)
* Fixed crash when creating item inside `TabView` tab (QTCREATORBUG-21542)

Version Control Systems

* Git
    * Improved messages when submit editor validation fails and when editor
      is closed
    * Added `Subversion` > `DCommit`
    * `Branches` View
        * Added `Push` action
        * Added entry for detached `HEAD` (QTCREATORBUG-21311)
        * Added tracking of external changes to `HEAD` (QTCREATORBUG-21089)
* Subversion
    * Improved handling of commit errors (QTCREATORBUG-15227)
* Perforce
    * Disabled by default
    * Fixed issue with setting P4 environment variables (QTCREATORBUG-18771)
* Mercurial
    * Added side-by-side diff viewer (QTCREATORBUG-21124)

Test Integration

* Added `Uncheck All Filters`
* Added grouping results by application (QTCREATORBUG-21740)
* QTest
    * Added support for `BXPASS` and `BXFAIL`
    * Fixed parsing of `BFAIL` and `BPASS`

FakeVim

* Added option for blinking cursor (QTCREATORBUG-21613)
* Added closing completion popups with `Ctrl+[` (QTCREATORBUG-21886)

Model Editor

* Added display of base class names

Serial Terminal

* Improved error message on connection failure

Platform Specific

Windows

* Added support for MSVC 2019
* Changed toolchain detection to use `vswhere` by default, which is recommended
  by Microsoft
* Fixed issue with UNC paths in `.pro` files (QTCREATORBUG-21881)
* Fixed language version detections with MSVC and precompiled headers
  (QTCREATORBUG-21860)
* Fixed submenu arrow styling (QTCREATORBUG-21376)

Linux

macOS

* Added support for Touch Bar (QTCREATORBUG-21263)

Android

* Removed separate `QmakeAndroidSupport` plugin and merged functionality into
  other plugins
* Fixed debugging for API level 22 (QTCREATORBUG-22098)

Remote Linux

* Removed use of Botan, exchanging it by use of separately installed OpenSSH
  tools (QTCREATORBUG-15744, QTCREATORBUG-15807, QTCREATORBUG-19306,
  QTCREATORBUG-20210)
* Added support for `ssh-askpass`
* Added optional deployment of public key for authentication to device setup
  wizard
* Added support for X11 forwarding
* Added `rsync` based deployment method
* Added support for `Run in Terminal`
* Added support for opening a remote terminal from device settings
* Fixed incremental deployment when target directory is changed
  (QTCREATORBUG-21225)
* Fixed issue with killing remote process (QTCREATORBUG-19941)

Boot to Qt

* Removed ADB-based Boot to Qt plugin that provided support for
  Boot to Qt versions 5.8, and earlier.

Credits for these changes go to:  
Aaron Barany  
Alessandro Portale  
Andre Hartmann  
André Pönitz  
Asit Dhal  
Bernhard Beschow  
Chris Rizzitello  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
Cristian Maureira-Fredes  
Daniel Wingerd  
David Schulz  
Eike Ziller  
Filip Bucek  
Filippo Cucchetto  
Frank Meerkoetter  
Friedemann Kleint  
Ivan Donchevskii  
James McDonnell  
Jochen Becher  
Kai Köhne  
Leena Miettinen  
Marco Benelli  
Marco Bubke  
Michael Kopp  
Michael Weghorn  
Miklós Márton  
Mitch Curtis  
Nikolai Kosjar  
Oliver Wolff  
Orgad Shaneh  
Przemyslaw Gorszkowski  
Robert Löhning  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Tobias Hunger  
Ulf Hermann  
Vikas Pachdha  
Ville Nummela  
Xiaofeng Wang  
