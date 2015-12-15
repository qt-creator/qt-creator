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
* Fixed that closing session was leaving invalid editor windows open
  (QTCREATORBUG-15193)
* Fixed that editors were closing even when closing Qt Creator was cancelled
  (QTCREATORBUG-14401)
* Fixed zooming text with touch pads, which was too sensitive (QTBUG-49024)

Project Management

* Added actions for building without dependencies and for rebuilding
  and cleaning with dependencies to context menu of project tree
  (QTCREATORBUG-14606)
* Added option to synchronize kits between all projects in a session
  (QTCREATORBUG-5823)
* Fixed that `%{CurrentBuild:Type}` was not expanded correctly
  (QTCREATORBUG-15178)
* Fixed that `Stop applications before building` also stopped applications
  when deploying (QTCREATORBUG-15281)

QMake Projects

* Added a build configuration type for profiling
  (release build with separate debug information)
* Changed project display names to be `QMAKE_PROJECT_NAME` if set
  (QTCREATORBUG-13950)
* Fixed that `.pri` files were shown in flat list instead of tree
  (QTCREATORBUG-487)
* Fixed that `QMAKE_EXT_H` was ignored for UI code model (QTCREATORBUG-14910)
* Fixed that `make` build step was not updated on environment changes
  (QTCREATORBUG-14831)
* Fixed adding files to `.qrc` files through the project tree
  (QTCREATORBUG-15277)

CMake Projects

* Improved handling of projects with CMake errors (QTCREATORBUG-6903)
* Added option for `Debug`, `Release`, `ReleaseWithDebugInfo` and
  `MinSizeRelease` build types (QTCREATORBUG-12219)
* Added auto-indent and parentheses and quote matching to CMake editor

C++ Support

* Added support for `noexcept`
* Fixed crash with function arguments hint (QTCREATORBUG-15275)
* Fixed that object instantiation was sometimes highlighted as function call
  (QTCREATORBUG-15212)
* Clang code model
    * Added more diagnostic messages to editors
    * Added Clang's Fix-its to refactoring actions (QTCREATORBUG-14868)
    * Added option for additional command line arguments

Debugging

* Made sub-registers editable
* Fixed breakpoint removal from disassembler view (QTCREATORBUG-14973)
* CDB
    * Fixed auto-detection of CDB from Windows 10 Kits
* LLDB
    * Fixed handling of large registers
* QML/JS
    * Fixed that debugger stopped at disabled breakpoints (QTCREATORBUG-15395)
* QML/JS Console
    * Implemented lazy loading of sub-items
    * Improved error reporting
* GDB/MinGW
    * Fixed wrong `GDB not responding` message (QTCREATORBUG-14350)

Analyzer

* Improved diagnostics view to use tree view instead of list

QML Profiler

* Improved performance of timeline view (QTCREATORBUG-14983)
* Fixed offset when dragging timeline categories (QTCREATORBUG-15333)

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
* Fixed that `DYLD_LIBRARY_PATH`, `DYLD_FRAMEWORK_PATH` and `DYLD_IMAGE_SUFFIX`
  were not taking effect when debugging with recent LLDB

Android

* Added support for Android 6.0
* Fixed that QML syntax errors where not clickable in application output
  (QTCREATORBUG-14832)
* Fixed deployment on devices without `readlink` (QTCREATORBUG-15006)
* Fixed debugging of signed applications (requires Qt 5.6) (QTCREATORBUG-13035)
* Fixed that building failed if Java is not in `PATH` (QTCREATORBUG-15382)

iOS

* Improved error messages for deployment
* Fixed issues with profiling QML (QTCREATORBUG-15383)

Remote Linux

* Added support for ECDSA public keys with 384 and 521 bits,
  ECDSA user keys, and ECDSA key creation
* Fixed environment and working directory for Valgrind analyzer
* Fixed attaching to remote debugging server (QTCREATORBUG-15210)

Credits for these changes go to:  
Adam Strzelecki  
Aleix Pol  
Alessandro Portale  
Alexander Drozdov  
Allan Sandfeld Jensen  
André Hartmann  
André Pönitz  
Benjamin Zeller  
BogDan Vatra  
Christian Kandeler  
Christian Stenger  
Christian Strømme  
Claus Steuer  
Cristian Adam  
Daniel Teske  
David Fries  
David Schulz  
Davide Pesavento  
Denis Shienkov  
Eike Ziller  
Finn Brudal  
Friedemann Kleint  
J-P Nurmi  
Jake Petroules  
Jakub Golebiewski  
Jan Dalheimer  
Jarek Kobus  
Jean Gressmann  
Jochen Becher  
Jörg Bornemann  
Lassi Hämäläinen  
Leena Miettinen  
Lorenz Haas  
Marc Mutz  
Marco Benelli  
Marco Bubke  
Martin Kampas  
Maurice Kalinowski  
Mitch Curtis  
Montel Laurent  
Nico Vertriest  
Niels Weber  
Nikita Baryshnikov  
Nikolai Kosjar  
Oliver Wolff  
Orgad Shaneh  
Oswald Buddenhagen  
Robert Löhning  
Sergey Belyashov  
Sze Howe Koh  
Thiago Macieira  
Thomas Hartmann  
Thorbjørn Lindeijer  
Tim Jenssen  
Tobias Hunger  
Tom Deblauwe  
Topi Reinio  
Ulf Hermann  
Viktor Ostashevskyi  
Vladyslav Gapchych  
