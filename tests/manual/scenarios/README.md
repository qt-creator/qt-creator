# UI scenarios

Declarative, replayable UI walkthroughs for Qt Creator. A *scenario* is a
YAML file of semantic steps ("click the Close button in the About Qt Creator
dialog"). `run_scenario.py` drives a real Qt Creator through its built-in MCP
server and generates a Markdown tutorial - numbered prose, the exact call made
at each step, the resolved widget, and screenshots - next to any captured
artefacts. Because the document is generated from what actually ran, it cannot
drift from the behaviour it describes.

This addresses the brittleness of coordinate-based GUI driving: coordinates
encode nothing a reader can verify and rot as soon as a layout changes. A
scenario names widgets instead, by (in order of robustness) `object_name`,
visible `text`, or `class_name` plus `window_title`.

## Requirements

- A built Qt Creator with the `McpServer` plugin.
- Python 3 with PyYAML.
- A display. For headless use, run under Xvfb; the runner does not manage the
  X display itself.

## Running

Attach to an already-running Creator (started with
`-load McpServer -mcp-port <PORT>`):

    ./run_scenario.py about-dialog.yaml --port 8765

Or let the runner launch Creator itself with a throwaway settings directory:

    ./run_scenario.py about-dialog.yaml --qtcreator ../../../bin/qtcreator --port 8765

A launched Creator is set up to keep recordings clean: the "Take a UI Tour?"
and "Link with an Installed Qt?" first-run prompts are suppressed via the
throwaway settings, and the FakeVim plugin is skipped with `-noload FakeVim`
so typed text is inserted rather than read as Vim commands. Pass
`--no-preseed` for a pristine configuration (default settings, all plugins).

`--noload PLUGIN` and `--load PLUGIN` (both repeatable) are passed through to a
launched Creator. Use `--noload` to skip a plugin whose load error would pop a
dialog over the recording (e.g. `--noload Profiler`), or restrict the set with
`--noload all --load <plugin>`. `McpServer` is always loaded regardless, so the
runner can still drive Creator.

The tutorial is written to `out/<scenario-name>/report.md` (override with
`--out`). Exit code is non-zero if any assertion fails.

## Regression mode

A scenario can double as a regression test. Record a baseline of each step's
stable observations next to the scenario:

    ./run_scenario.py about-dialog.yaml --port 8765 --update-baseline

This writes `about-dialog.baseline.json`, which you commit alongside the
scenario. Later, re-run in check mode:

    ./run_scenario.py about-dialog.yaml --port 8765 --check

`--check` fails (non-zero exit) if the run diverges from the baseline - a step
resolving to a different widget, an assertion count changing, a screenshot
resizing, and so on. The baseline deliberately stores only stable fields
(which widget was acted on: class, objectName, text; `widget_exists` counts;
the size of a screenshot that named its window), not volatile ones (screen
geometry, the main window's size, window ids, pane text with timestamps), so it
flags behaviour changes rather than cosmetic noise. A var's value is written
back as its `{name}` placeholder, so the run's own scratch directory (or a path
passed with `--set`) does not end up in the file. Regenerate it with
`--update-baseline` when a change is intended.

## Recording a video

Add `--video` to screen-record the run to `tutorial.mp4` in the output
directory, with each step's `describe` embedded as a chapter marker:

    ./run_scenario.py about-dialog.yaml --port 8765 --video

This records the current `$DISPLAY` with ffmpeg `x11grab` (so it works under
Xvfb), muxes chapters from the step timestamps, and links the video with a
chapter list from `report.md`. Override the capture size with
`--video-size WxH` if auto-detection picks the wrong geometry. Requires
`ffmpeg` with `libx264`.

To make the recording readable it adds two cues:

- **Cursor** - before a click, select or targeted type step, the real
  pointer glides onto the resolved widget (the `move_cursor` MCP tool, which
  warps the pointer in-process), so the recording shows where the action
  lands. The click itself still goes through the widget's slot; the movement
  is cosmetic. Disable with `--no-cursor`.
- **Keystroke bubble** - a `type_text` step's text is burned in as a
  bottom-centre caption for the duration of that step (ffmpeg `drawtext`).

Steps run in milliseconds, so video mode paces the run with a per-step dwell
(`--video-dwell`, default 1.5s) so each state and cue stays on screen. The
dwell is video-only; a `--check` run never waits.

By default a headless (Xvfb) display has no window manager, so windows are
undecorated. Pass `--window-manager CMD` to run a window manager on `$DISPLAY` so the
recording shows title bars and borders; it is terminated at the end. A tiny
`twmrc` is provided (plain `twm` would block on interactive window placement):

    ./run_scenario.py demo.yaml --qtcreator ../../../bin/qtcreator --port 8765 \
        --video --window-manager "twm -f twmrc"

Point it at a dedicated display, not your desktop. This implies full-display
capture (a frame sits outside the app's client area), so size the display
close to the window - or pass an explicit `--video-size`.

## Scenario format

`scenario.schema.json` (next to this README) is a JSON Schema for this
format. Editors with a `yaml-language-server` integration (e.g. VS Code with
`redhat.vscode-yaml`) will validate and autocomplete a scenario file that
starts with:

    # yaml-language-server: $schema=./scenario.schema.json

Top level:

- `name` - title, also the default output subdirectory.
- `intent` - one paragraph; appears as a blockquote in the tutorial.
- `vars` - placeholders the steps use as `{name}` (see below).
- `setup.open` - a file path to open first (supports `{scratch}`).
- `steps` - a list; each step has a `describe` (the tutorial sentence) plus
  exactly one action key.

Widget queries (`click_widget`, `type_text`, `select_combo_item`, `expect`,
`expect_gone`, `wait_for`, and the optional target of `screenshot`) are maps of
the fields the MCP widget tools understand: `object_name`, `text`,
`class_name`, `window_title`, `include_invisible`.

Action keys mirror the MCP tool names. Each step has exactly one:

| Key                 | Notes |
|---------------------|-------|
| `call_action`       | Value is an action id. Add `blocks: true` for a modal dialog that a later step dismisses, or `optional: true` where the action may legitimately be disabled (a tidying step with nothing to do). |
| `click_widget`      | Query; must resolve to exactly one widget. |
| `type_text`         | `input:` plus optional query fields. |
| `press_keys`        | A key/chord, e.g. `press_keys: "Ctrl+K"` or `press_keys: {keys: Escape, ...query}`. |
| `select_combo_item` | `item:` plus a combo query. |
| `menu`              | Navigate a menu with the cursor, e.g. `menu: [Help, About Qt Creator]`; drives `find_menu_item` + `activate_menu_item` (needs a DISPLAY). |
| `expect`            | `widget_exists`; fails if nothing matches. |
| `expect_gone`       | `widget_exists`; fails if anything matches. |
| `wait_for`          | `widget_exists`; polls until present; `timeout:` seconds (default 15). |
| `read_pane`         | Value is a pane display name; text saved as an artefact. |
| `screenshot`        | Optional query selects the window; PNG saved under `shots/`. |
| `click_item`        | A query for a tree, list or table plus `item:`, the row's full path as `find_items` reports it. `double_click:`/`context_menu:` where a view wants those. |
| `open`              | A file path to open in the editor (`setup.open` covers the first one). |
| `select_text`       | `start_line`/`end_line` plus optional columns in the current editor; `expect:` asserts the selected text. |
| `activate_mode`     | A mode id, e.g. `Welcome`. |
| `settings_page`     | A preferences page id, e.g. `D.ProjectExplorer.KitsOptions`. |
| `build`             | Builds the startup project and fails on a build error; `timeout:` seconds (default 300). |
| `run`               | Runs it. Dispatched, not awaited (see below). |
| `wait_for_output`   | `text:` plus `pane:` (default Application Output) and `timeout:`; polls the pane until a line contains the text. Only what the dispatched run itself wrote counts. |
| `remove`            | A path to remove recursively, so a scenario can start from nothing. Removing what is not there succeeds. |

`{scratch}` in any string expands to a fresh per-run temporary directory, so a
run never depends on the developer's home state.

For keyboard shortcuts, prefer `call_action` (focus-independent, reliably
triggers the effect). Use `press_keys` when a widget handles the key itself
(Return, Escape, Tab, arrows) or when the tutorial should show the keystroke;
a synthetic key event does not always drive application-wide shortcuts.

`call_action` triggers an action directly, so nothing visible happens on the
way to its effect. In a video, either add a `caption:` to any step (shown as
an on-screen overlay, e.g. `caption: "Help > About Qt Creator"`) to name what
is happening, or use a `menu:` step to actually navigate the menu with the
cursor. `menu` opens submenus and triggers the item through the menu API
(`activate_menu_item`), so it also triggers the effect - no separate
`call_action` needed.

`run` is dispatched on its own connection rather than awaited: the MCP
`run_project` tool returns when the application exits, which a windowed
application does not do by itself. What the run did is observed with
`wait_for_output` on the Application Output, and `ProjectExplorer.Stop` ends
it. The pane keeps what earlier runs wrote, and a needle as general as the
project name matches those lines too, so `run` notes how long the pane is and
`wait_for_output` looks only past that mark. Without it the second run of a
scenario matches the first run's output and stops an application that never
started. `build` does wait, attaching to the running build by its id for as long as
the step's `timeout` allows.

Preferences is a mode, not a modal dialog, so a `settings_page` step returns at
once and Escape does not leave the page. Switch away with `activate_mode`.

`select_text` selects, so a following `Return` would replace the line. Add an
`End` (or `Home`) `press_keys` step to put the caret at one end of the
selection first. The `expect:` field is what keeps a hard-coded line number
honest: it fails the moment the line means something else.

There is deliberately no `sleep`: wait only on observable conditions
(`wait_for`, `wait_for_output`). See `about-dialog.yaml` for a small complete
example and `cmake-project.yaml` for a whole development story - detected
device and kit, the wizard, an edit, a build and a run.

## Vars

A scenario declares its own placeholders under `vars` and uses them as
`{name}` in any string. `--set NAME=VALUE` (repeatable) overrides one per run:

    vars:
      workspace: "{scratch}"
      kit: Manual

    ./run_scenario.py cmake-project.yaml --port 8765 --set kit="Manual / Desktop"

This is what keeps a machine-specific path, project name or kit out of the
file. A var's value may itself use `{scratch}`, which is how a default stays
self-contained. Keep the defaults working on a plain desktop build, so the
scenario runs with no `--set` at all.

## Driving a Qt Creator that runs elsewhere

The runner only needs an MCP port, so a Qt Creator on another machine or on a
device is driven by the same scenario file once its port is forwarded (for
example `hdc fport tcp:8767 tcp:8767` for a HarmonyOS device, then
`--port 8767`). Two things change:

- **Paths in the scenario are the remote side's.** `{scratch}` is a directory
  on the machine the runner itself is on, so it is only a usable location when
  Qt Creator shares that file system. Otherwise pass the remote path in with
  `--set`.
- **The DISPLAY-bound parts do not apply**: `--video` records the local
  `$DISPLAY`, `--window-manager` starts one on it, and a `menu` step drives
  the menu with the local pointer. Everything else is in-process in the
  Creator being driven, so it works regardless of where that is.
- **Screenshots still land next to the tutorial.** `ui_screenshot` writes the
  file where Qt Creator runs, so when that file does not turn up locally the
  runner asks for the image itself and writes it under `shots/`.
