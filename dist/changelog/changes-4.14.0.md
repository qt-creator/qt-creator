Qt Creator 4.14
===============

Qt Creator version 4.14 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.13..v4.14.0

General
-------

* Added option for asking for confirmation before closing (QTCREATORBUG-7637)
* Improved visibility of controls in dark themes (QTCREATORBUG-23505)
* Fixed lines disappearing in output panes (QTCREATORBUG-24556)

Help
----

* Made `litehtml` rendering backend the default
* Fixed that Qt 5 documentation was not available if Qt 6 is installed

Editing
-------

### C++

* Updated to LLVM 11
* Added refactoring action that creates getters and setters for all class members
  (QTCREATORBUG-14504)
* Added refactoring action that generates member from class member initialization
  (QTCREATORBUG-11842)
* Added refactoring action that creates implementations for all member functions
  (QTCREATORBUG-12164)
* Added refactoring action for removing `using namespace` directives (QTCREATORBUG-24392)
* Added auto-completion of existing namespaces and classes to `New Class` wizard
  (QTCREATORBUG-10066)
* Added action for showing function arguments hint (QTCREATORBUG-19394)
* Added option for after how many characters auto-completion may trigger (QTCREATORBUG-19920)
* Added highlighting for structured bindings (QTCREATORBUG-24769)
* Restricted completion for second argument of `connect` calls to signals (QTCREATORBUG-13558)
* Fixed crash of backend with multiline `Q_PROPERTY` declarations (QTCREATORBUG-24746)
* Fixed issues with include completion (QTCREATORBUG-21490, QTCREATORBUG-24515)
* Fixed missing namespace when generating getters and setters (QTCREATORBUG-14886)
* Fixed missing `inline` when generating method definitions in header files
  (QTCREATORBUG-15052)
* Fixed that `Follow Symbol Under Cursor` on declarations and definitions did not offer items
  in subclasses (QTCREATORBUG-10160)
* Fixed that `RESET` function was not generated for `Q_PROPERTY`s (QTCREATORBUG-11809)
* Fixed that `Insert virtual functions of base class` refactoring action added already
  implemented operators (QTCREATORBUG-12218)
* Fixed that `Complete switch statement` indents unrelated code (QTCREATORBUG-12445)
* Fixed `Complete switch statement` with templates (QTCREATORBUG-24752)
* Fixed `Complete switch statement` for enum classes (QTCREATORBUG-20475)
* Fixed creating and moving template member function definitions (QTCREATORBUG-24801,
  QTCREATORBUG-24848)
* Fixed that `Apply function signature change` removed return values from `std::function`
  arguments (QTCREATORBUG-13698)
* Fixed handling of multiple inheritance in `Insert Virtual Functions` (QTCREATORBUG-12223)
* Fixed issue with `Convert to Camel Case` (QTCREATORBUG-16560)
* Fixed auto-indentation for lambdas with trailing return type (QTCREATORBUG-18497)
* Fixed indentation when starting new line in documentation comments (QTCREATORBUG-11749)
* Fixed that auto-indentation was applied within multiline string literals
  (QTCREATORBUG-20180)
* Fixed sorting in `Outline` view (QTCREATORBUG-12714)
* Fixed that renaming files did not adapt include guards in headers (QTCREATORBUG-4686)

### Language Client

* Improved outline for hierarchical symbols

### QML

* Fixed issues with `Move Component into Separate File` (QTCREATORBUG-21091)
* Fixed crash with malformed `property` (QTCREATORBUG-24587)
* Fixed `qmldir` parsing with Qt 6 (QTCREATORBUG-24772)

### GLSL

* Updated language specification (QTCREATORBUG-24068)

Projects
--------

* Renamed `CurrentProject:*` variables to `CurrentDocument:Project:*` (QTCREATORBUG-12724,
  QTCREATORBUG-24606)
* Added `ActiveProject:*` variables (QTCREATORBUG-24878)
* Changed `Qt Creator Plugin` wizard to CMake build system (QTCREATORBUG-24073)
* Fixed issue when environment changes after appending or prepending path (QTCREATORBUG-24105)
* Fixed `Embedding of the UI Class` option for widget applications (QTCREATORBUG-24422)
* Fixed shell used for console applications (QTCREATORBUG-24659)
* Fixed issue with auto-scrolling compile output (QTCREATORBUG-24728)

