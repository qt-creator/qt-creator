Qt Creator version 4.0 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/3.6..origin/4.0

General

* Changed licensing to GPLv3 with exception
* Made commercial-only features available as opensource:
    * Test integration
    * Clang static analyzer integration
    * QML Profiler extensions
* Merged Debug and Analyze modes
* Added support for using `git grep` for file system search
  (QTCREATORBUG-3556)
* Fixed issues with HiDPI (QTCREATORBUG-15222)
* Fixed that switching theme did not switch editor color scheme
  (QTCREATORBUG-15229)

Editing

* Added support for `(<linenumber>)` after file names when opening files
  (QTCREATORBUG-14724)
* Added `Go to Previous Split or Window`
* Fixed whitespace cleaning for mixed tabs and spaces configurations
  (QTCREATORBUG-7994)

Help

* Fixed issues with scrolling to right position (QTCREATORBUG-15548)

QMake Projects

* Added wizard for creating `Qt Labs Controls Application`
* Added support for `STATECHARTS`

CMake Projects

* Changed CMake to run automatically in the background
* Added CMake generator setting per kit
* Added CMake configuration setting per kit and build configuration
* Added reading of existing `CMakeCache.txt`
* Added parsing of CMake errors
* Changed building to use `cmake --build`
* Fixed that `clean` target could be missing
* Fixed issue with mapping source files to targets (QTCREATORBUG-15825)

Qbs Projects

* Improved performance when opening large projects
* Added support for SCXML files

Qt Support

* Added C++ and QML code model support for `SCXML` files via `qscxmlc`
* Fixed that moc notes were reported as errors

C++ Support

* Fixed issue with negative enum values
* Clang code model
    * Simplified activation (it is now active if the plugin is enabled)
    * Added customizable configurations for warnings (global and per project)
    * Added light bulb for Clang's Fix-its
    * Fixed that child diagnostics were not shown in tool tip

QML Support

* Fixed various issues with QML/JS Console (QTCREATORBUG-14931)

Debugging

* Added pretty printers for `std::set`, `std::map` and `std::multimap`
  (for simple types of keys and values)
* Improved performance for watches
* Improved visualization of `QByteArray` and `char[]` with non-printable
  values (QTCREATORBUG-15549)
* CDB
    * Fixed showing value of `std::[w]string` (QTCREATORBUG-15016)

QML Profiler

* Added visualizing statistics as flame graphs
* Added support for additional input event attributes
* Added zooming timeline with `Ctrl + mouse wheel`
* Added `self time` to events
* Renamed `Events View` to `Statistics View`
* Fixed that zooming time line moved it to different location
  (QTCREATORBUG-15440)

Clang Static Analyzer

* Fixed analyzing with MinGW tool chain settings

Test Integration

* Added searching through test results
* Fixed resolution of source file of failed test on Windows (QTCREATORBUG-15667)
* Fixed that additional output of passing tests was ignored
* Fixed test detection with CMake projects (QTCREATORBUG-15813)
* Google Test
  * Added support for typed tests
  * Fixed parsing of file and line information

Qt Quick Designer

* Added `Space + mouse drag` for dragging design area (QTCREATORBUG-11321)
* Added dialog for adding signal handlers
* Fixed `Always save when leaving subcomponent in bread crumb`

Version Control Systems

* Git
    * Increased minimum Git version to 1.8.0
    * Fixed missing update of file list in commit editor after merging files
      (QTCREATORBUG-15569)
    * Added optional hiding of branches without activity for 90 days to Branches
      dialog (QTCREATORBUG-15544)

Diff Viewer

* Added scrolling to file when showing a change from file log

Custom Wizards

* Added that directories are allowed as file generator source

FakeVim

* Fixed replacing with special characters (QTCREATORBUG-15512)
* Fixed issue with `Ctrl+[` (QTCREATORBUG-15261)

TODO

* Added searching through TODO entries

Model Editor

* Added exporting diagrams as PNG, PDF, or SVG
  (Tools > Model Editor > Export Diagram)
* Added support for model specific configuration
* Added automatic showing of class members
* Added in-place editing of item names
* Fixed issue with special characters in class member declarations
* Fixed support for static members

Platform Specific

Windows

* Added auto-detection of Clang tool chain (QTCREATORBUG-15641)

Linux

* Changed default terminal to `x-terminal-emulator`
* Fixed notification for externally modified files while modal dialog is open
  (QTCREATORBUG-15687)

Android

* Fixed issues with `Select Android Device`
  (QTCREATORBUG-15338, QTCREATORBUG-15422)

Credits for these changes go to:  
Alessandro Portale  
Alexandru Croitor  
Andre Hartmann  
André Pönitz  
Artem Chystikov  
Aurindam Jana  
BogDan Vatra  
Caspar Schutijser  
Christiaan Janssen  
Christian Kandeler  
Christian Stenger  
Daniel Teske  
David Schulz  
Denis Shienkov  
Dmytro Poplavskiy  
Eike Ziller  
Fathi Boudra  
Francois Ferrand  
Friedemann Kleint  
Jake Petroules  
Jesus Fernandez  
Jochen Becher  
Kai Köhne  
Leena Miettinen  
Lorenz Haas  
Lukas Holecek  
Marco Benelli  
Marco Bubke  
Marc Reilly  
Martin Kampas  
Mitch Curtis  
Niels Weber  
Nikita Baryshnikov  
Nikolai Kosjar  
Orgad Shaneh  
Oswald Buddenhagen  
Robert Löhning  
Svenn-Arne Dragly  
Takumi ASAKI  
Thomas Hartmann  
Tim Jenssen  
Tobias Hunger  
Tom Deblauwe  
Topi Reinio  
Ulf Hermann  
Yuchen Deng
