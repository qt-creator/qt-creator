Qt Creator version 3.6.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v3.6.0..v3.6.1

Editing

* Fixed issues with setting font size (QTCREATORBUG-15608, QTCREATORBUG-15609)

Help

* Fixed opening external links (QTCREATORBUG-15491)

C++ Support

* Clang code model
    * Fixed crash when closing many documents fast (QTCREATORBUG-15532)
    * Fixed that HTML code was shown in completion tool tip (QTCREATORBUG-15630)
    * Fixed highlighting for using a namespaced type (QTCREATORBUG-15271)
    * Fixed highlighting of current parameter in function signature tool tip
      (QTCREATORBUG-15108)
    * Fixed that template parameters were not shown in function signature tool
      tip (QTCREATORBUG-15286)

Qt Support

* Fixed crash when updating code model for `.ui` files (QTCREATORBUG-15672)

QML Support

* Added Qt 5.6 as an option to the wizards

Debugging

* LLDB
    * Fixed that switching thread did not update stack view (QTCREATORBUG-15587)
* GDB/MinGW
    * Fixed editing values while debugging

Beautifier

* Fixed formatting with `clang-format`

Platform Specific

Windows

* Added detection of Microsoft Visual C++ Build Tools
* Fixed issue with console applications that run only for a short time
  `Cannot obtain a handle to the inferior: The parameter is incorrect`
  (QTCREATORBUG-13042)
* Fixed that debug messages could get lost after the application finished
  (QTCREATORBUG-15546)

Android

* Fixed issues with Gradle wrapper (QTCREATORBUG-15568)
