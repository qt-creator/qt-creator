Qt Creator version 3.4.2 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v3.4.1..v3.4.2

Analyzer

* Fixed crash when opening context menu without data

Platform Specific

Windows

* Added support for MSVC 2015 (QTBUG-46344)

