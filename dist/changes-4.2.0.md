Qt Creator version 4.2 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.1..v4.2.0

General

* Added experimental editor for Qt SCXML
* Added pattern substitution for variable expansion
  `%{variable/pattern/replacement}` (and `%{variable//pattern/replacement}`
  for replacing multiple matches)
* Added default values for variable expansion (`%{variable:-default}`)
* Added Help > System Information for bug reporting purposes
  (QTCREATORBUG-16135)
* Added option to hide the central widget in Debug mode
* Fixed issues with output pane height
  (QTCREATORBUG-15986, QTCREATORBUG-16829)

Welcome

* Added keyboard shortcuts for opening recent sessions and projects
* Improved performance when many sessions are shown
* Fixed dropping files on Qt Creator when Welcome screen was visible
  (QTCREATORBUG-14194)

Editing

* Added action for selecting word under cursor (QTCREATORBUG-641)
* Fixed highlighting of Markdown files
  (QTCREATORBUG-16304)
* Fixed performance of cleaning whitespace (QTCREATORBUG-16420)
* Fixed selection color in help viewer for dark theme (QTCREATORBUG-16375)

Help

* Added option to open link and current page in window (QTCREATORBUG-16842)
* Fixed that no results could be shown in Locator (QTCREATORBUG-16753)

All Projects

* Reworked Projects mode UI
* Grouped all device options into one options category
* Added support for toolchains for different languages (currently C and C++)

QMake Projects

* Removed Qt Labs Controls wizard which is superseded by Qt Quick Controls 2
* Fixed that run button could spuriously stay disabled
  (QTCREATORBUG-16172, QTCREATORBUG-15583)
* Fixed `Open with Designer` and `Open with Linguist` for mobile and embedded Qt
  (QTCREATORBUG-16558)
* Fixed Add Library wizard when selecting library from absolute path or
  different drive (QTCREATORBUG-8413, QTCREATORBUG-15732, QTCREATORBUG-16688)
* Fixed issue with make steps in deploy configurations (QTCREATORBUG-16795)

CMake Projects

* Added support for CMake specific snippets
* Added support for platforms and toolsets
* Added warning for unsupported CMake versions
* Added drop down for selecting predefined values for properties
* Improved performance of opening project (QTCREATORBUG-16930)
* Made it possible to select CMake application on macOS
* Fixed that all unknown build target types were mapped to `ExecutableType`

Qbs Projects

* Made generated files available in project tree (QTCREATORBUG-15978)
* Fixed handling of generated files (QTCREATORBUG-16976)

C++ Support

* Added preview of images to tool tip on Qt resource URLs
* Added option to skip big files when indexing (QTCREATORBUG-16712)
* Fixed random crash in LookupContext (QTCREATORBUG-14911)
* Fixed `Move Definition to Class` for functions in template class and
  template member functions (QTCREATORBUG-14354)
* Fixed issues with `Add Declaration`, `Add Definition`, and
  `Move Definition Outside Class` for template functions
* Clang Code Model
    * Added notification for parsing errors in headers
    * Improved responsiveness of completion and highlighting

QML Support

* Fixed handling of circular dependencies (QTCREATORBUG-16585)

Debugging

* Added pretty printing of `QRegExp` captures, `QStaticStringData`,
  `QStandardItem`, `std::weak_ptr`, `std::__1::multiset`,
  and `std::pair`
* Added display of QObject hierarchy and properties in release builds
* Added support to pretty-print custom types without debug info
* Enhanced display of function pointers
* Improved pretty printing of QV4 types
* Made display of associative containers, pairs, and various smart
  pointers more compact
* Made creation of custom pretty printers easier
* Fixed pretty printing of `QFixed`
* Fixed scrolling in memory editor (QTCREATORBUG-16751)
* Fixed expansion of items in tool tip (QTCREATORBUG-16947)
* GDB
    * Fixed handling of built-in pretty printers from new versions of GDB
      (QTCREATORBUG-16758)
    * Fixed that remote working directory was used for local process
      (QTCREATORBUG-16211)
* LLDB
    * Added support for Qt Creator variables `%{...}` in startup commands
* CDB
    * Fixed display order of vectors in vectors (QTCREATORBUG-16813)
    * Fixed display of QList contents (QTCREATORBUG-16750)
