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

Help
----

* Made `litehtml` rendering backend the default

Editing
-------

* Added option to adjust line spacing (QTCREATORBUG-13727)

### C++

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
* Restricted completion for second argument of `connect` calls to signals (QTCREATORBUG-13558)
* Fixed duplicate items appearing in include completion (QTCREATORBUG-24515)
* Fixed missing namespace when generating getters and setters (QTCREATORBUG-14886)
* Fixed missing `inline` when generating method definitions in header files
  (QTCREATORBUG-15052)
* Fixed that `Follow Symbol Under Cursor` on declarations and definitions did not offer items
  in subclasses (QTCREATORBUG-10160)
* Fixed that `RESET` function was not generated for `Q_PROPERTY`s (QTCREATORBUG-11809)
* Fixed that `Insert virtual functions of base class` refactoring action added already
  implemented operators (QTCREATORBUG-12218)
* Fixed that `Complete switch statement` indents unrelated code (QTCREATORBUG-12445)
* Fixed that `Apply function signature change` removed return values from `std::function`
  arguments (QTCREATORBUG-13698)
* Fixed handling of multiple inheritance in `Insert Virtual Functions` (QTCREATORBUG-12223)
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

### GLSL

* Updated language specification (QTCREATORBUG-24068)

Projects
--------

* Renamed `CurrentProject:*` variables to `CurrentDocument:Project:*` (QTCREATORBUG-12724,
  QTCREATORBUG-24606)
* Fixed issue when environment changes after appending or prepending path (QTCREATORBUG-24105)
* Fixed `Embedding of the UI Class` option for widget applications (QTCREATORBUG-24422)
* Fixed shell used for console applications (QTCREATORBUG-24659)

### Wizards

* Fixed creation of form editor class with namespace (QTCREATORBUG-24723)

### CMake

* Added option to unselect multiple configuration variables simultaneously
  (QTCREATORBUG-22659)

### Meson

* Fixed updating of introspection data after reconfiguration

Debugging
---------

* Fixed disabling and enabling breakpoints (QTCREATORBUG-24669)

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

Platforms
---------

### macOS

* Fixed type display when debugging with newest LLDB

### Android

* Improved manifest editor
    * Added support for `xhdpi`, `xxhdpi` and `xxxhdpi` icons and splashscreens
    * Added support for setting preferred screen orientation
* Added missing Android variables to completion in `.pro` and `.pri` files
* Fixed passing command line arguments to application (QTCREATORBUG-23712)

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Alexander Mishin  
Alexis Jeandet  
Andre Hartmann  
André Pönitz  
Antonio Di Monaco  
Asit Dhal  
Assam Boudjelthia  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Denis Shienkov  
Eike Ziller  
Fabio Falsini  
Fawzi Mohamed  
Federico Guerinoni  
Henning Gruendl  
Jeremy Ephron  
Kai Köhne  
Knud Dollereder  
Lars Knoll  
Leander Schulten  
Leena Miettinen  
Lukasz Ornatek  
Mahmoud Badri  
Martin Kampas  
Michael Weghorn  
Miikka Heikkinen  
Miklós Márton  
Morten Johan Sørvig  
Orgad Shaneh  
Robert Löhning  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Tobias Hunger  
Vikas Pachdha  
Volodymyr Zibarov  
Wojciech Smigaj  
