Qt Creator version 4.8.2 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.8.1..v4.8.2

General

* Fixed UI for choosing executable for external tools (QTCREATORBUG-21937)

Editing

* Fixed highlighting of search results of regular expression search
  (QTCREATORBUG-21887)

Autotools Projects

* Fixed that includes, defines and flags of `SUBDIRS` were ignored
  (QTCREATORBUG-21618)

C++ Support

* Fixed crash when expanding macros (QTCREATORBUG-21642)
* Fixed crash in preprocessor (QTCREATORBUG-21981)
* Fixed infinite loop when resolving pointer types (QTCREATORBUG-22010)
* Fixed cursor position after completion of functions without arguments
  (QTCREATORBUG-21841)

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
* Fixed crash with gradients and Qt Quick 5.12 (QDS-472)

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
* Fixed crash on exiting debugger (QTCREATORBUG-21684)

Credits for these changes go to:  
Andre Hartmann  
André Pönitz  
Christian Kandeler  
Christian Stenger  
David Schulz  
Eike Ziller  
Ivan Donchevskii  
Kirill Burtsev  
Leena Miettinen  
Liang Qi  
Nikolai Kosjar  
Oliver Wolff  
Raoul Hecky  
Robert Löhning  
Thomas Hartmann  
Vikas Pachdha  
