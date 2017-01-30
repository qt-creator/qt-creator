Qt Creator version 4.2.2 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v4.2.1..v4.2.2

Qt Support

* Fixed ABI detection of static Qt builds

Qbs Projects

* Fixed duplicate include paths (QTCREATORBUG-17381)

Version Control Systems

* Gerrit
    * Fixed crash when committing and pushing to Gerrit (QTCREATORBUG-17634)
