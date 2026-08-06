# Qt Creator Coding Style (condensed)

Condensed from `doc/qtcreatordev/src/coding-style.qdoc`. Follow when writing or
editing Qt Creator code.

## Guiding principles
- KISS: prefer the simpler implementation. Write readable, object-oriented C++.
- Take advantage of src/libs/utils/ and Qt; don't reinvent the wheel.
  Consider whether a piece is generic enough to belong in Qt rather than Creator.
- Adapt to existing Creator structures. Discuss improvement ideas with other developers
  before writing the code.
- Rules are not set in stone: break one if you have a good reason, but get at least
  some other developers to agree first.

## Code constructs
- Prefer pre-increment/decrement (`++it`, `--it`) over post.
- Hoist loop-invariant work out of loops (e.g. cache `end = c.end()`).

## Naming (camelCase)
- Classes: CapitalFirst. Functions/variables: lowerFirst. Enum names & values: CapitalFirst (unscoped values embed part of enum type name).
- No abbreviations. Single-char names only for counters/obvious temporaries.
- One variable per declaration line. Declare variables where first needed. Favor `QString a = "Joe";` idiom.
- Member prefix `m_`, except public struct members. `d`/`q` pointers exempt (named `d`/`q`, not `m_d`; type `FooPrivate *` / `Foo *`). Don't wrap `d` in a smart pointer (compile/link overhead, more symbols).
- `FooPrivate` is declared in the same namespace as `Foo`, or in the corresponding `Internal` namespace if `Foo` is exported. It may be a friend of `Foo` if needed (e.g. to emit its signals).

## Whitespace / formatting
- 4 spaces, no tabs. One blank line max to group statements.
- Pointer/reference: space before `*`/`&`, never after: `char *p`, `char &r`.
- No space: `operator==(type)`, `mangle()`. Space after keyword and before `{`: `if (foo) {`.
- One space after `//`.
- Lines < 100 chars. Commas at end of broken line; operators start the new line.
- Group with parentheses: `if ((a && b) || c)`.

## Braces
- Left brace on same line as statement. Exception: function bodies and class declarations put `{` on its own line.
- Omit braces for single simple-line bodies; use them if the body is >1 line, complex, empty (`while (a) {}`), or the parent statement wraps, or in if/else where either branch spans multiple lines.
- Brace an outer `if` whose body is a nested if/else, to avoid a dangling `else`.

## Declarations
- Access order: public, protected, private.
- Prefer `class` over `struct`. Avoid global objects in class decl (use static member).
- Keep headers slim.

## Namespaces
- `{` on same line as `namespace`. Don't indent contents. Add `// namespace Foo` after closing brace if long.
- Exception: a namespace containing only a single class declaration goes on one line: `namespace MyPlugin { class MyClass; }`.
- No using-directives in headers; don't rely on them for defining classes/functions or accessing global functions. Otherwise OK — place near top after includes (never `#include` after a using-directive).
- Exported symbols in a plugin/lib namespace (`MyPlugin`); non-exported in `MyPlugin::Internal`.

## C++ features
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

## QObject
- Add `Q_OBJECT` only to subclasses using the meta-object system. Prefer Qt5-style `connect()`.
- Avoid `QObject::sender()` - pass sender explicitly via lambda capture. Avoid `QSignalMapper` (use a lambda).

## Passing file names
- Creator API expects portable format (slashes, also on Windows).

## Classes to use / not to use
- `Utils::FilePath` for any QString that semantically is a file or directory; prefer it over `QDir`/`QFileInfo`.
- Prefer `Utils::Process` over `QProcess`.
- If `Utils::FilePath`/`Utils::Process` are insufficient, enhance them rather than fall back to `QString`/`QProcess`.
- Avoid platform `#ifdef`s unless needed for locally executed code; even then prefer `Utils::HostInfo`.

## Plugin dependencies
- Keep hard run-time dependencies between plugins and to external libraries as few as reasonably possible.
- Callback pattern: a leaf plugin injects functionality into a central plugin via a `std::function` accessor (`std::function<void(...)> &fancyLeafCallback();` returning a function-scope static), so the central plugin need not depend on the leaf's dependencies. The central plugin checks the callback and falls back if unset.

## File headers
- New files start with the same header comment as other Creator source files.

## Includes
- `#include <QWhatEver>` (no module prefix). Order specific-generic: own header, other class in plugin, `<otherplugin/...>`, `<QtClass>`, `<stdthing>`, `<system.h>`.
- Angle brackets for other plugins' headers. Blank line between peer-header blocks; alphabetize within a block.

## Casting
- No C casts; use `static_cast`/`const_cast`/`reinterpret_cast`. No `dynamic_cast` — use `qobject_cast` for QObjects.

## Platform / portability
- Beware `?:` with differing types (may crash). Beware alignment when casting pointers to a type with stricter alignment — use a union to force correct alignment.
- Static header declarations: integral types / arrays / structs only.
- Function-scope statics are OK (not reentrant).
- `char` signedness is platform-dependent — use `signed char`/`uchar` explicitly. Avoid 64-bit enum values. Don't mix const/non-const iterators. Don't inline virtual destructors in exported classes (vtable duplication / RTTI break).

## Esthetics & design
- Prefer unscoped enums over `static const int`/defines for constants. Verbose argument names in headers.
- Inheritance for clear is-a; aggregation for orthogonal reuse; prefer aggregation. Beware inheriting template/tool classes (non-virtual dtors, unexported symbols).

## Lambdas
- Drop `()` when no args and no return type: `[] { ... }`. Glue `[]` to `()`: `[](int a) {`.
- Capture-list/params/return/`{` on first line, body indented, `}` on new line. Enclosing `);` on same line as closing `}`. Start lambda on new line inside `if`. One line if it fits.

## Documentation
- Put documentation into .cpp
