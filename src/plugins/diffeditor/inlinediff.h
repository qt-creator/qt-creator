// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "diffeditor_global.h"

#include <texteditor/inlinediffdecorator.h>
#include <texteditor/textdocument.h>

#include <utils/filepath.h>
#include <utils/result.h>

#include <functional>

namespace Core { class IEditor; }

namespace DiffEditor {

class ChunkData;

// 1-based, inclusive line ranges <first, last>
using InlineDiffLineRanges = QList<QPair<int, int>>;

// One block of changes: a range of editor lines paired with the baseline
// lines it replaces.
class DIFFEDITOR_EXPORT InlineDiffChunk
{
public:
    int editorStartLine = 1;   // first changed line in the editor, or the
                               // line a pure removal is shown above
    int editorLineCount = 0;   // 0 for pure removals
    int baselineStartLine = 1; // analogous for the baseline side
    QStringList baselineLines; // empty for pure additions
};

// Describes what the editor contents are compared against, e.g. the git index
// version or some other revision of the file.
class DIFFEDITOR_EXPORT InlineDiffBaseline
{
public:
    using TextCallback = std::function<void(const Utils::Result<QString> &)>;

    bool isValid() const { return bool(fetchText); }

    QString id;                       // e.g. "git-index", "git-rev:<sha>"
    QString displayName;              // e.g. "Index", "HEAD", a short sha
    Utils::FilePath contextDirectory; // repository top level, enables auto refresh
    // Asynchronous provider for the baseline contents. The callback must be
    // invoked on the main thread with '\n' line endings.
    std::function<void(const TextCallback &)> fetchText;
    // Optional: called for the read only baseline view of the side by side
    // mode, e.g. to attach revision annotations. Attached objects should
    // parent themselves to the widget; the view is recreated when the
    // editor is re-targeted to another baseline.
    std::function<void(TextEditor::TextEditorWidget *)> setupBaselineView;
    // Optional: takes over the editor's state of the hunk's lines, e.g.
    // staging them to the git index. editorText is the full current editor
    // contents; implementations derive what to apply from it, so that
    // staging works regardless of which baseline is displayed.
    std::function<void(const InlineDiffChunk &hunk, const QString &editorText)> stageHunk;
    // Optional: reports (asynchronously, on the main thread) which lines of
    // the given editor contents the hunk actions would affect, e.g. the
    // lines with unstaged changes. When set, the hunk controls are only
    // shown for blocks overlapping these lines - without it, all blocks get
    // controls, which fits baselines where every difference is actionable.
    std::function<void(const QString &editorText,
                       const std::function<void(const InlineDiffLineRanges &)> &callback)>
        fetchActionableLines;
};

// The result of diffing the baseline against the editor contents, expressed
// in terms of the InlineDiffDecorator. The editor side data feeds the inline
// view, together with the baseline side data it feeds the side by side view.
class DIFFEDITOR_EXPORT InlineDiffRenderModel
{
public:
    bool isEmpty() const
    {
        return ghosts.isEmpty() && changes.isEmpty() && baselineChanges.isEmpty();
    }

    // trailing newline state of the compared texts; a difference in it has
    // no visible line of its own (see the phantom row handling), but e.g.
    // reverting a hunk at the end of the file has to take it into account
    bool baselineEndsWithNewline = false;
    bool editorEndsWithNewline = false;

    // editor side, in editor line numbers
    QList<TextEditor::InlineDiffDecorator::GhostBlock> ghosts; // inline view only
    QList<TextEditor::InlineDiffDecorator::ChangedRange> changes;
    // baseline side, in baseline line numbers (side by side view only)
    QList<TextEditor::InlineDiffDecorator::ChangedRange> baselineChanges;
    // both sides paired, drives the side by side row alignment and the per
    // hunk actions
    QList<InlineDiffChunk> hunks;
};

// exported for the autotest; the flags identify the phantom "line" after a
// trailing newline on each side, which never produces decorations
DIFFEDITOR_EXPORT InlineDiffRenderModel mapChunkToRenderModel(
    const ChunkData &chunk,
    bool baselineEndsWithNewline = false,
    bool editorEndsWithNewline = false);

// Opens (or reuses and re-targets) an editor with the given title that shows
// the differences between the baseline and the document contents inline. The
// editor shares the text with sourceDocument, so edits show up immediately in
// its regular editors as well, which stay free of diff decorations.
// With readOnlySource set, the document is shown read only with a generic
// highlighter, e.g. for revision snapshots that have no regular editors.
// Returns nullptr if the document is too large for live diffing; callers
// should fall back to a regular diff view then.
DIFFEDITOR_EXPORT Core::IEditor *openInlineDiffEditor(
    const TextEditor::TextDocumentPtr &sourceDocument,
    const InlineDiffBaseline &baseline,
    const QString &title,
    bool readOnlySource = false);

// The text editor widget showing the document side of an inline diff editor,
// e.g. for attaching revision annotations to a read only snapshot.
DIFFEDITOR_EXPORT TextEditor::TextEditorWidget *inlineDiffEditorWidget(Core::IEditor *editor);

enum class InlineDiffViewMode { Inline, SideBySide };

// The view mode of an editor returned by openInlineDiffEditor: fully inline,
// or the baseline in a read only view side by side with the editable text.
DIFFEDITOR_EXPORT void setInlineDiffViewMode(Core::IEditor *editor, InlineDiffViewMode mode);
DIFFEDITOR_EXPORT InlineDiffViewMode inlineDiffViewMode(Core::IEditor *editor);

} // namespace DiffEditor
