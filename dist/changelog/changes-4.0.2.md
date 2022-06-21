Qt Creator version 4.0.2 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v4.0.1..v4.0.2

Debugging

* CDB
    * Fixed that Qt Creator did not register as post-mortem debugger for
      64-bit applications (QTCREATORBUG-16386)
    * Fixed issues with `.pdb` files larger than 1GB
