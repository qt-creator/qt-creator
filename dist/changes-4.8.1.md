Qt Creator version 4.8.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.8.0..v4.8.1

General

* Fixed too large minimum size of preferences dialog (QTCREATORBUG-21678)

Editing

* Fixed that text marks could vanish (QTCREATORBUG-21628)
* Fixed wrong background color for some text highlighting items
  (QTCREATORBUG-21661)
* Fixed handling of system text encoding on Windows (QTCREATORBUG-21622)
* Language Client
    * Fixed crash after failed server restarts (QTCREATORBUG-21635)

All Projects

* Fixed crash when renaming file in file system view (QTCREATORBUG-21741)
* Fixed that `Create suitable run configurations automatically` setting was not
  saved (QTCREATORBUG-21796)

QMake Projects

* Fixed handling of `unversioned_libname` (QTCREATORBUG-21687)

C++ Support

* Clang Code Model
    * Fixed Clang backend crashes when `bugprone-suspicious-missing-comma` check
      is enabled (QTCREATORBUG-21605)
    * Fixed that `Follow Symbol` could be triggered after already moving to a
      different location
    * Fixed tooltip for pointer variables (QTCREATORBUG-21523)
    * Fixed issue with multi-line completion items (QTCREATORBUG-21600)
    * Fixed include order issue that could lead to issues with C++ standard
      headers and intrinsics
    * Fixed highlighting of lambda captures (QTCREATORBUG-15271)
    * Fixed issues with parsing Boost headers
      (QTCREATORBUG-16439, QTCREATORBUG-21685)

* Clang Format
    * Fixed handling of tab size (QTCREATORBUG-21280)

Debugging

* Fixed `Switch to previous mode on debugger exit` (QTCREATORBUG-21415)
* Fixed infinite loop that could happen when adding breaking on non-source line
  (QTCREATORBUG-21611, QTCREATORBUG-21616)
* Fixed that debugger tooltips were overridden by editor tooltips
  (QTCREATORBUG-21825)
* Fixed pretty printing of multi-dimensional C-arrays (QTCREATORBUG-19356,
  QTCREATORBUG-20639, QTCREATORBUG-21677)
* Fixed issues with pretty printing and typedefs (QTCREATORBUG-21602,
  QTCREATORBUG-18450)
* Fixed updating of breakpoints when code changes
* CDB
    * Fixed `Step Into` after toggling `Operate by Instruction`
      (QTCREATORBUG-21708)

Test Integration

* Fixed display of UTF-8 characters (QTCREATORBUG-21782)
* Fixed issues with custom test macros (QTCREATORBUG-19910)
* Fixed source code links for test failures on Windows (QTCREATORBUG-21744)

Platform Specific

Android

* Fixed `ANDROID_NDK_PLATFORM` setting for ARMv8 (QTCREATORBUG-21536)
* Fixed debugging on ARMv8
* Fixed crash while detecting supported ABIs (QTCREATORBUG-21780)

Credits for these changes go to:  
Aaron Barany  
Andre Hartmann  
André Pönitz  
Andy Shaw  
Christian Kandeler  
Christian Stenger  
David Schulz  
Eike Ziller  
Haxor Leet  
Ivan Donchevskii  
Knud Dollereder  
Leena Miettinen  
Marco Benelli  
Nikolai Kosjar  
Orgad Shaneh  
Robert Löhning  
Thomas Hartmann  
Tim Jenssen  
Vikas Pachdha  
