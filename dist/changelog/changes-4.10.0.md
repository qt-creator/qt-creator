# Qt Creator 4.10

Qt Creator version 4.10 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.9..v4.10.0

## Editing

* Removed support for KDE code paster after removal of official API
* Added option for pinning files so they stay open when closing all files (QTCREATORBUG-21899)
* Fixed low contrast of hovered folding markers (QTCREATORBUG-21702)
* Fixed infinite global search in case of symlink loop (QTCREATORBUG-22662)

### Generic Highlighter

* Fixed that highlighting definition with MIME type `text/plain`
  overrode better matching definitions (QTCREATORBUG-22540)

### Language Client

* Removed `Experimental` flag
* Added option for starting server when needed
* Added option for starting one server per project
* Added support for `workspace/workspaceFolders` server request
* Added Locator filter for current document (`.`)
* Added Locator filters for symbols in workspace (`:`, `c`, and `m`) (QTCREATORBUG-21915)
* Added support for showing tooltip information from server
* Made client settings expand variables for executable and arguments
* Improved completion item tooltip (QTCREATORBUG-22429)

## Help

* Added option for scroll wheel zooming (QTCREATORBUG-14154)

## All Projects

* Added option for hiding kit settings (QTCREATORBUG-9134)
* Added support for drag & drop in Projects tree (QTCREATORBUG-6446)
* Added option for closing files of project when closing project (QTCREATORBUG-22198)
* Added filtering to `Issues`, `Application Output`, `Compile Output`, and `General Messages`
  (QTCREATORBUG-16356)
* Added `Re-detect` and `Remove All` to compiler settings
* Added Locator filter for all files in all project directory trees (`a`) (QTCREATORBUG-19122)
* Added `CurrentRun:WorkingDir` Qt Creator variable
* Added `Tools` > `Parse Build Output` (QTCREATORBUG-16017)
* Added option for not clearing `Issues` pane on build (QTCREATORBUG-22478)
* Moved `Application Output` and `Build Output` options to separate tabs in the
  `Build & Run` options
* Improved search for files from `Issues` pane (QTCREATORBUG-13623)

### Wizards

* Added build system choice to `Qt Widgets Application` and `C++ Library` wizards
* Added `value('variablename')` to JavaScript context in JSON wizards, adding support for
  lists and dictionaries as values
* Fixed that file names were always lower-cased by file wizards (QTCREATORBUG-14711)

## QMake Projects

* Added option for adding existing project as sub-project (QTCREATORBUG-5837)
* Added option for running `qmake` on every build (QTCREATORBUG-20888)
* Added completion of paths in project files (QTCREATORBUG-5915)
* Added forced `qmake` run on rebuild
* Fixed building sub-project in case of additional custom make steps (QTCREATORBUG-15794)
* Fixed missing items from `OBJECTIVE_HEADERS` (QTCREATORBUG-17569)

## CMake Projects

* Removed `Default` from build types (QTCREATORBUG-22013)
* Added support for Android targets
* Added support for building single file (QTCREATORBUG-18898)
* Added completion of paths in project files (QTCREATORBUG-5915)
* Improved text in `Configuration has changed on disk` dialog (QTCREATORBUG-22059)

## Qbs Projects

* Added support for Android targets
* Fixed `Build product` for files in groups

## Python Projects

* Added support for adding and removing files from project
* Improved wizards

## Compilation Database Projects

* Added setting for project header path (QTCREATORBUG-22031)
* Added custom build steps and run configuration (QTCREATORBUG-21727)
* Added option for specifying additional files in `compile_database.json.files`
* Fixed handling of relative paths (QTCREATORBUG-22338)
* Fixed handling of `--sysroot` (QTCREATORBUG-22339)

## Qt Support

* Added handling of QtTest messages in compile output (QTCREATORBUG-8091)

## C++ Support

* Improved auto-insertion of closing curly brace (QTCREATORBUG-18872)
* Fixed that snippet completion could get in the way (QTCREATORBUG-21767)
* Fixed crash because of small stack size (QTCREATORBUG-22496)
* Fixed recognition of C++ version (QTCREATORBUG-22444)
* Fixed `unknown argument: '-fno-keep-inline-dllexport'` (QTCREATORBUG-22452)

### Clang Format

* Improved configuration UI
* Fixed that clang format was triggered on save when Beautifier already was as well

## QML Support

