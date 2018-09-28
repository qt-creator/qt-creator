Qt Creator version 4.8 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.7..v4.8.0

General

* Added `HostOs:PathListSeparator` and `HostOs:ExecutableSuffix` Qt Creator
  variables
* Added `Create Folder` to context menu of path choosers if the path does not
  exist
* Fixed menu items shown in menu locator filter (QTCREATORBUG-20071,
  QTCREATORBUG-20626)

Editing

* Added experimental plugin `LanguageClient` for supporting the [language server
  protocol](https://microsoft.github.io/language-server-protocol)
  (QTCREATORBUG-20284)
* Added support for the pastecode.xyz code pasting service
* Made it possible to change default editors in MIME type settings

All Projects

* Added option for parallel jobs to `make` step, which is enabled by default
  if `MAKEFLAGS` are not set (QTCREATORBUG-18414)
* Added auto-detection of the Clang compiler shipped with Qt Creator
* Added option for disabling automatic creation of run configurations
  (QTCREATORBUG-18578)
* Added option to open terminal with build or run environment to project tree
  and the corresponding configuration widgets in `Projects` mode
  (QTCREATORBUG-19692)
* Improved handling of relative file paths for custom error parsers
  (QTCREATORBUG-20605)
* Fixed that `make` step required C++ tool chain
* Fixed that many very long lines in application or build output could lead to
  out of memory exception (QTCREATORBUG-18172)

QMake Projects

* Fixed that `make qmake_all` was run in top-level project directory even when
  building sub-project (QTCREATORBUG-20823)

Qbs Projects

* Added `qmlDesignerImportPaths` property for specifying QML import paths for
  Qt Quick Designer (QTCREATORBUG-20810)

C++ Support

* Added experimental plugin `CompilationDatabaseProjectManager` that opens a
  [compilation database](https://clang.llvm.org/docs/JSONCompilationDatabase.html)
  for code editing
* Added experimental plugin `ClangFormat` that bases auto-indentation on
  Clang Format
* Added experimental plugin `Cppcheck` for integration of
  [cppcheck](http://cppcheck.sourceforge.net) diagnostics
* Added highlighting style for punctuation tokens (QTCREATORBUG-20666)
* Clang Code Model
    * Added `Follow Symbol` for `auto` keyword (QTCREATORBUG-17191)
    * Added function overloads to tooltip in completion popup
    * Added `Build` > `Generate Compilation Database`
    * Fixed that braced initialization did not provide constructor completion
      (QTCREATORBUG-20957)
    * Fixed local references for operator arguments (QTCREATORBUG-20966)

QML Support

* Fixed indentation in object literals with ternary operator (QTCREATORBUG-7103)
* Fixed that symbols from closed projects were still shown in Locator
  (QTCREATORBUG-13459)

Debugging

* Added support for multiple simultaneous debugger runs
* Fixed automatic detection of debugging information for Qt from binary
  installer (QTCREATORBUG-20693)
* GDB
    * Fixed startup issue with localized debugger output (QTCREATORBUG-20765)
    * Fixed disassembler view for newer GCC
* CDB
    * Added option to suppress task entries for exceptions (QTCREATORBUG-20915)
* LLDB
    * Fixed instruction-wise stepping

Qt Quick Designer

* Fixed wrong property propagation from parent to child

Version Control Systems

* Git
    * Added navigation pane that shows branches
    * Added option for copy/move detection to `git blame` (QTCREATORBUG-20462)
    * Improved behavior if no merge tool is configured
    * Fixed that `git pull` blocked Qt Creator (QTCREATORBUG-13279)
    * Fixed handling of `file://` remotes (QTCREATORBUG-20618)
    * Fixed search for `gitk` executable (QTCREATORBUG-1577)

Test Integration

* Google Test
    * Fixed that not all failure locations were shown (QTCREATORBUG-20967)
    * Fixed that `GTEST_*` environment variables could break test execution
      and output parsing (QTCREATORBUG-21012)

Model Editor

* Fixed that selections and text cursors where exported (QTCREATORBUG-16689)

Platform Specific

Linux

* Added detection of Intel C compiler (QTCREATORBUG-18302)
* Fixed `Open Terminal Here` for `konsole` (QTCREATORBUG-20900)

macOS

* Fixed light themes for macOS Mojave (10.14)

Android

* Added support for command line arguments
* Added support for environment variables
* Fixed connecting to debugger for API level 24 and later

Credits for these changes go to:  
Alessandro Portale  
Alexandru Croitor  
Alexis Jeandet  
Andre Hartmann  
André Pönitz  
Christian Kandeler  
Christian Stenger  
Daniel Trevitz  
David Schulz  
Eike Ziller  
Frank Meerkoetter  
Hannes Domani  
Ivan Donchevskii  
Jaroslaw Kobus  
Jochen Becher  
Jörg Bornemann  
Leena Miettinen  
Marco Benelli  
Marco Bubke  
Michael Weghorn  
Morten Johan Sørvig  
Nicolas Ettlin  
Nikolai Kosjar  
Oliver Wolff  
Orgad Shaneh  
Razi Alavizadeh  
Robert Löhning  
Sergey Morozov  
Thomas Hartmann  
Tim Jenssen  
Tobias Hunger  
Uladzimir Bely  
Ulf Hermann  
Venugopal Shivashankar  
Vikas Pachdha  
