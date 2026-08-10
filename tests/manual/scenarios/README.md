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
screenshot dimensions), not volatile ones (screen geometry, window ids, pane
text with timestamps), so it flags behaviour changes rather than cosmetic
noise. Regenerate it with `--update-baseline` when a change is intended.

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

- **Cursor** - before a `click`/`select` (and a targeted `type`) the real
  pointer glides onto the resolved widget (the `move_cursor` MCP tool, which
  warps the pointer in-process), so the recording shows where the action
  lands. The click itself still goes through the widget's slot; the movement
  is cosmetic. Disable with `--no-cursor`.
- **Keystroke bubble** - a `type` step's text is burned in as a bottom-centre
  caption for the duration of that step (ffmpeg `drawtext`).

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

Top level:

- `name` - title, also the default output subdirectory.
- `intent` - one paragraph; appears as a blockquote in the tutorial.
- `setup.open` - a file path to open first (supports `{scratch}`).
- `steps` - a list; each step has a `describe` (the tutorial sentence) plus
  exactly one action key.

Widget queries (`click`, `type`, `select`, `expect`, `expect_gone`,
`wait_for`, and the optional target of `capture`) are maps of the fields the
MCP widget tools understand: `object_name`, `text`, `class_name`,
`window_title`, `include_invisible`.

Action keys:

| Key            | MCP tool            | Notes                                             |
|----------------|---------------------|---------------------------------------------------|
| `invoke_action`| `call_action`       | Value is an action id. Add `blocks: true` for a modal dialog that a later step dismisses. |
| `click`        | `click_widget`      | Query. Must resolve to exactly one widget.        |
| `type`         | `type_text`         | `input:` plus optional query fields.              |
| `select`       | `select_combo_item` | `item:` plus a combo query.                       |
| `expect`       | `widget_exists`     | Fails if nothing matches.                         |
| `expect_gone`  | `widget_exists`     | Fails if anything matches.                        |
| `wait_for`     | `widget_exists`     | Polls until present; `timeout:` seconds (default 15). |
| `read_pane`    | `read_pane`         | Value is a pane display name; text saved as an artefact. |
| `capture`      | `screenshot`        | Optional query selects the window; PNG saved under `shots/`. |

`{scratch}` in any string expands to a fresh per-run temporary directory, so a
run never depends on the developer's home state.

There is deliberately no `sleep`: wait only on observable conditions
(`wait_for`). See `about-dialog.yaml` for a complete example.
