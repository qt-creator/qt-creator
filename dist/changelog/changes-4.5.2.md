Qt Creator version 4.5.2 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v4.5.1..v4.5.2

General

* Added workaround for Qt bug that made summary progress bar and some other
  controls disappear (QTCREATORBUG-19716)

All Projects

* Improved automatic selection of tool chain for desktop kits when a
  cross-compilation tool chain has the same ABI as the host
