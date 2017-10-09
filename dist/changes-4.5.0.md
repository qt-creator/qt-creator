Qt Creator version 4.5 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.4..v4.5.0

General

* Implemented "fuzzy" camel case lookup similar to code completion for locator
  (QTCREATORBUG-3111)
* Changed `File System` pane to tree view with top level directory selectable
  from `Computer`, `Home`, `Projects`, and individual project root directories
  (QTCREATORBUG-8305)

Editing

* Added shortcut for sorting selected lines

All Projects

* Added progress indicator to project tree while project is parsed
* Added support for changing the maximum number of lines shown in compile output
  (QTCREATORBUG-2200)

CMake Projects

* Added groups to CMake configuration UI
* Added option to change configuration variable types
* Fixed that value was removed when renaming configuration variable
  (QTCREATORBUG-17926)

C++ Support

* Fixed lookup of functions that differ only in const-ness of arguments
  (QTCREATORBUG-18475)
* Fixed detection of macros defined by tool chain for `C`
* Fixed that `Refactoring` context menu blocked UI while checking for available
  actions
* Clang Code Model
    * Added sanity check to `Clang Code Model Warnings` option
      (QTCREATORBUG-18864)
    * Fixed completion in `std::make_unique` and `std::make_shared` constructors
      (QTCREATORBUG-18615)
    * Fixed that function argument completion switched selected overload back to
      default after typing comma (QTCREATORBUG-11688)
* GCC
    * Improved auto-detection to include versioned binaries and cross-compilers

QML Support

* Added wizards with different starting UI layouts

Python Support

* Added simple code folding

Debugging

* Changed pretty printing of `QFlags` and bitfields to hexadecimal
* Fixed `Run in terminal` for debugging external application
  (QTCREATORBUG-18912)
* LLDB / macOS
    * Added pretty printing of Core Foundation and Foundation string-like types
      (QTCREATORBUG-18638)

QML Profiler

* Improved robustness when faced with invalid data

Qt Quick Designer

* Added option to only show visible items in navigator

Version Control Systems

* Added query for saving modified files before opening commit editor
  (QTCREATORBUG-3857)

Beautifier

* Clang Format
    * Added action `Disable Formatting for Selected Text`
    * Changed formatting without selection to format the syntactic entity
      around the cursor

Model Editor

* Added support for custom relations

SCXML Editor

* Fixed crash after warnings are removed

Platform Specific

Windows

* Fixed that environment variable keys were converted to upper case in build
  and run configurations (QTCREATORBUG-18915)

macOS

* Fixed several issues when using case sensitive file systems while `File system
  case sensitivity` is set to `Case Insensitive` (QTCREATORBUG-17929,
  QTCREATORBUG-18672, QTCREATORBUG-18678)

Android

* Removed support for local deployment (QTBUG-62995)
* Removed support for Ant
* Improved checks for minimum requirements of Android tools (QTCREATORBUG-18837)

Universal Windows Platform

* Fixed deployment on Windows 10 Phone emulator

Credits for these changes go to:  
Alessandro Portale  
Alexander Volkov  
Andre Hartmann  
André Pönitz  
Christian Kandeler  
Christian Stenger  
Claus Steuer  
Daniel Trevitz  
David Schulz  
Eike Ziller  
Friedemann Kleint  
Ivan Donchevskii  
Jake Petroules  
Jaroslaw Kobus  
Jochen Becher  
Knud Dollereder  
Laurent Montel  
Marco Benelli  
Marco Bubke  
Mitch Curtis  
Nikita Baryshnikov  
Nikolai Kosjar  
Oliver Wolff  
Orgad Shaneh  
Robert Löhning  
Ryuji Kakemizu  
Samuel Gaist  
Serhii Moroz  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Tobias Hunger  
Ulf Hermann  
Vikas Pachdha
