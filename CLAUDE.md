# Qt Creator

## Build system sync rule

This project maintains two parallel build system descriptions: CMake
(`CMakeLists.txt`) and qbs (`.qbs` files). They must be kept in sync.

Whenever you modify a `CMakeLists.txt` file, also update the corresponding
`.qbs` file in the same directory (and vice versa). The two files describe
the same targets, sources, and dependencies — changes to one must be
reflected in the other.

## Building and running tests

Build and test through a Qt Creator MCP server when the instance behind it has
this checkout open: ask `project_get_current` and compare its
`project_directory` with the directory you are working in. An agent started
from within Qt Creator always talks to that instance. Use these instead of
invoking a compiler or build tool from the shell:

- `build_project` to start a build; it returns a `build_id` and does not wait.
  `build_get_status` then waits for that build and reports whether it
  succeeded. `build_cancel` stops one.
- `build_get_issues` and `build_get_compile_output` / `cpp_get_file_problems`
  to read what failed, passing the same `build_id`. Do not re-run the build
  just to see its output again.
- `test_run`, `test_get_status`, `test_get_last_results`, `test_get_details`
  for tests.
- `build_list_configs`, `build_get_current_config` and `build_switch_config`
  when the target configuration matters. Do not switch it without saying so.

The instance behind the MCP server is the user's own editor: it holds their
build configuration, and its build directory is the one they look at. Use it so
that what you build is what they see.

Build from the shell when `project_directory` is a different directory, which
includes another checkout of this same repository, when no Qt Creator MCP
server is connected, or when the user's own instructions call for it — and say
which one you used. Never drive a shell build and an MCP build of the same build
directory in parallel; they fight over the same files.

## Commit message rules

- Lines must not exceed 72 characters.
- Structure: (1) short title summarizing the change, (2) brief description
  of what changed and why, if not clear from the title or diff, (3) detailed
  explanation only when necessary.
- Commits addressing Coverity diagnostics include the Coverity IDs in a
  `Coverity-Id` footer.
- Commits fixing a regression introduced by an earlier commit reference it
  in an `Amends <full-sha>.` footer (full 40-character hash, trailing
  period). The `Amends` line must come before other footer fields (such as
  `Task-number` and `Change-Id`), separated from them by a blank line.
- Commits written with AI assistance carry an `Assisted-by: Claude Code`
  footer, placed after `Fixes`/`Task-number` and directly before
  `Change-Id`. Never use a `Co-Authored-By` footer.
- Never change the `Change-Id` trailer on the last line of a commit when
  editing its message. The Change-Id identifies the Gerrit review;
  changing it orphans that review and opens a new one. This requires an
  active step, not just intent: the Gerrit `commit-msg` hook regenerates a
  fresh Change-Id whenever an amended message lacks one, so rewriting the
  message (e.g. `git commit --amend -F -`) silently changes it. Before
  amending, read the current trailer with

      git log -1 --format=%b | grep -i change-id

  and re-append that exact `Change-Id:` line as the last line of the new
  message (after any `Amends`/`Task-number` footers).

## UI design rules

- Use `Utils::creatorColor()` or `QPalette::color()` for `QColor`. No
  hard-coded colors, no alpha-blended text.
- Use `Utils::StyleHelper::uiFont()` for fonts. No manual
  `QFont::setPixelSize/setPointSize/setBold` etc.
- Use `Utils::SpacingTokens` for margins/spacings/paddings. No hard-coded
  pixel numbers.

## Testing rules

- Never use `QTest::qWait()`, `QThread::sleep()` or any other wall-clock wait
  in a test. Bound every wait on a causal signal instead - `QTRY_VERIFY*` on a
  state or event the code under test must produce, or a round trip whose answer
  proves the earlier command was processed.
- Asserting that something does *not* happen ("no error is reported") has no
  event to wait for, which is exactly what tempts a sleep. Use ordering
  instead: wait for an event that is guaranteed to arrive *after* the moment in
  question, and check the absence once it has. Commands are processed in order,
  so if the unwanted event were coming, it would already be there. Where no
  such later event exists, `QSKIP` with an honest reason.
