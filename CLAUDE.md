# Qt Creator

This project maintains two parallel build system descriptions: CMake (`CMakeLists.txt`) and qbs (`.qbs` files). They must be kept in sync.

## Build system sync rule

Whenever you modify a `CMakeLists.txt` file, also update the corresponding `.qbs` file in the same directory (and vice versa). The two files describe the same targets, sources, and dependencies — changes to one must be reflected in the other.

## Building and running tests

Build and test through a Qt Creator MCP server when the instance behind it has
this checkout open: ask `get_current_project` and compare its
`project_directory` with the directory you are working in. An agent started
from within Qt Creator always talks to that instance. Use these instead of
invoking a compiler or build tool from the shell:

- `build` to build, `get_build_status` to see whether it is still running and
  whether it succeeded.
- `get_compile_output` and `list_issues` / `get_file_problems` to read what
  failed. Do not re-run the build just to see its output again.
- `run_tests`, `get_test_status`, `get_last_test_results`, `get_test_details`
  for tests.
- `list_build_configs`, `get_current_build_config` and `switch_build_config`
  when the target configuration matters. Do not switch it without saying so.

The instance behind the MCP server is the user's own editor: it holds their
build configuration, and its build directory is the one they look at. Use it so
that what you build is what they see.

Build from the shell when `project_directory` is a different directory, which
includes another checkout of this same repository, when no Qt Creator MCP
server is connected, or when the user's own instructions call for it — and say
which one you used. Never drive a shell build and an MCP build of the same build
directory in parallel; they fight over the same files.

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

## Code Style Guide

- Always follow the rules in STYLE.md
- Do not describe your changes in the source. What changed, and why, belongs in
  the commit message.
- Comment only genuinely non-obvious code, and then tersely.
- No comments narrating where a file came from, or restating what the code does.
- Leave out bug numbers: provenance is discoverable via `git blame` and the
  commit's `Fixes:` or `Task-number:` trailer.

## Additional coding style rules

- When calling free functions from the Utils namespace, always qualify the call with the `Utils::` namespace.
- Do not use Q_ASSERT, use QTC_ASSERT, QTC_CHECK, and QTC_GUARD as appropriate instead.
