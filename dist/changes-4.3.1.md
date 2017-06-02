Qt Creator version 4.3.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v4.3.0..v4.3.1

Version Control Systems

* Git
    * Fixed that ref names were missing for `Show`
* Gerrit
    * Fixed parsing output from Gerrit 2.14

Platform Specific

Windows

* Fixed checking whether example should be copied to writable location
  (QTCREATORBUG-18184)

WinRT

* Fixed running MSVC 2017 based applications (QTCREATORBUG-18288)

iOS

* Fixed crash that could occur at startup while device is connected
  (QTCREATORBUG-18226)