* Fixed various formatting issues
* Fixed incorrect syntax warning in JavaScript template literal
  (QTCREATORBUG-22474)

## Debugging

* Added pretty printer for `QMargin`
* Fixed pretty printers for `QFile`, `QStandardItem`,
  `std::vector` and `std::basic_string` with custom allocator, and `std::map<K,V>::iterator`
* Fixed issues with restoring layout (QTCREATORBUG-21669)

### LLDB

* Fixed running with command line arguments with spaces (QTCREATORBUG-22811)

### CDB

* Fixed loading of custom debugging helpers (QTCREATORBUG-20481)

## Clang Analyzer Tools

* Fixed display of diagnostic for files outside of project directory (QTCREATORBUG-22213)

## QML Profiler

* Improved behavior in case of slow connections (QTCREATORBUG-22641)

## Perf Profiler

* Changed format of saved traces
* Added support for multiple attributes per sample
* Added CPU ID for events

## Qt Quick Designer

* Added support for `ShapeGradient` (QDS-359)
* Added gradient picker that allows loading and saving of presets
* Added support for changing properties for multiple items at once (QDS-324)
* Added missing properties for `LineEdit` and `ComboBox`
* Added all fonts from project directory to font selector (QDS-100)
* Updated properties of `Flickable`
* Improved handling of errors in state editor (QDS-695)
* Improved selection behavior (QDS-853)

## Version Control Systems

* Added zoom buttons to `Version Control` output pane

### Git

* Added support for different reset types in `Branches` view
* Added choice of build system to `Git Clone` wizard if cloned project supports multiple
  build systems (QTCREATORBUG-17828)
* Fixed popping stash after checkout from `Branches` view

## Test Integration

* Added basic support for Boost tests
* Added wizard for Boost tests (QTCREATORBUG-21169)
* Added option for automatically opening test results pane
* Improved handling of unexpected test output (QTCREATORBUG-22354)

## Platform Specific

### Windows

* Added `Clone` for MSVC toolchains (QTCREATORBUG-22163)
* Fixed that `mingw32-make`'s warnings were categorized as errors (QTCREATORBUG-22171)
* Fixed bitness detection for MinGW (QTCREATORBUG-22160)
* Fixed registration as post mortem debugger on recent Windows versions

### Linux

* Improved auto-detection of toolchains (QTCREATORBUG-19179, QTCREATORBUG-20044, QTCREATORBUG-22081)

### Android

* Removed support for MIPS64

### iOS

* Fixed simulator detection with Xcode 11 (QTCREATORBUG-22757)

### Remote Linux

* Added deployment method that deploys everything that is installed by the build system
  in its install step (QTCREATORBUG-21855)
* Added support for opening remote terminal with run environment
* Added option for `rsync` flags for deployment (QTCREATORBUG-22352)
* Fixed deployment of files with `executable` `CONFIG` value (QTCREATORBUG-22663)
* Fixed `Unexpected stat output for remote file` (QTCREATORBUG-22603)

### Bare Metal

* Added include path detection and output parsers for `IAR`, `KEIL` and `SDCC` toolchains

## Credits for these changes go to:
Aleksei German  
Alessandro Ambrosano  
Alessandro Portale  
Andre Hartmann  
André Pönitz  
Anton Danielsson  
Antonio Di Monaco  
Asit Dhal  
BogDan Vatra  
Christian Gagneraud  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
Cristián Maureira-Fredes  
Daniel Teske  
David Schulz  
Denis Shienkov  
Denis Vygovskiy  
Eike Ziller  
Friedemann Kleint  
Giuseppe D'Angelo  
Haxor Leet  
Henning Gruendl  
illiteratecoder  
Ivan Donchevskii  
Ivan Komissarov  
Joel Smith  
Jörg Bornemann  
Kavindra Palaraja  
Knud Dollereder  
Leena Miettinen  
Luca Carlon  
Marc Mutz  
Marco Bubke  
Martin Haase  
Michael Weghorn  
Mitch Curtis  
Nikolai Kosjar  
Oliver Wolff  
Orgad Shaneh  
Przemyslaw Gorszkowski  
Robert Löhning  
Thiago Macieira  
Thomas Hartmann  
Thomas Otto  
Tim Henning  
Tim Jenssen  
Tobias Hunger  
Tor Arne Vestbø  
Uladzislau Paulovich  
Ulf Hermann  
Vikas Pachdha  
Ville Nummela  
