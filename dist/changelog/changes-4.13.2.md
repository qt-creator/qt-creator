Qt Creator 4.13.2
=================

Qt Creator version 4.13.2 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.13.1..v4.13.2

Editing
-------

* Fixed annotation color for dark themes (QTCREATORBUG-24644)

Projects
--------

* Fixed missing removal of replacement kits (QTCREATORBUG-24589)
* Fixed issues with newlines in output windows (QTCREATORBUG-24668)

### qmake

* Fixed crash when parsing projects (QTCREATORBUG-23504)
* Fixed crash when re-parsing project (QTCREATORBUG-24683)

### Python

* Fixed working directory for run configurations (QTCREATORBUG-24440)

Qt Quick Designer
-----------------

* Improved connection editor dialog (QDS-2498, QDS-2495, QDS-2496)
* Fixed list model editing (QDS-2696)
* Fixed state editor updating (QDS-2798)

Test Integration
----------------

### Catch2

* Fixed file information on Windows with CMake

Platforms
---------

### Web Assembly

* Fixed missing C toolchains

Credits for these changes go to:
--------------------------------
Aleksei German  
Alessandro Portale  
Christian Kandeler  
Christian Stenger  
Corey Pendleton  
David Schulz  
Eike Ziller  
Fawzi Mohamed  
Henning Gruendl  
Jacek Nijaki  
Johanna Vanhatapio  
Kai Köhne  
Leena Miettinen  
Lukasz Ornatek  
Marco Bubke  
Thomas Hartmann  
Venugopal Shivashankar  
