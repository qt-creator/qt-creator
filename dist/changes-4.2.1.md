Qt Creator version 4.2.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v4.2.0..v4.2.1

Editing

* Fixed that viewport could change unexpectedly when block selection was
  active but not visible in viewport (QTCREATORBUG-17475)

All Projects

* Fixed issue with upgrading tool chain settings in auto-detected kits

Qbs Projects

* Fixed that target OS defaulted to host OS if tool chain does not specify
  target OS (QTCREATORBUG-17452)

Generic Projects

* Fixed that project files were no longer shown in project tree

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
