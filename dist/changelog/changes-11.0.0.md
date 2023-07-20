Qt Creator 11
=============

Qt Creator version 11 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/10.0..v11.0.0

What's new?
------------

* Markdown editor with preview
  ([QTCREATORBUG-27883](https://bugreports.qt.io/browse/QTCREATORBUG-27883))
* Internal terminal
  ([QTCREATORBUG-8511](https://bugreports.qt.io/browse/QTCREATORBUG-8511))
* Experimental support for GitHub Copilot
* Experimental support for the `vcpkg` C/C++ package manager
* Experimental support for the Axivion static analyzer

### Markdown

You can open markdown (.md) files for editing or select `File > New File >
General > Markdown File` to create a new file.

([Documentation](https://doc.qt.io/qtcreator/creator-markdown-editor.html))

### Terminal

When you select the `Run in Terminal` check box and run an application or the
`Open Terminal` button to open a terminal, the default terminal opens in the
`Terminal` output view. It supports multiple tabs, as well as various
shells, colors, and fonts.

To use an external terminal, deselect the `Use internal terminal` check box in
`Preferences > Terminal`.

([Documentation](https://doc.qt.io/qtcreator/creator-reference-terminal-view.html))

### Copilot

The Copilot plugin (disabled by default) integrates
[GitHub Copilot](https://github.com/features/copilot), which uses OpenAI to
suggest code in the `Edit` mode.

To set Copilot preferences, select `Preferences > Copilot`.

([Documentation](https://doc.qt.io/qtcreator/creator-copilot.html))

### vcpkg

The experimental vcpkg plugin integrates the [vcpgk](https://vcpkg.io)
package manager for downloading and managing libraries.

Select the `vcpkg` installation location in `Preferences > CMake > Vcpkg > Path`.

To create a new `vcpkg.json` package manifest file, select `File > New File >
vcpkg`. The file is automatically added to the CMakeLists.txt file for the
project.

Edit manifest files in the manifest editor. To search for packages to add to the
file, select the `Search Package` button on the manifest editor toolbar.

([Documentation](https://doc.qt.io/qtcreator/creator-vcpkg.html))

### Axivion

After you configure access to the [Axivion](https://www.axivion.com) Dashboard
and link a project to an Axivion project in the project settings, Qt Creator
shows annotations of the latest run in the editors and allows you to view some
details on the issues.

([Documentation](https://doc.qt.io/qtcreator/creator-axivion.html))

General
-------

* Added a more spacious `Relaxed` toolbar style to `Preferences > Environment >
  Interface`
* Added a pin button to progress details instead of automatically resetting
  their position
  ([QTCREATORBUG-28829](https://bugreports.qt.io/browse/QTCREATORBUG-28829))
* Improved the selection and navigation in the `Issues` view
  ([QTCREATORBUG-26128](https://bugreports.qt.io/browse/QTCREATORBUG-26128),
   [QTCREATORBUG-27006](https://bugreports.qt.io/browse/QTCREATORBUG-27006),
   [QTCREATORBUG-27506](https://bugreports.qt.io/browse/QTCREATORBUG-27506))
* Fixed a crash with a large number of search hits from Silver Searcher
  ([QTCREATORBUG-29130](https://bugreports.qt.io/browse/QTCREATORBUG-29130))
* Locator
    * Improved performance
    * Added the creation of directories to the `Files in File System` filter
    * Added device roots and browsing remote file systems to the
      `Files in File System` filter

Editing
-------

* Improved the performance of the multi-cursor support
* Fixed the saving of hardlinked files
  ([QTCREATORBUG-19651](https://bugreports.qt.io/browse/QTCREATORBUG-19651))
* Fixed an issue of copy and paste with multiple cursors
  ([QTCREATORBUG-29117](https://bugreports.qt.io/browse/QTCREATORBUG-29117))
* Fixed the handling of pre-edit text for input methods
  ([QTCREATORBUG-29134](https://bugreports.qt.io/browse/QTCREATORBUG-29134))

### C++

* Improved the style of forward declarations in the outline
  ([QTCREATORBUG-312](https://bugreports.qt.io/browse/QTCREATORBUG-312))
* Added highlighting for typed string literals and user-defined literals
  ([QTCREATORBUG-28869](https://bugreports.qt.io/browse/QTCREATORBUG-28869))
* Extended the `Add Class Member` refactoring action to create class
  members from assignments
  ([QTCREATORBUG-1918](https://bugreports.qt.io/browse/QTCREATORBUG-1918))
* Fixed that generated functions did not have a `const` qualifier when
  required
  ([QTCREATORBUG-29274](https://bugreports.qt.io/browse/QTCREATORBUG-29274))
* Fixed that the locator showed both the declaration and the definition of symbols
  ([QTCREATORBUG-13894](https://bugreports.qt.io/browse/QTCREATORBUG-13894))
* Fixed the handling of C++20 keywords and concepts
* Fixed that the automatic Doxygen comment generation did not work when
  initializer lists `{}` were present
  ([QTCREATORBUG-29198](https://bugreports.qt.io/browse/QTCREATORBUG-29198))
* Fixed an issue when matching braces
  ([QTCREATORBUG-29339](https://bugreports.qt.io/browse/QTCREATORBUG-29339))
* Clangd
    * Fixed that the index could be outdated after VCS operations
    * Fixed the highlighting of labels
      ([QTCREATORBUG-27338](https://bugreports.qt.io/browse/QTCREATORBUG-27338))
    * Fixed freezes when showing tool tips
      ([QTCREATORBUG-29356](https://bugreports.qt.io/browse/QTCREATORBUG-29356))
* Built-in
    * Fixed support for `if`-statements with initializer
      ([QTCREATORBUG-29182](https://bugreports.qt.io/browse/QTCREATORBUG-29182))
* Clang Format
    * Fixed the conversion of tab indentation settings to Clang Format
      ([QTCREATORBUG-29185](https://bugreports.qt.io/browse/QTCREATORBUG-29185))

### Language Server Protocol

* Added actions for opening the `Call Hierarchy` to the context menu of the
  editor
  ([QTCREATORBUG-28839](https://bugreports.qt.io/browse/QTCREATORBUG-28839),
   [QTCREATORBUG-28842](https://bugreports.qt.io/browse/QTCREATORBUG-28842))

### QML

* Fixed the reformatting in the presence of JavaScript directives and function
  return type annotations
  ([QTCREATORBUG-29001](https://bugreports.qt.io/browse/QTCREATORBUG-29001),
   [QTCREATORBUG-29046](https://bugreports.qt.io/browse/QTCREATORBUG-29046))
* Fixed that reformatting changed `of` to `in`
  ([QTCREATORBUG-29123](https://bugreports.qt.io/browse/QTCREATORBUG-29123))
* Fixed the completion for Qt Quick Controls
  ([QTCREATORBUG-28648](https://bugreports.qt.io/browse/QTCREATORBUG-28648))
* Fixed that `qmllint` issues were not shown in the `Issues` view
  ([QTCREATORBUG-28720](https://bugreports.qt.io/browse/QTCREATORBUG-28720),
   [QTCREATORBUG-27762](https://bugreports.qt.io/browse/QTCREATORBUG-27762))
* Fixed a wrong `M16` warning
  ([QTCREATORBUG-28468](https://bugreports.qt.io/browse/QTCREATORBUG-28468))

### Python

* Added the `Create Virtual Environment` option to the Python interpreter
  selector on the editor toolbar and to the wizards in `File > New Project >
  > Application (Qt for Python)`
  ([PYSIDE-2152](https://bugreports.qt.io/browse/PYSIDE-2152))
* Fixed that too many progress indicators could be created
  ([QTCREATORBUG-29224](https://bugreports.qt.io/browse/QTCREATORBUG-29224))

  ([Documentation](https://doc.qt.io/qtcreator/creator-python-development.html))

### Meson

* Fixed the file targets
  ([QTCREATORBUG-29349](https://bugreports.qt.io/browse/QTCREATORBUG-29349))

Projects
--------

* Made it possible to add devices in `Preferences > Devices > Add` without going
  through the wizard
  ([Documentation](https://doc.qt.io/qtcreator/creator-developing-b2qt.html))
  ([Documentation](https://doc.qt.io/qtcreator/creator-developing-generic-linux.html))
* Added support for moving files to a different directory when renaming them in
  the `File System` view
  ([QTCREATORBUG-15981](https://bugreports.qt.io/browse/QTCREATORBUG-15981))
  ([Documentation](https://doc.qt.io/qtcreator/creator-file-system-view.html))

### CMake

* Implemented adding files to the project
  ([QTCREATORBUG-25922](https://bugreports.qt.io/browse/QTCREATORBUG-25922),
   [QTCREATORBUG-26006](https://bugreports.qt.io/browse/QTCREATORBUG-26006),
   [QTCREATORBUG-27213](https://bugreports.qt.io/browse/QTCREATORBUG-27213),
   [QTCREATORBUG-27538](https://bugreports.qt.io/browse/QTCREATORBUG-27538),
   [QTCREATORBUG-28493](https://bugreports.qt.io/browse/QTCREATORBUG-28493),
   [QTCREATORBUG-28904](https://bugreports.qt.io/browse/QTCREATORBUG-28904),
   [QTCREATORBUG-28985](https://bugreports.qt.io/browse/QTCREATORBUG-28985),
   [QTCREATORBUG-29006](https://bugreports.qt.io/browse/QTCREATORBUG-29006))
  ([Documentation](https://doc.qt.io/qtcreator/creator-project-cmake.html))
* Added support for the `block()` and `endblock()` CMake commands
  ([CMake documentation](https://cmake.org/cmake/help/latest/command/block.html#command:block))
* Fixed issues with detecting a configured Qt version when importing a build
  ([QTCREATORBUG-29075](https://bugreports.qt.io/browse/QTCREATORBUG-29075))
* Fixed the project wizards for Qt 6.3 and earlier
  ([QTCREATORBUG-29067](https://bugreports.qt.io/browse/QTCREATORBUG-29067))
* Presets
    * Added `Build > Reload CMake Presets` to reload CMake presets after making
      changes to them
      ([Documentation](https://doc.qt.io/qtcreator/creator-build-settings-cmake-presets.html))
    * Fixed that presets were not visible in the `Projects` view
      ([QTCREATORBUG-28966](https://bugreports.qt.io/browse/QTCREATORBUG-28966))
    * Fixed the type handling of the `architecture` and `toolset` fields
      ([QTCREATORBUG-28693](https://bugreports.qt.io/browse/QTCREATORBUG-28693))
    * Fixed the setting for QML debugging when creating build configurations
      ([QTCREATORBUG-29311](https://bugreports.qt.io/browse/QTCREATORBUG-29311))


### Qmake

* Fixed an infinite loop when the Qt ABI changed
  ([QTCREATORBUG-29204](https://bugreports.qt.io/browse/QTCREATORBUG-29204))

### Python

* Added an option for selecting the interpreter to the wizards in
  `File > New Project > Application (Qt for Python)`
  ([Documentation](https://doc.qt.io/qtcreator/creator-project-creating.html))

Debugging
---------

* Improved the UI for enabling and disabling debuggers in `Projects > Run >
  Debugger settings`
  ([QTCREATORBUG-28627](https://bugreports.qt.io/browse/QTCREATORBUG-28627))
  ([Documentation](https://doc.qt.io/qtcreator/creator-debugging-qml.html))
* Fixed the automatic source mapping for Qt versions from an installer
  ([QTCREATORBUG-28950](https://bugreports.qt.io/browse/QTCREATORBUG-28950))
* Fixed pretty printer for `std::string` for recent `libc++`
  ([QTCREATORBUG-29230](https://bugreports.qt.io/browse/QTCREATORBUG-29230))
* Fixed the pretty printers on Fedora 37
  ([QTCREATORBUG-28659](https://bugreports.qt.io/browse/QTCREATORBUG-28659))
* Fixed the display of arrays with `Array of 10,000 items`
  ([QTCREATORBUG-29196](https://bugreports.qt.io/browse/QTCREATORBUG-29196))

### C++

* Added the `Default array size` option for setting the default number of array
  elements to show in `Preferences > Debugger > Locals & Expressions`
* Fixed debugging in a terminal as the root user
  ([QTCREATORBUG-27519](https://bugreports.qt.io/browse/QTCREATORBUG-27519))
* CDB
    * Added automatic source file mapping for Qt packages
    * Fixed the variables view on remote Windows devices
      ([QTCREATORBUG-29000](https://bugreports.qt.io/browse/QTCREATORBUG-29000))
* LLDB
    * Fixed that long lines in the application output were broken into multiple
      lines
      ([QTCREATORBUG-29098](https://bugreports.qt.io/browse/QTCREATORBUG-29098))

### Qt Quick

* Improved the auto-detection of whether QML debugging is required
  ([QTCREATORBUG-28627](https://bugreports.qt.io/browse/QTCREATORBUG-28627))
* Added the `Use customized static analyzer` option for disabling static analyzer
  messages to `Preferences > Qt Quick > QML/JS Editing`
  ([QTCREATORBUG-29095](https://bugreports.qt.io/browse/QTCREATORBUG-29095))
  ([Documentation](https://doc.qt.io/qtcreator/creator-checking-code-syntax.html))

Analyzer
--------

### Clang

* Fixed that a `.clang-tidy` file in the project directory was not used by
  default
  ([QTCREATORBUG-28852](https://bugreports.qt.io/browse/QTCREATORBUG-28852))

Version Control Systems
-----------------------

### Git

* Instant Blame
    * Improved the performance
      ([QTCREATORBUG-29151](https://bugreports.qt.io/browse/QTCREATORBUG-29151))
    * Fixed that it did not show at the end of the document

Platforms
---------

### Android

* Fixed an issue with building library targets
  ([QTCREATORBUG-26980](https://bugreports.qt.io/browse/QTCREATORBUG-26980))

### iOS

* Improved the bundle ID that the project wizards create
  ([QTCREATORBUG-29340](https://bugreports.qt.io/browse/QTCREATORBUG-29340))

### Remote Linux

* Removed the automatic sourcing of target-side shell profiles

### Docker

* Added support for `qmake` based projects
  ([QTCREATORBUG-29140](https://bugreports.qt.io/browse/QTCREATORBUG-29140))
* Fixed issues after deleting the Docker image for a registered Docker device
  ([QTCREATORBUG-28880](https://bugreports.qt.io/browse/QTCREATORBUG-28880))

### MCU

* Fixed that errors were shown for valid QML code
  ([QTCREATORBUG-26655](https://bugreports.qt.io/browse/QTCREATORBUG-26655),
   [QTCREATORBUG-29155](https://bugreports.qt.io/browse/QTCREATORBUG-29155))
* Fixed that files were missing from locator and project search
  ([QTCREATORBUG-29297](https://bugreports.qt.io/browse/QTCREATORBUG-29297))

### QNX

* Added `slog2info` as a requirement for devices
* Fixed the support for remote working directories
  ([QTCREATORBUG-28900](https://bugreports.qt.io/browse/QTCREATORBUG-28900))

Credits for these changes go to:
--------------------------------
Aleksei German  
Alessandro Portale  
Alexander Drozdov  
Alexander Pershin  
Alexey Edelev  
Alexis Jeandet  
Ali Kianian  
Alibek Omarov  
Amr Essam  
Andre Hartmann  
André Pönitz  
Artem Mukhin  
Artem Sokolovskii  
Assam Boudjelthia  
Björn Schäpers  
Brook Cronin  
Burak Hancerli  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Esa Törmänen  
Fabian Kosmale  
Filippo Gentile  
Friedemann Kleint  
Haowei Hsu  
Henning Gruendl  
Jaroslaw Kobus  
Joni Poikelin  
Jussi Witick  
Kai Köhne  
Knud Dollereder  
Knut Petter Svendsen  
Leena Miettinen  
Mahmoud Badri  
Marco Bubke  
Marcus Tillmanns  
Martin Delille  
Mats Honkamaa  
Miikka Heikkinen  
Mitch Curtis  
Niels Weber  
Olivier Delaune  
Orgad Shaneh  
Pranta Dastider  
Robert Löhning  
Samuel Ghinet  
Semih Yavuz  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Ulf Hermann  
Vikas Pachdha  
Wladimir Leuschner  
Yasser Grimes  
Yixue Wang  
