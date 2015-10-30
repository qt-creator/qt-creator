Qt Creator version 3.6 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/3.5..origin/3.6

General

* Added text zoom in application and compile output (QTCREATORBUG-12476)
* Fixed that context help was shown for current keyboard focus widget, even when
  a tool tip with help icon was visible (QTCREATORBUG-5345)

Editing

* Added experimental editor for UML-like diagrams (`ModelEditor` plugin)
* Made it possible to use Qt Creator variables in snippets
* Fixed indentation in block selection mode (QTCREATORBUG-12697)
* Fixed that Qt Creator tried to write auto-save files in read-only
  directories
* Fixed possible crash with code completion (QTCREATORBUG-14875)

Project Management

* Added actions for building without dependencies and for rebuilding
  and cleaning with dependencies to context menu of project tree
  (QTCREATORBUG-14606)
* Added option to synchronize kits between all projects in a session
  (QTCREATORBUG-5823)

QMake Projects

* Changed project display names to be `QMAKE_PROJECT_NAME` if set
  (QTCREATORBUG-13950)
* Fixed that `.pri` files were shown in flat list instead of tree
  (QTCREATORBUG-487)
* Fixed that `QMAKE_EXT_H` was ignored for UI code model (QTCREATORBUG-14910)
* Fixed that `make` build step was not updated on environment changes
  (QTCREATORBUG-14831)

CMake Projects

* Improved handling of projects with CMake errors (QTCREATORBUG-6903)
* Added option for `Debug`, `Release`, `ReleaseWithDebugInfo` and
  `MinSizeRelease` build types (QTCREATORBUG-12219)
* Added auto-indent and parentheses and quote matching to CMake editor

C++ Support

* Added support for `noexcept`
* Clang code model
    * Added more diagnostic messages to editors
    * Added Clang's Fix-its to refactoring actions (QTCREATORBUG-14868)

Debugging

* Made sub-registers editable
* Fixed breakpoint removal from disassembler view (QTCREATORBUG-14973)
* CDB
    * Fixed auto-detection of CDB from Windows 10 Kits
* LLDB
    * Fixed handling of large registers
* QML/JS Console
    * Implemented lazy loading of sub-items
    * Improved error reporting

Analyzer

* Improved diagnostics view to use tree view instead of list

QML Profiler

* Improved performance of timeline view (QTCREATORBUG-14983)

Qt Quick Designer

* Made Qt Quick Designer aware of QRC files in project
* Improved selection behavior with regard to z-order in form editor
  (QTCREATORBUG-11703)
* Added `Go to Implementation` action from `.ui.qml` file to its
  associated `.qml` file
* Added connection editor and path editor

Version Control Systems

* Subversion
    * Fixed encoding issues for commit message (QTCREATORBUG-14965)
* Perforce
    * Fixed showing differences of files in submit editor when using
      P4CONFIG (QTCREATORBUG-14538)

TODO

* Added option to show TODOs only for current sub-project

Platform Specific

Windows

* Fixed detection of `cygwin` ABIs

OS X

* Added option for file system case-sensitivity and made it case-insensitive by
  default (QTCREATORBUG-13507)
* Added option to set `DYLD_LIBRARY_PATH` and `DYLD_FRAMEWORK_PATH` in
  run configurations (QTCREATORBUG-14022)

Android

* Fixed that QML syntax errors where not clickable in application output
  (QTCREATORBUG-14832)
* Fixed deployment on devices without `readlink` (QTCREATORBUG-15006)
* Fixed debugging of signed applications (requires Qt 5.6) (QTCREATORBUG-13035)

iOS

* Improved error messages for deployment

Remote Linux

* Added support for ECDSA public keys with 384 and 521 bits,
  ECDSA user keys, and ECDSA key creation
* Fixed environment and working directory for Valgrind analyzer

Credits for these changes go to:  
Aleix Pol  
Alessandro Portale  
Alexander Drozdov  
Andre Hartmann  
André Pönitz  
Benjamin Zeller  
BogDan Vatra  
Christian Kandeler  
Christian Stenger  
Christian Strømme  
Claus Steuer  
Cristian Adam  
Daniel Teske  
David Schulz  
Eike Ziller  
Jake Petroules  
Jakub Golebiewski  
Jan Dalheimer  
Jarek Kobus  
Jean Gressmann  
Jochen Becher  
Leena Miettinen  
Lorenz Haas  
Marco Bubke  
Maurice Kalinowski  
Mitch Curtis  
Montel Laurent  
Niels Weber  
Nikita Baryshnikov  
Nikolai Kosjar  
Oliver Wolff  
Orgad Shaneh  
Oswald Buddenhagen  
Robert Loehning  
Sze Howe Koh  
Thiago Macieira  
Thomas Hartmann  
Thorbjørn Lindeijer  
Tim Jenssen  
Tobias Hunger  
Ulf Hermann  
Vladyslav Gapchych  