* QML
    * Fixed that expansion state was reset when stepping
    * Fixed `Load QML Stack` with Qt 5.7 and later (QTCREATORBUG-17097)

QML Profiler

* Added option to show memory usage and allocations as flame graph
* Added option to show vertical orientation lines in timeline
  (click the time ruler)
* Separated compile events from other QML/JS events in statistics and
  flamegraph, since compilation can happen asynchronously

Qt Quick Designer

* Added completion to expression editor
* Added menu for editing `when` condition of states
* Added editor for managing C++ backend objects
* Added reformatting of `.ui.qml` files on save
* Added support for exporting single properties
* Added support for padding (Qt Quick 2.6)
* Added support for elide and various font properties to text items
* Fixed that it was not possible to give extracted components
  the file extension `.ui.qml`
* Fixed that switching from Qt Quick Designer failed to commit pending changes
  (QTCREATORBUG-14830)
* Fixed issues with pressing escape

Qt Designer

* Fixed that resources could not be selected in new form
  (QTCREATORBUG-15560)

Diff Viewer

* Added local diff for modified files in Qt Creator (`Tools` > `Diff` >
  `Diff Current File`, `Tools` > `Diff` > `Diff Open Files`)
  (QTCREATORBUG-9732)
* Added option to diff files when they changed on disk
  (QTCREATORBUG-1531)
* Fixed that reload prompt was shown when reverting change

Version Control Systems

* Gerrit
    * Fixed pushing to Gerrit when remote repository is empty
      (QTCREATORBUG-16780)

Test Integration

* Added option to disable crash handler when debugging
* Fixed that results were not shown when debugging (QTCREATORBUG-16693)
* Fixed that progress indicator sometimes did not stop

Model Editor

* Added zooming
* Added synchronization of selected diagram in diagram browser

Beautifier

* Fixed that beautifier was not enabled for Objective-C/C++ files
  (QTCREATORBUG-16806)

Platform Specific

Windows

* Added support for MSVC 2017
* Fixed that environment variables containing special characters were not
  passed correctly to user applications (QTCREATORBUG-17219)

macOS

* Fixed issue with detecting LLDB through `xcrun`

Android

* Added API level 24 for Android 7
* Improved stability of determination if application is running
* Fixed debugging on Android 6+ with NDK r11+ (QTCREATORBUG-16721)
* Fixed that running without deployment did not start emulator
  (QTCREATORBUG-10237)
* Fixed that permission model downgrade was not detected as error
  (QTCREATORBUG-16630)
* Fixed handling of minimum required API level (QTCREATORBUG-16740)

iOS

* Fixed simulator support with Xcode 8 (QTCREATORBUG-16942)
* Fixed that standard paths reported by QStandardPaths were wrong when
  running on simulator (QTCREATORBUG-13655)
* Fixed QML debugging on device (QTCREATORBUG-15812)

Remote Linux

* Fixed crash when creating SSH key pair (QTCREATORBUG-17349)

QNX

* Fixed QML debugging (QTCREATORBUG-17208)

Credits for these changes go to:  
Aaron Barany  
Alessandro Portale  
Alexander Drozdov  
Andre Hartmann  
André Pönitz  
Arnold Dumas  
Christian Kandeler  
Christian Stenger  
Daniel Langner  
Daniel Trevitz  
David Schulz  
Eike Ziller  
Florian Apolloner  
Francois Ferrand  
Friedemann Kleint  
Giuseppe D'Angelo  
Jake Petroules  
Jaroslaw Kobus  
Jochen Becher  
Konstantin Shtepa  
Kudryavtsev Alexander  
Leena Miettinen  
Louai Al-Khanji  
Marc Reilly  
Marco Benelli  
Marco Bubke  
Mitch Curtis  
Nazar Gerasymchuk  
Nikita Baryshnikov  
Nikolai Kosjar  
Orgad Shaneh  
Oswald Buddenhagen  
Øystein Walle  
Robert Löhning  
Serhii Moroz  
Takumi ASAKI  
Tasuku Suzuki  
Thomas Hartmann  
Tim Jenssen  
Tobias Hunger  
Ulf Hermann  
Vikas Pachdha  
