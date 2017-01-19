Qt Creator version 4.2.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v4.2.0..v4.2.1

General

* Fixed `Open Command Prompt Here` on Windows (QTCREATORBUG-17439)

Editing

* Fixed that viewport could change unexpectedly when block selection was
  active but not visible in viewport (QTCREATORBUG-17475)

Help

* Fixed crash when using drag & drop with bookmarks (QTCREATORBUG-17547)

All Projects

* Fixed issue with upgrading tool chain settings in auto-detected kits
* Fixed crash when setting custom executable (QTCREATORBUG-17505)
* Fixed MSVC support on Windows Vista and earlier (QTCREATORBUG-17501)

QMake Projects

* Fixed wrong warning about incompatible compilers
* Fixed various issues with run configurations
  (QTCREATORBUG-17462, QTCREATORBUG-17477)
* Fixed that `OTHER_FILES` and `DISTFILES` in subdirs projects were no longer
  shown in project tree (QTCREATORBUG-17473)
* Fixed crash caused by unnormalized file paths (QTCREATORBUG-17364)

Qbs Projects

* Fixed that target OS defaulted to host OS if tool chain does not specify
  target OS (QTCREATORBUG-17452)

Generic Projects

* Fixed that project files were no longer shown in project tree

C++ Support

* Fixed crash that could happen when using `auto` (QTCREATORBUG-16731)

Debugging

* Fixed issue with infinite message boxes being displayed
  (QTCREATORBUG-16971)
* Fixed `QObject` property extraction with namespaced Qt builds

Platform Specific

Windows

* Fixed detection of MSVC 2017 RC as MSVC 2017
* Fixed that environment detection could time out with MSVC
  (QTCREATORBUG-17474)

iOS

* Fixed that starting applications in simulator could fail, especially with
  iOS 10 devices (QTCREATORBUG-17336)

Android

* Fixed that password prompt was not shown again after entering invalid
  keystore password (QTCREATORBUG-17317)
