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

    // editor side, in editor line numbers
    QList<TextEditor::InlineDiffDecorator::GhostBlock> ghosts; // inline view only
    QList<TextEditor::InlineDiffDecorator::ChangedRange> changes;
    QList<TextEditor::InlineDiffDecorator::Spacer> editorSpacers; // side by side only
    // baseline side, in baseline line numbers (side by side view only)
    QList<TextEditor::InlineDiffDecorator::ChangedRange> baselineChanges;
    QList<TextEditor::InlineDiffDecorator::Spacer> baselineSpacers;
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
// Returns nullptr if the document is too large for live diffing; callers
// should fall back to a regular diff view then.
DIFFEDITOR_EXPORT Core::IEditor *openInlineDiffEditor(
    const TextEditor::TextDocumentPtr &sourceDocument,
    const InlineDiffBaseline &baseline,
    const QString &title);

enum class InlineDiffViewMode { Inline, SideBySide };

// The view mode of an editor returned by openInlineDiffEditor: fully inline,
// or the baseline in a read only view side by side with the editable text.
DIFFEDITOR_EXPORT void setInlineDiffViewMode(Core::IEditor *editor, InlineDiffViewMode mode);
DIFFEDITOR_EXPORT InlineDiffViewMode inlineDiffViewMode(Core::IEditor *editor);

} // namespace DiffEditor
