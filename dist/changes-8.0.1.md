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

* Delayed context menu tooltip action creation
* Cached the last QIcon created from a Utils::Icon

### C++

* Prevent opening unneeded generated ui header files in clangd
* Fixed documents getting opened in wrong clangd

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
* Fixed bug when cloning jLink gdb server provider

Debugging
---------

* Fixed bitfield display with Python 3


Credits for these changes go to:
--------------------------------
Alexander Akulich  
Alibek Omarov  
André Pönitz  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Dmitry Kovalev  
Eike Ziller  
Ivan Komissarov  
Jaroslaw Kobus  
Knud Dollereder  
Mahmoud Badri  
Marcus Tillmanns  
Mats Honkamaa  
Miikka Heikkinen  
Orgad Shaneh  
Oswald Buddenhagen  
Samuel Ghinet  
Thomas Hartmann  
Vikas Pachdha  
