Qt Creator 8.0.1
================

Qt Creator version 8.0.1 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v8.0.0..v8.0.1

Editing
-------

### C++

Projects
--------

### Compilation Database

* Fixed project state when compilation database changes timestamp without
  changing contents

Platforms
---------

### macOS

* Fixed crash of user applications that access protected resources
  (QTCREATORBUG-27962)

### Boot2Qt

* Fixed crash when manually rebooting (QTCREATORBUG-27879)

### QNX

* Fixed debugger startup (QTCREATORBUG-27915)

### Baremetal

* Fixed running and debugging applications (QTCREATORBUG-27972)

Credits for these changes go to:
--------------------------------
Alibek Omarov  
André Pönitz  
Christian Kandeler  
Cristian Adam  
Eike Ziller  
Ivan Komissarov  
Jaroslaw Kobus  
Mahmoud Badri  
Mats Honkamaa  
Oswald Buddenhagen  
Thomas Hartmann  
