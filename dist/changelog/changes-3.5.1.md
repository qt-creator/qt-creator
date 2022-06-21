Qt Creator version 3.5.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v3.5.0..v3.5.1

General

* Fixed dark theme for wizards (QTCREATORBUG-13395)
* Fixed that cancel button was ignored when wizards ask about overwriting files
  (QTCREATORBUG-15022)
* Added support for MSYS2 compilers and debuggers

Editing

* Fixed crashes with code completion (QTCREATORBUG-14991, QTCREATORBUG-15020)

Project Management

* Fixed that some context actions were wrongly enabled
  (QTCREATORBUG-14768, QTCREATORBUG-14728)

C++ Support

* Improved performance for Boost (QTCREATORBUG-14889, QTCREATORBUG-14741)
* Fixed that adding defines with compiler flag did not work with space after `-D`
  (QTCREATORBUG-14792)

QML Support

* Fixed that `.ui.qml` warnings accumulated when splitting (QTCREATORBUG-14923)

QML Profier

* Fixed that notes were saved but not loaded (QTCREATORBUG-15077)

Version Control Systems

* Git
    * Fixed encoding of log output
* Mercurial
    * Fixed crash when annotating (QTCREATORBUG-14975)

Diff Editor

* Fixed handling of mode changes (QTCREATORBUG-14963)

Platform Specific

Remote Linux

* Fixed wrong SSH key compatibility check

BareMetal

* Fixed that GDB server provider list did not update on host change
