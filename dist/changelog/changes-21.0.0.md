Qt Creator 21
=============

Qt Creator version 21 contains bug fixes and new features.
It is a free upgrade for all users.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=20.0..v21.0.0>

New plugins
-----------

### Zephyr

Adds support for [Zephyr RTOS](https://www.zephyrproject.org/) and West.

### HarmonyOS

Adds support for [HarmonyOS](https://consumer.huawei.com/en/harmonyos/).
([Documentation](https://doc-snapshots.qt.io/qtcreator-master/creator-developing-harmonyos.html))

General
-------

Added

* Multiple selection in the `File System` view
  ([QTCREATORBUG-16](https://bugreports.qt.io/browse/QTCREATORBUG-16))
* `Copy Path Relative to Project` and `Copy Path and Line Number Relative to
  Project` to the file context menus
  ([QTCREATORBUG-9028](https://bugreports.qt.io/browse/QTCREATORBUG-9028))
* The option to turn the atomic saving of files off
  ([QTCREATORBUG-7598](https://bugreports.qt.io/browse/QTCREATORBUG-7598))

Changed

* Moved the `Extensions` browser into `Preferences` mode
* Improved the file dialog for remote paths
* Improved the search keywords of various preferences pages

Fixed

* Performance issues with the `File System` view
  ([QTCREATORBUG-33785](https://bugreports.qt.io/browse/QTCREATORBUG-33785))
* That JavaScript macros could block endlessly
  ([QTCREATORBUG-19691](https://bugreports.qt.io/browse/QTCREATORBUG-19691))
* That the `File System` view did not integrate with the version control
  system when renaming files or directories
  ([QTCREATORBUG-34605](https://bugreports.qt.io/browse/QTCREATORBUG-34605))
* That external tools could be run even if they used unavailable macros
  ([QTCREATORBUG-8490](https://bugreports.qt.io/browse/QTCREATORBUG-8490))
* That external tools could modify read-only editors and replace read-only
  selections
  ([QTCREATORBUG-12405](https://bugreports.qt.io/browse/QTCREATORBUG-12405))
* That external tool actions were enabled even without the context that they
  need
  ([QTCREATORBUG-3837](https://bugreports.qt.io/browse/QTCREATORBUG-3837))
* That the navigation history could contain duplicate entries
  ([QTCREATORBUG-5179](https://bugreports.qt.io/browse/QTCREATORBUG-5179))
* That icons were scaled wrongly with large UI scaling
  ([QTCREATORBUG-17829](https://bugreports.qt.io/browse/QTCREATORBUG-17829))
* That `file://` links in the application output were not opened
  ([QTCREATORBUG-34870](https://bugreports.qt.io/browse/QTCREATORBUG-34870))
* That nested macros could recurse endlessly

### Agent Client Protocol (ACP)

Added

* Support for version 2 of the protocol, including elicitation
* Automatic updating of agents from the registry
  ([QTCREATORBUG-34760](https://bugreports.qt.io/browse/QTCREATORBUG-34760))
* The option to create a new session for a workspace from the history
  ([QTCREATORBUG-34619](https://bugreports.qt.io/browse/QTCREATORBUG-34619))
* The option to delete sessions (if supported)
* The option to switch the session (if supported)
* Dropping files onto the chat input to add them to the context
* The display of the token usage of a session and of the running turn
  ([QTCREATORBUG-34869](https://bugreports.qt.io/browse/QTCREATORBUG-34869))
* Scroll wheel zoom for the chat
  ([QTCREATORBUG-34921](https://bugreports.qt.io/browse/QTCREATORBUG-34921))
* Actions for clearing the chat input history
* Suggestions from the chat input history
  ([QTCREATORBUG-34877](https://bugreports.qt.io/browse/QTCREATORBUG-34877))

Changed

* Enabled the ACP integration by default
* Changed to use tabs for chats only when tabs are used for the editors
* Improved the chat input field and the session options

Fixed

* That the chat button was only added to the tool bar of text editors

### Model Context Protocol

Added

* Authentication of clients with a bearer token
* The option to import and export the Qt Creator MCP server settings
  ([QTCREATORBUG-34918](https://bugreports.qt.io/browse/QTCREATORBUG-34918))
* The option to enable and disable tools in
  `Preferences > AI > Qt Creator MCP Server`
  ([QTCREATORBUG-34617](https://bugreports.qt.io/browse/QTCREATORBUG-34617))
* The `-mcp-port` command line option
* Lots of new tools for tests, CMake, projects, devices, kits, plugins,
  debugging, the code model, and more
* The option to provide a line and column to the `open_file` tool
* The option to provide a starting line and ending line to the `file_plain_text`
  tool
* Tools for the tests, CMake, projects, devices, plugins,
  language server protocol, C++, the Qt Creator documentation, profiling, kits,
  Qt versions, and the debugger console
  ([QTCREATORBUG-34629](https://bugreports.qt.io/browse/QTCREATORBUG-34629))
* The `-mcp-dump-tools` command line option

Changed

* Turned on the plugin and server by default,
  requiring authentication with a bearer token and listening on a local port
* Improved `list_projects` and `set_active_project`
* Prefixed the names of the tools with their area

Fixed

* That modified documents could be overwritten with `set_file_plain_text`

Help
----

Fixed

* That registered Qt Creator documentation could accumulate over version updates

Editing
-------

Added

* The option to open a project file as a project in Qt Creator
* The option to immediately reload unchanged documents
  ([QTCREATORBUG-34634](https://bugreports.qt.io/browse/QTCREATORBUG-34634))
* `Window > Equally Distribute Splits`
  ([QTCREATORBUG-30013](https://bugreports.qt.io/browse/QTCREATORBUG-30013))
* That jumping to search results from `Advanced Search` selects the matching
  text
  ([QTCREATORBUG-34592](https://bugreports.qt.io/browse/QTCREATORBUG-34592))
* Support for spell checking text files and comments using the built-in services
  on Windows and macOS, and the `enchant-2` library on Linux (if available)
  ([QTCREATORBUG-1861](https://bugreports.qt.io/browse/QTCREATORBUG-1861))
* The option to resolve merge conflicts directly in the text editor
* `Move Document to Next Split` and `Move Document to Previous Split`
  ([QTCREATORBUG-8135](https://bugreports.qt.io/browse/QTCREATORBUG-8135))
* Auto-scrolling with the middle mouse button on Windows and macOS
  ([QTCREATORBUG-8514](https://bugreports.qt.io/browse/QTCREATORBUG-8514))
* A separate `Template Parameter` text highlighting style
  ([QTCREATORBUG-6225](https://bugreports.qt.io/browse/QTCREATORBUG-6225))

Changed

* Replaced the dialog that opened for externally modified files by non-intrusive
  editor notifications
* Changed the tab stop distance to be based on the width of a space, so tabs
  and spaces line up
  ([QTCREATORBUG-10367](https://bugreports.qt.io/browse/QTCREATORBUG-10367))
* Changed Makefiles to always be indented with tabs
  ([QTCREATORBUG-3408](https://bugreports.qt.io/browse/QTCREATORBUG-3408))

Fixed

* A performance issue with the tabs for editors
* That `Clean Whitespace` could wrongly convert tabs to spaces
  ([QTCREATORBUG-33670](https://bugreports.qt.io/browse/QTCREATORBUG-33670))
* An issue with code folding after saving files
  ([QTCREATORBUG-8078](https://bugreports.qt.io/browse/QTCREATORBUG-8078))
* That printing a selection printed the whole document
  ([QTCREATORBUG-12552](https://bugreports.qt.io/browse/QTCREATORBUG-12552))
* The page range that is offered in the print dialog
  ([QTCREATORBUG-12059](https://bugreports.qt.io/browse/QTCREATORBUG-12059))
* That `Clean Whitespace` reflowed code that follows a line comment
  ([QTCREATORBUG-31149](https://bugreports.qt.io/browse/QTCREATORBUG-31149))
* That the indentation was cleaned up even for ignored file types
  ([QTCREATORBUG-32894](https://bugreports.qt.io/browse/QTCREATORBUG-32894))
* `Open Terminal Here` for subdirectories
* That the `Do not ask again` check box of the dialog for large files was not
  persisted
  ([QTCREATORBUG-34916](https://bugreports.qt.io/browse/QTCREATORBUG-34916))

### C++

Added

* The `Wrap in std::as_const()` quick fix
* The `Inline Function Call` and `Inline Function Call and Remove Function If
  Unused` quick fixes
  ([QTCREATORBUG-9615](https://bugreports.qt.io/browse/QTCREATORBUG-9615))
* Built-in
    * Support for `__has_include`
      ([QTCREATORBUG-25181](https://bugreports.qt.io/browse/QTCREATORBUG-25181))

Changed

* Improved the code style settings
* Jumping to a function from the `Class View` now opens the definition if
  available
  ([QTCREATORBUG-16175](https://bugreports.qt.io/browse/QTCREATORBUG-16175))
* That `Open Include Hierarchy` opens the hierarchy of the `#include` under
  the text cursor, if any
  ([QTCREATORBUG-2311](https://bugreports.qt.io/browse/QTCREATORBUG-2311))

Fixed

* That the `Generate Missing Q_PROPERTY Members` quick fix did not consider the
  `BINDABLE` attribute
  ([QTCREATORBUG-34561](https://bugreports.qt.io/browse/QTCREATORBUG-34561))
* That the `Class View` expanded on each update
  ([QTCREATORBUG-18891](https://bugreports.qt.io/browse/QTCREATORBUG-18891))
* That a space was inserted into operator symbols
  ([QTCREATORBUG-34875](https://bugreports.qt.io/browse/QTCREATORBUG-34875))
* The indentation of lambdas with the Whitesmith style
  ([QTCREATORBUG-34742](https://bugreports.qt.io/browse/QTCREATORBUG-34742))
* That symbols were displayed differently in the various Locator filters
  ([QTCREATORBUG-34858](https://bugreports.qt.io/browse/QTCREATORBUG-34858))
* That the code model could work on an outdated document
  ([QTCREATORBUG-18800](https://bugreports.qt.io/browse/QTCREATORBUG-18800))

### QML

Fixed

* That `qmlls` was not enabled for Python projects
  ([QTCREATORBUG-34467](https://bugreports.qt.io/browse/QTCREATORBUG-34467))
* That `qmlls` was enabled for `.qbs` files
* That formatting code dropped destructuring variables
  ([QTCREATORBUG-33279](https://bugreports.qt.io/browse/QTCREATORBUG-33279))
* Issues with `.mjs` module files
  ([QTCREATORBUG-27738](https://bugreports.qt.io/browse/QTCREATORBUG-27738))

### Python

Fixed

* That the language servers of interpreters that no longer exist were kept
  ([QTCREATORBUG-34264](https://bugreports.qt.io/browse/QTCREATORBUG-34264))
* That `qmlImportPaths` from `pyproject.toml` was ignored
  ([QTCREATORBUG-34911](https://bugreports.qt.io/browse/QTCREATORBUG-34911))

### Language Server Protocol

Fixed

* That the tool button in the editor could vanish if the language server failed
* That symbol tags that are not part of the released protocol were advertised
  ([QTCREATORBUG-34906](https://bugreports.qt.io/browse/QTCREATORBUG-34906))

### Diff Viewer

Added

* An editable live diff view for single files
* The patience algorithm for computing diffs
  ([QTCREATORBUG-34836](https://bugreports.qt.io/browse/QTCREATORBUG-34836))
* The display of the description of patch files, including links
  ([QTCREATORBUG-13120](https://bugreports.qt.io/browse/QTCREATORBUG-13120))

Fixed

* Misaligned indentation guides
  ([QTCREATORBUG-33858](https://bugreports.qt.io/browse/QTCREATORBUG-33858))
* The highlighting of changes that contain repeated substrings
  ([QTCREATORBUG-33879](https://bugreports.qt.io/browse/QTCREATORBUG-33879))
* `Jump to Original File` for renamed files

### Widget Designer

* Added the option `Generate pointer-to-member connections in "Go to Slot"`
  (enabled by default)
  ([QTCREATORBUG-29257](https://bugreports.qt.io/browse/QTCREATORBUG-29257))

### TODO

Changed

* The view now shows file paths relative to the active project
  ([QTCREATORBUG-9165](https://bugreports.qt.io/browse/QTCREATORBUG-9165))

### Markdown

Added

* Support for language servers
* Support for multiple editors on the same document (splitting)
  ([QTCREATORBUG-30767](https://bugreports.qt.io/browse/QTCREATORBUG-30767))
* Support for file links with line numbers
* Zoom shortcuts for the preview

Fixed

* Some rendering issues
  ([QTCREATORBUG-34571](https://bugreports.qt.io/browse/QTCREATORBUG-34571))
* That searching in the preview was not possible
* That search results were not highlighted in the other view
  ([QTCREATORBUG-34901](https://bugreports.qt.io/browse/QTCREATORBUG-34901))

### SCXML

Fixed

* The width of transition items
  ([QTCREATORBUG-33139](https://bugreports.qt.io/browse/QTCREATORBUG-33139))

### FakeVim

Added

* Support for `:delmarks`
* Automatic removal of indentation from new lines that are left empty
  ([QTCREATORBUG-15009](https://bugreports.qt.io/browse/QTCREATORBUG-15009))
* Support for `,` with the option to revert to the old behavior that passes
  shortcuts to Qt Creator
  ([QTCREATORBUG-12115](https://bugreports.qt.io/browse/QTCREATORBUG-12115))
* Support for `gp` and `gw`
  ([QTCREATORBUG-28622](https://bugreports.qt.io/browse/QTCREATORBUG-28622))
* Support for `zi`
  ([QTCREATORBUG-11753](https://bugreports.qt.io/browse/QTCREATORBUG-11753))
* Support for the `timeout` and `timeoutlen` configure options
  ([QTCREATORBUG-29162](https://bugreports.qt.io/browse/QTCREATORBUG-29162))
* Support for `gd`
  ([QTCREATORBUG-27191](https://bugreports.qt.io/browse/QTCREATORBUG-27191))
* Support for `breakindent`
  ([QTCREATORBUG-20974](https://bugreports.qt.io/browse/QTCREATORBUG-20974))
* The `Cursor blink rate` option
  ([QTCREATORBUG-18181](https://bugreports.qt.io/browse/QTCREATORBUG-18181))
* The option to `Use the editor's tabulator and indentation settings`
  ([QTCREATORBUG-14273](https://bugreports.qt.io/browse/QTCREATORBUG-14273))
* The option to show the `Command line in the editor`
  ([QTCREATORBUG-21005](https://bugreports.qt.io/browse/QTCREATORBUG-21005))
* Support for Vimscript
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
    * Legacy scripts and Vim9 script, with `:def` functions, `var`, `const`,
      and `final` declarations, `(args) => expr` lambdas, string
      interpolation, implicit line continuation, and `import`/`export`
    * Expressions, including `&option`, `@register`, `$ENV`, the `=~`, `!~`,
      `is`, `isnot`, and `??` operators, short-circuiting `&&` and `||`, and
      indexing and slicing of lists and strings
    * Variables with `:let`, `:const`, `:unlet`, and `:lockvar`, including
      list unpacking, heredoc assignment, and a separate `s:` scope per script
    * The `List`, `Dictionary`, `Blob`, and `Tuple` types, including `d.key`
      access
    * The `v:` variables
    * `:if`, `:while`, and `:for`, with `:break` and `:continue`
    * User functions with `:function`, `:call`, `:return`, and `:delfunction`,
      including variadic arguments, `range` functions, and autoload functions
      that are read from `runtimepath` when they are first used
    * Funcrefs and lambdas, with `function()`, `funcref()`, `call()`, and the
      `v->f()` method syntax
    * `:try`, `:catch`, `:finally`, `:throw`, and `:defer`
    * `:execute`, `:finish`, `:silent`, `:echoerr`, `:echon`, `:messages`,
      and `:redir`
    * Around 400 builtin functions, such as `printf`, `substitute`, `split`,
      `sort`, `map`, `filter`, `search`, `searchpair`, `cursor`, `getpos`,
      `matchlist`, `readfile`, `writefile`, `strftime`, and `system`
    * Vim's pattern syntax, including the magic levels (`\v`, `\m`, `\M`,
      and `\V`), `\zs`, `\ze`, the look-around patterns, the character
      classes, and `\=` replacements
    * `:autocmd`, `:augroup`, and `:doautocmd`, with 60 of Vim's events
    * User-defined commands with `:command`, `:delcommand`, and `:comclear`
    * `:map <expr>`, `<Cmd>` and `<ScriptCmd>` mappings, and `<SID>`,
      `<SNR>`, and `<Plug>` in names and keys
    * The `=` expression register (`Ctrl+R =`)
* Support for running Vim plugins, such as `matchit`, `comment`, `justify`,
  `editorconfig`, `repeat.vim`, `vim-unimpaired`, `vim-exchange`, and
  `ReplaceWithRegister`
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* Support for `g@` and the `operatorfunc` option
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* Support for modelines, with the `modeline` and `modelines` options
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* Detection of the file type of a document, which sets `filetype` and the
  buffer-local `commentstring` that `gc` uses as the comment leader
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* Support for Select mode
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* Many more Ex commands, options, motions, text objects, and insert-mode and
  command-line keys, such as `gi`, `g_`, `gn`, `g;`, `gf`, `]p`, `:copy`,
  `:sort`, `:retab`, `:put`, `:mark`, `:earlier`, `:startinsert`,
  `nrformats`, `matchpairs`, `whichwrap`, `softtabstop`, and `cpoptions`
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* Support for the `it` and `at` tag text objects
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* Support for `Ctrl+G` and `Ctrl+^`
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* Support for `K` that opens the context help
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* The option to match brackets for `%` like Vim, instead of using the
  syntax-aware bracket matching of the editor

Fixed

* The `normal` Ex mode command
  ([QTCREATORBUG-33296](https://bugreports.qt.io/browse/QTCREATORBUG-33296))
* Delete and backspace in replace mode
  ([QTCREATORBUG-12120](https://bugreports.qt.io/browse/QTCREATORBUG-12120))
* That indentation could be lost during block insert
  ([QTCREATORBUG-24094](https://bugreports.qt.io/browse/QTCREATORBUG-24094))
* Yanking and pasting wrapped lines in visual line mode
  ([QTCREATORBUG-16713](https://bugreports.qt.io/browse/QTCREATORBUG-16713))
* Issues with quoting text selections
  ([QTCREATORBUG-22484](https://bugreports.qt.io/browse/QTCREATORBUG-22484))
* The scroll behavior (`Ctrl+E` and `Ctrl+Y`) with regards to `scrolloff`
  ([QTCREATORBUG-34074](https://bugreports.qt.io/browse/QTCREATORBUG-34074))
* `:move` on the last line of the document
* `zz`, `zt`, and `zb` when `Center cursor on scroll` is enabled
  ([QTCREATORBUG-15407](https://bugreports.qt.io/browse/QTCREATORBUG-15407))
* A crash when `tabstop=0` is set
  ([QTCREATORBUG-29376](https://bugreports.qt.io/browse/QTCREATORBUG-29376))
* The handling of the `ISO_Level5_Shift` modifier key
  ([QTCREATORBUG-26818](https://bugreports.qt.io/browse/QTCREATORBUG-26818))
* The handling of `Ctrl-]` and `Ctrl-T`
  ([QTCREATORBUG-11754](https://bugreports.qt.io/browse/QTCREATORBUG-11754))
* The text cursor past the end of the line
  ([QTCREATORBUG-29553](https://bugreports.qt.io/browse/QTCREATORBUG-29553))
* Multi-line selection with `$` in block-visual mode
  ([QTCREATORBUG-22192](https://bugreports.qt.io/browse/QTCREATORBUG-22192))
* Issues with `inoremap` with `"` in its expansion
  ([QTCREATORBUG-11617](https://bugreports.qt.io/browse/QTCREATORBUG-11617))
* That `Ctrl+Y` and `Ctrl+E` did not scroll past `scrolloff`
  ([QTCREATORBUG-34074](https://bugreports.qt.io/browse/QTCREATORBUG-34074))
* The position of line numbers relative to the cursor
  ([QTCREATORBUG-26802](https://bugreports.qt.io/browse/QTCREATORBUG-26802))
* Issues with some key bindings
  ([QTCREATORBUG-20998](https://bugreports.qt.io/browse/QTCREATORBUG-20998))
* That FakeVim stayed disabled after a document becomes writable
  ([QTCREATORBUG-24237](https://bugreports.qt.io/browse/QTCREATORBUG-24237))
* That `Ctrl+O` and `Ctrl+I` didn't work across files
  ([QTCREATORBUG-12114](https://bugreports.qt.io/browse/QTCREATORBUG-12114))
* The behavior of repeating pastes with `.`
  ([QTCREATORBUG-18298](https://bugreports.qt.io/browse/QTCREATORBUG-18298))
* That the standard shortcuts for copy & paste did not work in the FakeVim
  command line
  ([QTCREATORBUG-23785](https://bugreports.qt.io/browse/QTCREATORBUG-23785))
* That operators did not work with the `/` and `?` motions when
  `Use search dialog` is enabled
  ([QTCREATORBUG-24172](https://bugreports.qt.io/browse/QTCREATORBUG-24172))
* That text was overwritten when the editor handles the keys itself, for
  example for snippets and `Rename Symbol Under Cursor`
  ([QTCREATORBUG-34908](https://bugreports.qt.io/browse/QTCREATORBUG-34908))
* That the mini buffer showed the Vim state even when FakeVim was disabled
  ([QTCREATORBUG-34897](https://bugreports.qt.io/browse/QTCREATORBUG-34897))
* Many differences to Vim's behavior in motions, text objects, operators,
  registers, marks, Ex ranges, mappings, options, and the insert-mode and
  command-line keys
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* That `:xmap` and its variants did not apply in visual mode
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* That `:map` dropped the options that follow `<silent>`
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* That the abbreviations of Ex commands were only accepted in their shortest
  and longest spelling
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* That `:normal` did not take the rest of the line after a `|`
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* That `:w {file}` wrote only the current line
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))
* A crash with `:global` and `:vglobal` without arguments
  ([QTCREATORBUG-34817](https://bugreports.qt.io/browse/QTCREATORBUG-34817))

### Binary Files

Added

* The option to group bytes into words for the display
  ([QTCREATORBUG-4392](https://bugreports.qt.io/browse/QTCREATORBUG-4392))

Fixed

* That a missing file was not reported as missing
  ([QTCREATORBUG-34870](https://bugreports.qt.io/browse/QTCREATORBUG-34870))

Projects
--------

Added

* The option to create and open a Workspace project for executable files that
  are opened in an editor
  ([QTCREATORBUG-30837](https://bugreports.qt.io/browse/QTCREATORBUG-30837))
* The option to filter for run configurations in the target selector
  ([QTCREATORBUG-34608](https://bugreports.qt.io/browse/QTCREATORBUG-34608))
* The `sp` Locator filter for switching the active project
* The `Create Kits Now` button and the `Set as Active Kit` context menu item
  to the device and kit preferences
  ([QTCREATORBUG-31233](https://bugreports.qt.io/browse/QTCREATORBUG-31233))
* The `Use debug version of frameworks` option for custom executables
  ([QTCREATORBUG-9333](https://bugreports.qt.io/browse/QTCREATORBUG-9333))
* A warning when adding files to a resource that already contains them
  ([QTCREATORBUG-10328](https://bugreports.qt.io/browse/QTCREATORBUG-10328))

Changed

* The wizards now allow creating projects even if no kit is usable
* Devices and their kits are now created after the tools of the device have
  been detected, and remote toolchains are revalidated when a device
  reconnects
* Unified the context menu of the `Projects` view with the other file menus


Fixed

* The handling of ANSI escape sequences in incomplete lines
  ([QTCREATORBUG-33704](https://bugreports.qt.io/browse/QTCREATORBUG-33704))
* That switching application output tabs temporarily disabled the search tool
  bar
  ([QTCREATORBUG-32444](https://bugreports.qt.io/browse/QTCREATORBUG-32444))
* That only scripts with certain file extensions could be used to read
  environment variables in
  ([QTCREATORBUG-34717](https://bugreports.qt.io/browse/QTCREATORBUG-34717))
* That project wide searches with `Show Paths in Relation to Active Project`
  could show absolute paths for files in the project that are located outside
  the project's top level directory
  ([QTCREATORBUG-31530](https://bugreports.qt.io/browse/QTCREATORBUG-31530))
* That Qt versions that were registered with `qtpaths` were marked as invalid
  ([QTCREATORBUG-33606](https://bugreports.qt.io/browse/QTCREATORBUG-33606))
* That the project encoding setting did not affect documents that are already
  open
  ([QTCREATORBUG-33844](https://bugreports.qt.io/browse/QTCREATORBUG-33844))
* That items with the same name were indistinguishable in the project tree
  ([QTCREATORBUG-34500](https://bugreports.qt.io/browse/QTCREATORBUG-34500))
* That the output pane offered actions that a passive run, such as the Android
  `Logcat`, cannot perform
  ([QTCREATORBUG-34397](https://bugreports.qt.io/browse/QTCREATORBUG-34397),
   [QTCREATORBUG-33853](https://bugreports.qt.io/browse/QTCREATORBUG-33853))
* That the environment of the run device was fetched only while starting the
  run, and that a failure to fetch it was not reported
  ([QTCREATORBUG-34920](https://bugreports.qt.io/browse/QTCREATORBUG-34920))
* That devices whose type is not available were permanently removed
* That run configurations could end up without a name

### CMake

Added

* The option to run arbitrary CMake commands with the `cm` Locator filter
  ([QTCREATORBUG-34769](https://bugreports.qt.io/browse/QTCREATORBUG-34769))
* The option `Include support for Figma to Qt content` to make the feature
  opt-out in the `Qt Quick Application` wizard
  ([QTCREATORBUG-34719](https://bugreports.qt.io/browse/QTCREATORBUG-34719))
* The option to format and indent with a built-in indenter instead of
  `cmake-format` (`Preferences > CMake > Formatter`, enabled by default)
  ([QTCREATORBUG-19417](https://bugreports.qt.io/browse/QTCREATORBUG-19417))
* An outline of CMake files, completion for the arguments of custom commands,
  highlighting of command keywords, documentation tool tips, and renaming of
  the symbols of a project
* Quick fixes for installing missing Qt components and for creating missing
  source files, and the option to create the `CMakeLists.txt` for a
  subdirectory
* The `ct` Locator filter for running arbitrary `ctest` commands
* The `runSettings` and `runDevice` vendor extensions for CMake Presets
  ([QTCREATORBUG-34563](https://bugreports.qt.io/browse/QTCREATORBUG-34563),
   [QTCREATORBUG-34834](https://bugreports.qt.io/browse/QTCREATORBUG-34834))

Changed

* Updated to the CMake parser version 4.2
* Deployment data is now derived from the `install()` rules, unless the
  project has a `QtCreatorDeployment.txt`
* Files are now added to the variable that holds the sources, if there is one
  ([QTCREATORBUG-30582](https://bugreports.qt.io/browse/QTCREATORBUG-30582))

Fixed

* Issues with `FILE_SET` in combination with `INTERFACE` libraries
  ([QTCREATORBUG-34298](https://bugreports.qt.io/browse/QTCREATORBUG-34298))
* The coloring of CMake output in `General Messages`
  ([QTCREATORBUG-34812](https://bugreports.qt.io/browse/QTCREATORBUG-34812))
* That some targets were missing from the `cm` Locator filter
  ([QTCREATORBUG-34776](https://bugreports.qt.io/browse/QTCREATORBUG-34776))
* That the sources of untaken branches were missing from the project tree
  ([QTCREATORBUG-33444](https://bugreports.qt.io/browse/QTCREATORBUG-33444))
* That the DLLs of private and imported dependencies were not found for
  running and debugging
  ([QTCREATORBUG-31018](https://bugreports.qt.io/browse/QTCREATORBUG-31018),
   [QTCREATORBUG-33022](https://bugreports.qt.io/browse/QTCREATORBUG-33022))
* That a source file was not mapped to the binaries that link it
  ([QTCREATORBUG-30265](https://bugreports.qt.io/browse/QTCREATORBUG-30265),
   [QTCREATORBUG-27058](https://bugreports.qt.io/browse/QTCREATORBUG-27058))
* That the indentation was lost when renaming a file
* That some `target_` commands were missing from code completion
  ([QTCREATORBUG-34930](https://bugreports.qt.io/browse/QTCREATORBUG-34930))
* That issues could be reported for the wrong line
  ([QTCREATORBUG-31944](https://bugreports.qt.io/browse/QTCREATORBUG-31944))

### qmake

Fixed

* That qmake was passed paths that its device does not know
  ([QTCREATORBUG-27230](https://bugreports.qt.io/browse/QTCREATORBUG-27230))
* That qmake arguments could be split for the wrong device OS

### Qbs

Fixed

* That a source file was not mapped to the binaries that link it
  ([QTCREATORBUG-30265](https://bugreports.qt.io/browse/QTCREATORBUG-30265),
   [QTCREATORBUG-27058](https://bugreports.qt.io/browse/QTCREATORBUG-27058))

### Workspace

Fixed

* That the same project could be opened multiple times

Debugging
---------

Added

* The experimental `Use native combined debugging` option that enables combined
  stack traces when debugging C++ and QML (GDB and LLDB)
* The option `Resolve symbolic links in breakpoint paths`
  ([QTCREATORBUG-17554](https://bugreports.qt.io/browse/QTCREATORBUG-17554))
* Support for passing arguments to the debuggee with the `-debug` command line
  option
  ([QTCREATORBUG-19398](https://bugreports.qt.io/browse/QTCREATORBUG-19398))
* The experimental option to use new debugger backends

Fixed

* That `Copy Current Value to Clipboard` truncated long values
  ([QTCREATORBUG-13192](https://bugreports.qt.io/browse/QTCREATORBUG-13192))
* That the debugger unnecessarily got keyboard focus during startup
  ([QTCREATORBUG-15818](https://bugreports.qt.io/browse/QTCREATORBUG-15818))
* That the user's display format choice was stored based only on the name,
  not the type
  ([QTCREATORBUG-17221](https://bugreports.qt.io/browse/QTCREATORBUG-17221))
* That the source path mappings were not shown the way they were entered
  ([QTCREATORBUG-22001](https://bugreports.qt.io/browse/QTCREATORBUG-22001),
   [QTCREATORBUG-26715](https://bugreports.qt.io/browse/QTCREATORBUG-26715))
* That breakpoints stopped working when a source path mapping is used
  ([QTCREATORBUG-33433](https://bugreports.qt.io/browse/QTCREATORBUG-33433))
* That the views were not refreshed when the display format changes
* That a jump to a line with several locations was performed for an arbitrary
  one
* That the QML console popped up for messages that are filtered out
  ([QTCREATORBUG-19886](https://bugreports.qt.io/browse/QTCREATORBUG-19886))
* The name of the menu in the tool tip of the `Modules` view
  ([QTCREATORBUG-19539](https://bugreports.qt.io/browse/QTCREATORBUG-19539))
* That a scratch buffer kept its display name after it was saved
  ([QTCREATORBUG-12276](https://bugreports.qt.io/browse/QTCREATORBUG-12276))

### C++

Added

* A pretty printer for `QColor`
  ([QTCREATORBUG-34499](https://bugreports.qt.io/browse/QTCREATORBUG-34499))

Changed

* Made the display of classes with a single member more compact in the `Locals`
  view
  ([QTCREATORBUG-22565](https://bugreports.qt.io/browse/QTCREATORBUG-22565))

Fixed

* The display of 256-bit registers in the `Registers` view
* `Start and Break on Main` on Android devices
* The display of two-dimensional arrays in the `Locals` view
  ([QTCREATORBUG-18946](https://bugreports.qt.io/browse/QTCREATORBUG-18946))
* Conditional breakpoints with LLDB
  ([QTCREATORBUG-34583](https://bugreports.qt.io/browse/QTCREATORBUG-34583))
* Interrupting debugging when using the J-Link GDB server
  ([QTCREATORBUG-30508](https://bugreports.qt.io/browse/QTCREATORBUG-30508))
* Issues with parsing peripheral register values
* That `Operate by Instruction` did not show disassembly on macOS/LLDB
  ([QTCREATORBUG-32393](https://bugreports.qt.io/browse/QTCREATORBUG-32393))
* The handling of timeouts while GDB is downloading debug information
  ([QTCREATORBUG-34811](https://bugreports.qt.io/browse/QTCREATORBUG-34811))
* The available display formats for `typedef`s
  ([QTCREATORBUG-7186](https://bugreports.qt.io/browse/QTCREATORBUG-7186))
* That the Qt source path and the Qt namespace were not taken from the debug
  information of the binary
  ([QTCREATORBUG-21007](https://bugreports.qt.io/browse/QTCREATORBUG-21007),
   [QTCREATORBUG-33211](https://bugreports.qt.io/browse/QTCREATORBUG-33211))
* That the locals of a constructor were listed several times
  ([QTCREATORBUG-18165](https://bugreports.qt.io/browse/QTCREATORBUG-18165))
* That a run that ends in an uncaught exception was not explained
  ([QTCREATORBUG-21234](https://bugreports.qt.io/browse/QTCREATORBUG-21234))
* GDB
    * That static class members were missing from the `Locals` view
      ([QTCREATORBUG-14133](https://bugreports.qt.io/browse/QTCREATORBUG-14133))
    * The detection of `gdb-multiarch` and of a crosstool-NG GDB
      ([QTCREATORBUG-34887](https://bugreports.qt.io/browse/QTCREATORBUG-34887))
* LLDB
    * Issues with `Additional startup commands`
      ([QTCREATORBUG-15584](https://bugreports.qt.io/browse/QTCREATORBUG-15584))
    * That registers were fetched twice per step
      ([QTCREATORBUG-17843](https://bugreports.qt.io/browse/QTCREATORBUG-17843))
    * That threads were not ordered and identified by their index ID
      ([QTCREATORBUG-34580](https://bugreports.qt.io/browse/QTCREATORBUG-34580))
    * The display of signed bitfields
* CDB
    * That the output of the debuggee was not shown in `Application Output`
      ([QTCREATORBUG-10626](https://bugreports.qt.io/browse/QTCREATORBUG-10626))
    * The display of multi-dimensional arrays
      ([QTCREATORBUG-18946](https://bugreports.qt.io/browse/QTCREATORBUG-18946))
    * That breaking on a thrown C++ exception was not possible
      ([QTCREATORBUG-11871](https://bugreports.qt.io/browse/QTCREATORBUG-11871))
    * That all registers were dropped when one of them is unreadable
    * That Qt Creator did not tell the user when the system blocks
      `qtcreatorcdbext.dll`

### Python

Fixed

* The inspection of Qt properties of PySide objects
  ([QTCREATORBUG-34195](https://bugreports.qt.io/browse/QTCREATORBUG-34195))

Analyzer
--------

### Profiler

Added

* The new `Profile` mode that integrates the new `Qt Profiler`
    * A `Call-Stack Sampler` for Windows (Event Tracing for Windows)
      and macOS (Mach Task API)
* Trace Viewer
    * Support for the [Common Trace Format version 2](https://diamon.org/ctf/)
      and [version 1.8](https://diamon.org/ctf/v1.8.3/) (in contrast to the
      already supported Chrome Trace Event Format)
      ([QTCREATORBUG-29909](https://bugreports.qt.io/browse/QTCREATORBUG-29909))
    * Opening `.qtd`, `.qzt`, and `.ptq` files in the profiler by opening
      the file (for example on the command line or with `File > Open File`)
    * Opening several traces at once, each in its own editor
    * The option to open the source code behind an event of a trace
* A dashboard for the QML profiler that shows statistics and findings with
  suggestions
* Support for Qt tracepoints as a profiling backend
* The option to show and control `debuginfod` downloads
* Better error reporting when a recording is not permitted or captured no
  samples, including the option to lower `perf_event_paranoid`

### Axivion

Changed

* Simplified triggering a single file analysis for the active project

Fixed

* That an unknown project was silently substituted by another one
* A crash when the project information is not available yet
  ([QTCREATORBUG-33008](https://bugreports.qt.io/browse/QTCREATORBUG-33008))

### Cppcheck

Added

* Support for analyzing files outside the project directory
  ([QTCREATORBUG-25416](https://bugreports.qt.io/browse/QTCREATORBUG-25416))

Terminal
--------

Added

* Support for OSC 8 hyperlinks
  ([QTCREATORBUG-34910](https://bugreports.qt.io/browse/QTCREATORBUG-34910))
* Support for the images that an application draws
  ([QTCREATORBUG-32316](https://bugreports.qt.io/browse/QTCREATORBUG-32316))
* Support for OSC 11 background color queries
* A confirmation prompt for pasting control characters

Changed

* Improved the rendering of box drawing and block characters
* Changed to use a newer console host on Windows, if available

Fixed

* That the scrollback was not reflowed when the terminal is resized
  ([QTCREATORBUG-32648](https://bugreports.qt.io/browse/QTCREATORBUG-32648))
* That the title that the shell sets was not shown in the tab
* That the cursor could move when the terminal is resized
* Various crashes and rendering issues

Version Control Systems
-----------------------

Added

* The `Changes` view that shows changed files and provides actions on these
* Spell checking for submit messages

Fixed

* That files could not be opened from the `Version Control` output pane
  ([QTCREATORBUG-23690](https://bugreports.qt.io/browse/QTCREATORBUG-23690))
* The activation of a link at the end of a match

### Git

Added

* The `Merge`, `Rebase`, and `Stop tracking` actions to the tracked branch in
  the `Branches` view
* Added information about the number of locally modified files to the `Branches`
  view
* The `Tools > Git > Current File > Delete` action
* Shortcut handling (`d`, `p`, `r`, etc) for the various actions on commits
  in the editor for interactive rebases
* `Diff Against Parent` to the tool tip of `Instant Blame`
* The option to use `--patience` for `Git Show` and `Git Diff`
* The `Git Log` navigation view that shows the `git log` graph including the
  stats for the file changes
* Syntax highlighting for the reflog
* `Instant Blame` for documents with unsaved changes
* The diff of an uncommitted line in the tool tip of `Instant Blame`

Changed

* Removed support for Gerrit version earlier than 3
* Increased the width of command output in the output view
  ([QTCREATORBUG-27374](https://bugreports.qt.io/browse/QTCREATORBUG-27374))

Fixed

* Amending commits with `log.showsignature true`
  ([QTCREATORBUG-34756](https://bugreports.qt.io/browse/QTCREATORBUG-34756))
* That instant blame was not available when showing files at a revision
* That opening a commit from the editor for an interactive rebase did not
  find the right repository for a submodule

Test Integration
----------------

Added

* `Debug All Tests Without Deployment`, `Debug Selected Tests`, and `Debug
  Selected Tests Without Deployment`
* Markers for failed tests in the test tree

Fixed

* That disabling a test framework also removed all its data from the test tree
* Running autotests on Android
  ([QTCREATORBUG-25875](https://bugreports.qt.io/browse/QTCREATORBUG-25875))
* The performance of parsing for tests
  ([QTCREATORBUG-34915](https://bugreports.qt.io/browse/QTCREATORBUG-34915))

### Qt Quick

Fixed

* The lookup of derived tests
  ([QTCREATORBUG-25066](https://bugreports.qt.io/browse/QTCREATORBUG-25066))
* The quoting for Qt Quick tests

Platforms
---------

### Windows

Fixed

* Links to Windows paths with drive letters in the
  `Application Output`
  ([QTCREATORBUG-34808](https://bugreports.qt.io/browse/QTCREATORBUG-34808))

### Linux

Added

* Progress information in the task bar or Dock icon
  ([QTCREATORBUG-19219](https://bugreports.qt.io/browse/QTCREATORBUG-19219))

Fixed

* That the global menu vanished after opening a `QMainWindow` `.ui` file
  ([QTBUG-148580](https://bugreports.qt.io/browse/QTBUG-148580))
* That the POSIX ACLs of a file were lost when saving it
  ([QTCREATORBUG-7598](https://bugreports.qt.io/browse/QTCREATORBUG-7598))

### macOS

Fixed

* The copy and paste actions in file dialogs
  ([QTCREATORBUG-31352](https://bugreports.qt.io/browse/QTCREATORBUG-31352))

### Android

Added

* `Tools > Android > Logcat`
* A parser for crashes in the logcat output
  ([QTCREATORBUG-29935](https://bugreports.qt.io/browse/QTCREATORBUG-29935))
* The name for API level 37

Fixed

* Fixed a freeze when resolving emulator names
* That the keystore password was asked for twice
  ([QTCREATORBUG-34712](https://bugreports.qt.io/browse/QTCREATORBUG-34712))

### iOS

Changed

* Replaced the dialog for a device that is in user mode by an information
  popup, and improved the reporting of the pairing and development state of a
  device
  ([QTCREATORBUG-33222](https://bugreports.qt.io/browse/QTCREATORBUG-33222))
* Devices are only reported when a kit for iOS devices is configured

Fixed

* That no useful error message was shown when Rosetta is not installed or the
  architecture of the simulator does not match the application binary
  ([QTCREATORBUG-34070](https://bugreports.qt.io/browse/QTCREATORBUG-34070))

### Remote SSH

Added

* Support for remote Windows devices
* Separate macOS and Windows devices

Fixed

* Performance issues with the SFTP file transfer method
  ([QTCREATORBUG-30378](https://bugreports.qt.io/browse/QTCREATORBUG-30378))
* That an ambient SSH `ControlMaster` was trusted
  ([QTCREATORBUG-34898](https://bugreports.qt.io/browse/QTCREATORBUG-34898))
* The deployment of SSH keys to administrator accounts
* That triggering a run was blocked when the remote executable was not known yet
  ([QTCREATORBUG-34860](https://bugreports.qt.io/browse/QTCREATORBUG-34860))
* That the default display for X11 forwarding was resolved only once
  ([QTCREATORBUG-34872](https://bugreports.qt.io/browse/QTCREATORBUG-34872))

### Development Container

Added

* A confirmation prompt before running a command outside of the container
  ([QTCREATORBUG-34672](https://bugreports.qt.io/browse/QTCREATORBUG-34672),
   [QTCREATORBUG-34557](https://bugreports.qt.io/browse/QTCREATORBUG-34557))

### Docker

Added

* Support for Podman
* The option to mount a host directory elsewhere in the container
  ([QTCREATORBUG-31762](https://bugreports.qt.io/browse/QTCREATORBUG-31762))

Fixed

* That the container start event could be lost, and that a transient failure
  of the command bridge setup was fatal
  ([QTCREATORBUG-34867](https://bugreports.qt.io/browse/QTCREATORBUG-34867))
* That the X11 display was hardcoded
  ([QTCREATORBUG-34872](https://bugreports.qt.io/browse/QTCREATORBUG-34872))

### QNX

Added

* Auto-detection of toolchains and debuggers from an `sdpenv` file on the build
  device

Changed

* Removed the separate `SDK > QNX` settings in favor of the device settings
  and auto-detection

Fixed

* That application output was only shown when debugging
  ([QTCREATORBUG-18906](https://bugreports.qt.io/browse/QTCREATORBUG-18906))
* Cross-compilation with auto-detected QNX kits
* That the device test required commands that are not standard on QNX
  ([QTCREATORBUG-30631](https://bugreports.qt.io/browse/QTCREATORBUG-30631))
* That `pdebug` could not be stopped
  ([QTCREATORBUG-32505](https://bugreports.qt.io/browse/QTCREATORBUG-32505))
* The remote `Custom Executable` run configuration
* The pretty printers, which could be confused by an unknown allocator size
  ([QTCREATORBUG-32959](https://bugreports.qt.io/browse/QTCREATORBUG-32959))

### Bare Metal

Fixed

* The cloning of debug server providers
  ([QTCREATORBUG-25716](https://bugreports.qt.io/browse/QTCREATORBUG-25716))
* That the debugger was started before the debug server was ready

Credits for these changes go to:
--------------------------------
Ahmed El Khazari  
Alessandro Portale  
Andre Hartmann  
André Pönitz  
Assam Boudjelthia  
Christian Kandeler  
Christian Stenger  
corvofeng  
Cristian Adam  
David Schulz  
Dmitrii Akshintsev  
Eike Ziller  
Friedemann Kleint  
Imre Péntek  
Jaroslaw Kobus  
Jeff Heller  
Joni Poikelin  
Jörg Bornemann  
Kai Köhne  
Leena Miettinen  
Marcus Tillmanns  
Mitch Curtis  
Orgad Shaneh  
Paul Olav Tvete  
Sami Shalayel  
Samuel Gaist  
Tasuku Suzuki  
Ulf Hermann  
Xavier Besson  