- Never raise a shared timeout to make one flaky test pass. Fix the test.
- An assertion that cannot fail is worse than no assertion. Before claiming a
  test covers a fix, disable the fix and confirm the test goes red.

## Code style rules

Condensed from `doc/qtcreatordev/src/coding-style.qdoc`, which is the full
reference. Follow when writing or editing Qt Creator code.

### Comments and documentation
- Put documentation into .cpp
- Do not describe the change you are making in the source: that belongs in the commit message.
- Otherwise treat a comment as an indication of a code smell, and comment only what is not evident from the code. Needing one usually means the code should be clearer.
- Out of the source entirely: notes aimed at the reviewer, where code was taken from, bug numbers. Exception: a workaround for a bug outside Creator does name it (`// Work around QTBUG-12345.`).
- When editing an existing comment, keep the wording close to the original.

### Guiding principles
- KISS: prefer the simpler implementation. Write readable, object-oriented C++.
- Take advantage of src/libs/utils/ and Qt; don't reinvent the wheel.
  Consider whether a piece is generic enough to belong in Qt rather than Creator.
- Adapt to existing Creator structures. Discuss improvement ideas with other developers
  before writing the code.
- Rules are not set in stone: break one if you have a good reason, but get at least
  some other developers to agree first.

### Code constructs
- Prefer pre-increment/decrement (`++it`, `--it`) over post.
- Hoist loop-invariant work out of loops (e.g. cache `end = c.end()`).

### Naming (camelCase)
- Classes: CapitalFirst. Functions/variables: lowerFirst. Enum names & values: CapitalFirst (unscoped values embed part of enum type name).
- No abbreviations. Single-char names only for counters/obvious temporaries.
- One variable per declaration line. Declare variables where first needed. Favor `QString a = "Joe";` idiom.
- Member prefix `m_`, except public struct members. `d`/`q` pointers exempt (named `d`/`q`, not `m_d`; type `FooPrivate *` / `Foo *`). Don't wrap `d` in a smart pointer (compile/link overhead, more symbols).
- `FooPrivate` is declared in the same namespace as `Foo`, or in the corresponding `Internal` namespace if `Foo` is exported. It may be a friend of `Foo` if needed (e.g. to emit its signals).

### Whitespace / formatting
- 4 spaces, no tabs. One blank line max to group statements.
- Pointer/reference: space before `*`/`&`, never after: `char *p`, `char &r`.
- No space: `operator==(type)`, `mangle()`. Space after keyword and before `{`: `if (foo) {`.
- One space after `//`.
- Lines < 100 chars. Commas at end of broken line; operators start the new line.
- Group with parentheses: `if ((a && b) || c)`.

### Braces
- Left brace on same line as statement. Exception: function bodies and class declarations put `{` on its own line.
- Omit braces for single simple-line bodies; use them if the body is >1 line, complex, empty (`while (a) {}`), or the parent statement wraps, or in if/else where either branch spans multiple lines.
- Brace an outer `if` whose body is a nested if/else, to avoid a dangling `else`.

### Declarations
- Access order: public, protected, private.
- Prefer `class` over `struct`. Avoid global objects in class decl (use static member).
- Keep headers slim.

### Namespaces
- `{` on same line as `namespace`. Don't indent contents. Add `// namespace Foo` after closing brace if long.
- Exception: a namespace containing only a single class declaration goes on one line: `namespace MyPlugin { class MyClass; }`.
- No using-directives in headers; don't rely on them for defining classes/functions or accessing global functions. Otherwise OK — place near top after includes (never `#include` after a using-directive).
- Exported symbols in a plugin/lib namespace (`MyPlugin`); non-exported in `MyPlugin::Internal`.
- Qualify calls to free functions from the `Utils` namespace with `Utils::`, even where a using-directive makes it unnecessary.

