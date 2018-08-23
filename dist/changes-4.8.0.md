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

* Made it possible to change default editors in MIME type settings

Help

All Projects

* Added option for parallel jobs to `make` step, which is enabled by default
  if `MAKEFLAGS` are not set (QTCREATORBUG-18414)
* Added auto-detection of the Clang compiler shipped with Qt Creator
* Improved handling of relative file paths for custom error parsers
  (QTCREATORBUG-20605)
* Fixed that `make` step required C++ tool chain

QMake Projects

* Fixed that `make qmake_all` was run in top-level project directory even when
  building sub-project (QTCREATORBUG-20823)

C++ Support

* Clang Code Model
    * Added function overloads to tooltip in completion popup
    * Added `Build` > `Generate Compilation Database`
    * Fixed that braced initialization did not provide constructor completion
      (QTCREATORBUG-20957)
    * Fixed local references for operator arguments (QTCREATORBUG-20966)

QML Support

* Fixed indentation in object literals with ternary operator (QTCREATORBUG-7103)
* Fixed that symbols from closed projects were still shown in Locator
  (QTCREATORBUG-13459)

Python Support

Debugging

* GDB
    * Fixed startup issue with localized debugger output (QTCREATORBUG-20765)
    * Fixed disassembler view for newer GCC
* CDB
    * Added option to suppress task entries for exceptions (QTCREATORBUG-20915)
* LLDB
    * Fixed instruction-wise stepping

Clang Static Analyzer

QML Profiler

Version Control Systems

* Git
    * Improved behavior if no merge tool is configured
    * Fixed handling of `file://` remotes (QTCREATORBUG-20618)

Image Viewer

Test Integration

* Google Test
    * Fixed that not all failure locations were shown (QTCREATORBUG-20967)

Model Editor

* Fixed that selections and text cursors where exported (QTCREATORBUG-16689)

Platform Specific

Windows

Linux

* Added detection of Intel C compiler (QTCREATORBUG-18302)

macOS

* Fixed light themes for macOS Mojave (10.14)

Android

* Added support for command line arguments
* Added support for environment variables
* Fixed connecting to debugger for API level 24 and later

Credits for these changes go to:  