### qmake

* Added option to not execute `system` directives (QTCREATORBUG-24551)
* Fixed deployment with wildcards (QTCREATORBUG-24695)

### Wizards

* Fixed creation of form editor class with namespace (QTCREATORBUG-24723)

### CMake

* Added option to unselect multiple configuration variables simultaneously
  (QTCREATORBUG-22659)
* Improved kit detection when importing build (QTCREATORBUG-25069)
* Fixed missing run of CMake when saving `CMakeLists.txt` files in
  subdirectories
* Fixed that changing build directory to existing build ran CMake with initial
  arguments
* Fixed that configuration changes were lost when done before triggering a first
  build (QTCREATORBUG-24936)
* Fixed `QML Debugging and Profiling`

### Meson

* Fixed updating of introspection data after reconfiguration

Debugging
---------

* Updated various pretty printers for Qt 6
* Fixed disabling and enabling breakpoints (QTCREATORBUG-24669)
* Fixed setting source mappings with variables (QTCREATORBUG-24816)

### GDB

* Fixed loading of symbol files with `Load Core File` (QTCREATORBUG-24541)

### CDB

* Fixed debugging when `PYTHONPATH` is set (QTCREATORBUG-24859)
* Fixed pretty printer of containers with signed chars

Analyzer
--------

### Clang

* Re-added automatic analyzation of files on save
* Added multi-selection in diagnostics view (QTCREATORBUG-24396)

Version Control Systems
-----------------------

* Improved removal of multiple files (QTCREATORBUG-24385)
* Added option to add file when creating it from locator (QTCREATORBUG-24168)

### Git

* Added option to show file at specified revision (QTCREATORBUG-24689)

### Gerrit

* Added suggestion for local branch name when checking out patch set (QTCREATORBUG-24006)
* Fixed commit list in `Push to Gerrit` (QTCREATORBUG-24436)

Test Integration
----------------

* Made it easier to re-run failed tests
* Added support for `QTest::addRow()` (QTCREATORBUG-24777)

Platforms
---------

### Linux

* Fixed initial directory when opening Konsole (QTCREATORBUG-24947)

### macOS

* Fixed type display when debugging with newest LLDB

### Android

* Improved manifest editor
    * Added support for `xhdpi`, `xxhdpi` and `xxxhdpi` icons and splashscreens
    * Added support for setting preferred screen orientation
* Added missing Android variables to completion in `.pro` and `.pri` files
* Fixed passing command line arguments to application (QTCREATORBUG-23712)
* Fixed fetching of logcat output when application crashes

### iOS

* Fixed persistence of signing settings (QTCREATORBUG-24586)

### Remote Linux

* Fixed password prompt missing with SSH (QTCREATORBUG-24979)

### MCU

* Improved creation of kits (QTCREATORBUG-24354, QTCREATORBUG-25052, QTCREATORBUG-25053)

Credits for these changes go to:
--------------------------------
Aleksei German  
Alessandro Portale  
Alexander Mishin  
Alexis Jeandet  
Andre Hartmann  
André Pönitz  
Antonio Di Monaco  
Asit Dhal  
Assam Boudjelthia  
Björn Schäpers  
Christiaan Janssen  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Dawid Sliwa  
Denis Shienkov  
Eike Ziller  
Fabio Falsini  
Fawzi Mohamed  
Federico Guerinoni  
Henning Gruendl  
Ivan Komissarov  
Jaroslaw Kobus  
Jeremy Ephron  
Jochen Seemann  
Johanna Vanhatapio  
Kai Köhne  
Knud Dollereder  
Lars Knoll  
Leander Schulten  
Leena Miettinen  
Lukas Holecek  
Lukasz Ornatek  
Mahmoud Badri  
Marco Bubke  
Martin Kampas  
Michael Weghorn  
Michael Winkelmann  
Miikka Heikkinen  
Miklós Márton  
Morten Johan Sørvig  
Orgad Shaneh  
Oswald Buddenhagen  
Raphaël Cotty  
Richard Weickelt  
Robert Löhning  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Tobias Hunger  
Venugopal Shivashankar  
Vikas Pachdha  
Ville Voutilainen  
Volodymyr Zibarov  
Wojciech Smigaj  
