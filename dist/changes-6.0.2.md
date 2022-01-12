Qt Creator 6.0.2
================

Qt Creator version 6.0.2 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v6.0.1..v6.0.2

General
-------

* Fixed crash in process launcher (QTCREATORBUG-26726)

Editing
-------

* Fixed that `Select All` scrolled to bottom (QTCREATORBUG-26736)
* Fixed copying with block selection (QTCREATORBUG-26761)

### C++

* ClangCodeModel
  * Fixed performance regression of code completion on Windows and macOS
    (QTCREATORBUG-26754)

### Python

* Fixed working directory for `REPL`

### Modeling

* Fixed missing options in property editor (QTCREATORBUG-26760)

Projects
--------

* Fixed that closing application in `Application Output` pane killed process
  even if `Keep Running` was selected
* Fixed filtering in target setup page (QTCREATORBUG-26779)

### CMake

* Fixed that GUI project wizards did not create GUI applications on Windows and
  app bundles on macOS

Platforms
---------

### Linux

* Fixed that commercial plugins linked to libGLX and libOpenGL
  (QTCREATORBUG-26744)

### macOS

* Fixed crash when switching screen configuration (QTCREATORBUG-26019)

Credits for these changes go to:
--------------------------------
Allan Sandfeld Jensen  
André Pönitz  
Antti Määttä  
Christiaan Janssen  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Henning Gruendl  
Jaroslaw Kobus  
Knud Dollereder  
Leena Miettinen  
Marco Bubke  
Mats Honkamaa  
Samuel Ghinet  
Tapani Mattila  
Thomas Hartmann  
Tuomo Pelkonen  
