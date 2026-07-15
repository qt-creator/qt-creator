# Qt Creator

This project maintains two parallel build system descriptions: CMake (`CMakeLists.txt`) and qbs (`.qbs` files). They must be kept in sync.

## Build system sync rule

Whenever you modify a `CMakeLists.txt` file, also update the corresponding `.qbs` file in the same directory (and vice versa). The two files describe the same targets, sources, and dependencies — changes to one must be reflected in the other.

## Commit message rule

Commit message lines must not exceed 72 characters.

Commit messages should follow this structure:
1. A short title summarizing the change
2. A brief description of what was changed and why, if not clear from the title or code diff
3. Detailed explanation only when necessary

When a commit addresses Coverity diagnostics, include the Coverity IDs in a
Coverity-Id footer.

When a commit fixes a regression introduced by a specific earlier commit,
reference that commit in an `Amends <full-sha>.` footer (full 40-character
hash, trailing period). The `Amends` line must come before other footer
fields (such as `Task-number` and `Change-Id`) and be separated from them by
a blank line.

Never change the `Change-Id` trailer on the last line of a commit when you
edit its message. The Change-Id identifies the Gerrit review; changing it
orphans that review and opens a new one.

This requires an active step, not just intent: the Gerrit `commit-msg` hook
regenerates a fresh Change-Id whenever an amended message lacks one, so
rewriting the message (e.g. `git commit --amend -F -`) silently changes it.
Before amending, read the current trailer with

    git log -1 --format=%b | grep -i change-id

and re-append that exact `Change-Id:` line as the last line of the new
message (after any `Amends`/`Task-number` footers).

## UI design rules

- Use `Utils::creatorColor()` or `QPalette::color()` for `QColor`. No hard-coded colors, no alpha-blended text.
- Use `Utils::StyleHelper::uiFont()` for fonts. No manual `QFont::setPixelSize/setPointSize/setBold` etc.
- Use `Utils::SpacingTokens` for margins/spacings/paddings. No hard-coded pixel numbers.

## Additional coding style rules

- When calling free functions from the Utils namespace, always qualify the call with the `Utils::` namespace.
- Do not use Q_ASSERT, use QTC_ASSERT, QTC_CHECK, and QTC_GUARD as appropriate instead.
