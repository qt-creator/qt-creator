Qt Creator version 4.8.2 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.8.1..v4.8.2

Editing

* Fixed highlighting of search results of regular expression search
  (QTCREATORBUG-21887)

Autotools Projects

* Fixed that includes, defines and flags of `SUBDIRS` were ignored
  (QTCREATORBUG-21618)

C++ Support

* Fixed crash when expanding macros (QTCREATORBUG-21642)

QML Support

* Fixed auto-insertion of single quotes

Debugging

* GDB
    * Fixed detaching from process (QTCREATORBUG-21908)
* LLDB
    * Fixed stopping at some breakpoints with newer LLDB (QTCREATORBUG-21615)
    * Fixed `Attach to Process` and `Run in Terminal` with newer LLDB
* CDB
    * Fixed display of `QDateTime` (QTCREATORBUG-21864)

Qt Quick Designer

* Added support for more JavaScript functions in `.ui.qml` files

Test Integration

* Fixed handling of empty tests

Platform Specific

macOS

* Fixed crash when file change dialog is triggered while another modal dialog
  is open
* Fixed running of user applications that need access to camera, microphone or
  other restricted services on macOS 10.14 (QTCREATORBUG-21887)

Android

* Fixed upload of GDB server on some devices (QTCREATORBUG-21317)

Credits for these changes go to:  
André Pönitz  
Christian Kandeler  
Christian Stenger  
David Schulz  
Eike Ziller  
Ivan Donchevskii  
Leena Miettinen  
Liang Qi  
Oliver Wolff  
Raoul Hecky  
Robert Löhning  
Thomas Hartmann  
Vikas Pachdha  
