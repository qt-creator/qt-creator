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
