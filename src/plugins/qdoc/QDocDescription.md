Write QDoc documentation with a rendered preview next to the source. The
preview follows your typing, and problems in the markup are reported as you
write them.

### 1. Activate the extension

Activate the **QDoc** extension.

### 2. Open a documentation file

Open a `.qdoc` or `.qdocinc` file. The editor shows the markup on the left and
the preview on the right.

To hide either side, select **Show Editor** or **Show Preview** on the editor
toolbar.

The preview follows the cursor: it scrolls to the documentation comment that
you move the cursor into. To go the other way, double-click the preview to move
the cursor to the comment that the text was rendered from.

### 3. Check the markup

Problems in the markup are reported in **Issues**, and marked in the left
margin of the editor. Select a problem to move to it.

To make a check quieter or to turn it off, go to
**Preferences > Text Editor > QDoc**. If the documentation configuration
silences a warning with `spurious`, the preview reports it one level quieter
and labels it as silenced.

### 4. Preview what the documentation build would show

The preview reads the `.qdocconf` file that governs the file you are editing:
the closest configuration above it that lists the file in `sourcedirs`,
`headerdirs`, or `exampledirs`. With it, macros expand, and `\image`,
`\snippet`, and `\include` find their files.

QDoc is not run, so the preview shows one page at a time. A link to another
page has no target to point at, and markup that needs the whole project, such
as `\annotatedlist`, is shown as a placeholder.
