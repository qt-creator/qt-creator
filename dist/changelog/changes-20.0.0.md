Qt Creator 20
=============

Qt Creator version 20 contains bug fixes and new features.
It is a free upgrade for all users.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=19.0..v20.0.0>

New plugins
-----------

### Agent Client Protocol (ACP) Support


Adds a chat panel that allows connecting to and chatting with an ACP server
in Qt Creator.

([Documentation](https://doc-snapshots.qt.io/qtcreator-20.0/creator-how-to-use-acp-client.html))

### Zen Mode

Adds `Tools > Zen Mode > Toggle Distraction Free Mode` and `Zen Mode` and
corresponding actions and tool buttons that put the editor into the focus of
your work.

([Documentation](<URL>))

### GN (Generate Ninja) Project Manager

Adds support for opening GN projects and working with them.

([Documentation](<URL>))

General
-------

Added

* Highlighting of all search results in searchable text views

Changed

* Split the `File > Open File or Project` menu item into two menu items:
  `Open File` and `Open Project`. The `Open File or Project` action is kept and
  can be assigned a keyboard shortcut.
  ([QTCREATORBUG-34162](https://bugreports.qt.io/browse/QTCREATORBUG-34162))

Fixed

* Issues with mixed file path case-sensitivity
* That the `Files in File System` advanced search expanded macros while editing
  ([QTCREATORBUG-31294](https://bugreports.qt.io/browse/QTCREATORBUG-31294))

### Model Context Protocol

Added

* A preferences page `AI > MCP Servers` for managing MCP servers
* Support for [Tasks](https://modelcontextprotocol.io/specification/draft/basic/utilities/tasks)
* Support for Cross-Origin Resource Sharing (CORS) for connecting from a web
  application
* Tools for
    * Running command-line tools and other executables
    * Version control repositories
    * Listing visible files
    * Managing breakpoints
    * Getting debugger stack traces
    * Finding and triggering actions
* Commands for
    * Retrieving project dependencies
    * Listing issues for a file
    * Finding files in projects
    * Searching and replacing text in project files and other files

Fixed

* That `set_file_plain_text` did not work if no editor is open for the file
* That `close_file` could loose unsaved data in an editor

Editing
-------

Fixed

* The generic highlighter's handling of `Ignored file patterns`
  ([QTCREATORBUG-33443](https://bugreports.qt.io/browse/QTCREATORBUG-33443))

### C++

Added

* Built-in
    * Support for `std::size_t` literals
      ([QTCREATORBUG-34208](https://bugreports.qt.io/browse/QTCREATORBUG-34208))
* Clangd
    * The option `Use externally provided compilation database` instead of the
      automatically created one
      ([QTCREATORBUG-34066](https://bugreports.qt.io/browse/QTCREATORBUG-34066))
    * `Fold/Unfold All Inactive Code`

Changed

* The prebuilt binaries to LLVM 22.1.2

Fixed

* Built-in
    * The parsing of compound literals
      ([QTCREATORBUG-34089](https://bugreports.qt.io/browse/QTCREATORBUG-34089))

### QML

Changed

* qmlls
    * Semantic highlighting to be enabled by default after fixes in qmlls
    * The context menu to show the refactoring actions that are available from `qmlls`

### Language Server Protocol

Added

* Support for code folding preprocessor branches, C++-style comment blocks,
  and `#pragma` regions
  ([QTCREATORBUG-2](https://bugreports.qt.io/browse/QTCREATORBUG-2),
   [QTCREATORBUG-5170](https://bugreports.qt.io/browse/QTCREATORBUG-5170),
   [QTCREATORBUG-22561](https://bugreports.qt.io/browse/QTCREATORBUG-22561))
* Support for new symbol tags and using them for symbol icons

### Markdown

Fixed

* Issues with rendering multi-line headings
  ([QTCREATORBUG-33781](https://bugreports.qt.io/browse/QTCREATORBUG-33781))

### GLSL

Added

* Vulkan compatible file wizards
  ([QTCREATORBUG-26058](https://bugreports.qt.io/browse/QTCREATORBUG-26058),
   [QTCREATORBUG-32869](https://bugreports.qt.io/browse/QTCREATORBUG-32869))

Projects
--------

Added

* The option to `Get variables from text file or shell script`
  to modify the environment
  ([QTCREATORBUG-27746](https://bugreports.qt.io/browse/QTCREATORBUG-27746))
* Output parsing of `file://` links
  ([QTCREATORBUG-34176](https://bugreports.qt.io/browse/QTCREATORBUG-34176))

Changed

* The `Auto-detected` tools category to `Automatically Managed`

Fixed

* A performance issue with the `Application Output`
  ([QTCREATORBUG-34132](https://bugreports.qt.io/browse/QTCREATORBUG-34132))

### CMake

Added

* Presets
    * That included preset files are watched for changes

Changed

* The kits that are created for Presets to temporary, project-specific kits
  ([QTCREATORBUG-33522](https://bugreports.qt.io/browse/QTCREATORBUG-33522))

Debugging
---------

Added

* A `Remote Debugger` run configuration type with functionality similar to
  `Debug > Start Debugging > Attach to Running Debug Server`

Fixed

* Issues with non-ASCII file paths
  ([QTCREATORBUG-33863](https://bugreports.qt.io/browse/QTCREATORBUG-33863))

### C++

Added

* LLDB
    * Support for trace points

Fixed

* The pretty printers of `QSharedDataPointer`, `QPixmap`,
  and `boost::unordered_set`
  ([QTCREATORBUG-34299](https://bugreports.qt.io/browse/QTCREATORBUG-34299))
* The display options for 128-bit integers
  ([QTCREATORBUG-28134](https://bugreports.qt.io/browse/QTCREATORBUG-28134))

Analyzer
--------

### QML Profiler

Fixed

* That notes were not saved and restored from trace files
  ([QTCREATORBUG-34249](https://bugreports.qt.io/browse/QTCREATORBUG-34249))

Version Control Systems
-----------------------

Added

* The version control operations to the context menu in `Open Documents` and for
  editors
  ([QTCREATORBUG-29164](https://bugreports.qt.io/browse/QTCREATORBUG-29164))
* `Mark Untracked` to the version control context menu
* The version control state to the file properties dialog
* Improvements to the detection of which line to jump to when double-clicking
  into diffs
  ([QTCREATORBUG-29374](https://bugreports.qt.io/browse/QTCREATORBUG-29374))

Fixed

* Issues with updating the live version control state
  ([QTCREATORBUG-33691](https://bugreports.qt.io/browse/QTCREATORBUG-33691))

### Git

Added

* A warning if already staged changes were unselected in the submit editor
  which leads to these being unstaged
  ([QTCREATORBUG-34014](https://bugreports.qt.io/browse/QTCREATORBUG-34014))

Fixed

* That the `Continue Rebase` dialog made it too easy to accidentally trigger
  destructive operations
  ([QTCREATORBUG-34358](https://bugreports.qt.io/browse/QTCREATORBUG-34358))
* Issues with non-ASCII file paths

### SVN

Added

* Support for the live showing of the version control state
  ([QTCREATORBUG-34027](https://bugreports.qt.io/browse/QTCREATORBUG-34027))

Fixed

* That authentication was used for local commands that do not need it

Test Integration
----------------

Added

* The option to filter the `Tests` view
  ([QTCREATORBUG-23377](https://bugreports.qt.io/browse/QTCREATORBUG-23377),
   [QTCREATORBUG-34244](https://bugreports.qt.io/browse/QTCREATORBUG-34244))

Platforms
---------

### Android

Added

* The menu `Tools > Android` with
    * `Icon Editor`
    * `Splashscreen Editor`
    * `Permissions Editor`

### iOS

Added

* Limited file access to iOS devices, the crash reports directory and the
  developer's application directories, to the `File System` view, the Locator,
  and remote file dialogs
* The OS version and supported architectures to the Simulator selection

### Remote Linux

Added

* Automatic connection to devices before deployment and running
  ([QTCREATORBUG-34011](https://bugreports.qt.io/browse/QTCREATORBUG-34011))
* The option to `Use the Qt VNC platform for display` when running
* The `Accessible host paths` options that specifies directories on the host
  that can be accessed by the remote device

Fixed

* Potential hangs for devices that do not support the command bridge
  ([QTCREATORBUG-33699](https://bugreports.qt.io/browse/QTCREATORBUG-33699))

### Development Container

Added

* Support for specifying arbitrary CMake variables
  ([QTCREATORBUG-34220](https://bugreports.qt.io/browse/QTCREATORBUG-34220))

### Docker

Fixed

* Support for QML debugging
  ([QTCREATORBUG-34094](https://bugreports.qt.io/browse/QTCREATORBUG-34094))

Credits for these changes go to:
--------------------------------
Alessandro Portale  
André Hartmann  
André Pönitz  
Björn Schäpers  
BogDan Vatra  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Jaroslaw Kobus  
Jeff Heller  
Leena Miettinen  
Marcus Tillmanns  
Mike Jyu  
Orgad Shaneh  
Patrik Teivonen  
Sami Shalayel  
Sebastian Mosiej  
Sheree Morphett  
Teea Poldsam  
Thiago Macieira  
Ulf Hermann  
Ville Lavonius  
Vladislav Navrocky  
Xavier Besson  
