# Qt Creator

This project maintains two parallel build system descriptions: CMake (`CMakeLists.txt`) and qbs (`.qbs` files). They must be kept in sync.

## Build system sync rule

Whenever you modify a `CMakeLists.txt` file, also update the corresponding `.qbs` file in the same directory (and vice versa). The two files describe the same targets, sources, and dependencies — changes to one must be reflected in the other.

## Commit message rule

Commit message titles must not exceed 72 characters, and commit message body lines must not exceed 80 characters.

Commit messages should follow this structure:
1. A short title summarizing the change
2. A brief description of what was changed and why, if not clear from the title or code diff
3. Detailed explanation only when necessary

When a commit addresses Coverity diagnostics, include the Coverity IDs in a
Coverity-Id footer.

Make sure to not change the Change-Id on the last line of a commit if you change its commit message.

## UI design rules

- Use `Utils::creatorColor()` or `QPalette::color()` for `QColor`. No hard-coded colors, no alpha-blended text.
- Use `Utils::StyleHelper::uiFont()` for fonts. No manual `QFont::setPixelSize/setPointSize/setBold` etc.
- Use `Utils::SpacingTokens` for margins/spacings/paddings. No hard-coded pixel numbers.

## Additional coding style rules

- When calling free functions from the Utils namespace, always qualify the call with the `Utils::` namespace.
- Do not use Q_ASSERT, use QTC_ASSERT, QTC_CHECK, and QTC_GUARD as appropriate instead.
