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

Editing

* Made it possible to change default editors in MIME type settings

Help

All Projects

* Added option for parallel jobs to `make` step, which is enabled by default
  if `MAKEFLAGS` are not set (QTCREATORBUG-18414)
* Improved handling of relative file paths for custom error parsers
  (QTCREATORBUG-20605)
* Fixed that `make` step required C++ tool chain

QMake Projects

C++ Support

QML Support

* Fixed indentation in object literals with ternary operator (QTCREATORBUG-7103)
* Fixed that symbols from closed projects were still shown in Locator
  (QTCREATORBUG-13459)

Python Support

Debugging

Clang Static Analyzer

QML Profiler

Version Control Systems

* Git
    * Improved behavior if no merge tool is configured

Image Viewer

Test Integration

Platform Specific

Windows

Android

Credits for these changes go to:  