### C++ features
- `#pragma once`, not header guards. No exceptions, RTTI, `dynamic_cast`, or virtual inheritance unless truly needed.
- Use templates wisely, not just because you can.
- ASCII-only source (use `\nnn`/`\xnn` escapes; in docs use qdoc `\unicode` or the relevant macro).
- `static` over anonymous namespaces (anonymous namespaces mandate external linkage).
- `nullptr` (not NULL/0). Exception: imported third-party code and code interfacing native APIs may use NULL or 0.
- Use `auto` only to avoid repeating a type in the same statement or for iterators; skip if it hurts readability.
- Scoped enums where int-conversion is undesired. Delegating constructors when ctors share code. Initializer lists for containers. Curly-brace init follows same rules as round; don't overuse. Exception: LayoutBuilder item inialization.
- Non-static data member init for trivial cases, except public exported classes.
- Use `=default`/`=delete`. Use `final` for non-inheritable classes and terminal overrides; `override` otherwise; never `virtual` on `final`. Mark all overrides in a class consistently.
- Range-based for: use `std::cref()` if read-only and constness/sharing unclear (avoid detach).
- `std::optional`: avoid throwing `value()`; check then use `*`/`->` or `value_or()`.

### QObject
- Add `Q_OBJECT` only to subclasses using the meta-object system. Prefer Qt5-style `connect()`.
- Avoid `QObject::sender()` - pass sender explicitly via lambda capture. Avoid `QSignalMapper` (use a lambda).

### Passing file names
- Creator API expects portable format (slashes, also on Windows).

### Classes to use / not to use
- `Utils::FilePath` for any QString that semantically is a file or directory; prefer it over `QDir`/`QFileInfo`.
- Prefer `Utils::Process` over `QProcess`.
- If `Utils::FilePath`/`Utils::Process` are insufficient, enhance them rather than fall back to `QString`/`QProcess`.
- Avoid platform `#ifdef`s unless needed for locally executed code; even then prefer `Utils::HostInfo`.

### Assertions
- Use `QTC_ASSERT(cond, action)` (runs `action`, typically `return`, `return {}`, `continue`, `break`), `QTC_CHECK(cond)` (reports only) or `QTC_GUARD(cond)` (reports and evaluates to the condition) from `utils/qtcassert.h`, not `Q_ASSERT`.
- Unlike `Q_ASSERT` these also report in release builds and none of them aborts.

### Plugin dependencies
- Keep hard run-time dependencies between plugins and to external libraries as few as reasonably possible.
- Callback pattern: a leaf plugin injects functionality into a central plugin via a `std::function` accessor (`std::function<void(...)> &fancyLeafCallback();` returning a function-scope static), so the central plugin need not depend on the leaf's dependencies. The central plugin checks the callback and falls back if unset.

### File headers
- New files start with the same header comment as other Creator source files.

### Includes
- `#include <QWhatEver>` (no module prefix). Order specific-generic: own header, other class in plugin, `<otherplugin/...>`, `<QtClass>`, `<stdthing>`, `<system.h>`.
- Angle brackets for other plugins' headers. Blank line between peer-header blocks; alphabetize within a block.

### Casting
- No C casts; use `static_cast`/`const_cast`/`reinterpret_cast`. No `dynamic_cast` — use `qobject_cast` for QObjects.

### Platform / portability
- Beware `?:` with differing types (may crash). Beware alignment when casting pointers to a type with stricter alignment — use a union to force correct alignment.
- Static header declarations: integral types / arrays / structs only.
- Function-scope statics are OK (not reentrant).
- `char` signedness is platform-dependent — use `signed char`/`uchar` explicitly. Avoid 64-bit enum values. Don't mix const/non-const iterators. Don't inline virtual destructors in exported classes (vtable duplication / RTTI break).

### Esthetics & design
- Prefer unscoped enums over `static const int`/defines for constants. Verbose argument names in headers.
- Inheritance for clear is-a; aggregation for orthogonal reuse; prefer aggregation. Beware inheriting template/tool classes (non-virtual dtors, unexported symbols).

### Lambdas
- Drop `()` when no args and no return type: `[] { ... }`. Glue `[]` to `()`: `[](int a) {`.
- Capture-list/params/return/`{` on first line, body indented, `}` on new line. Enclosing `);` on same line as closing `}`. Start lambda on new line inside `if`. One line if it fits.
