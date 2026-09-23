// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

QT_BEGIN_NAMESPACE
class QMenu;
class QTextCursor;
class QTextDocument;
QT_END_NAMESPACE

namespace Core { class IEditor; }
namespace TextEditor { class TextEditorWidget; }

namespace DiffEditor {

// The declaration a collapsed region belongs to, shown on its placeholder the
// way git puts the function name on a hunk header. With searchForward false,
// find the closest qualifying line at or above lastLine (1-based). With
// searchForward true, find the closest qualifying line below lastLine.
QString inlineDiffContextLine(const QTextDocument *document, int lastLine,
                              bool searchForward = false);

enum class InlineDiffViewMode { Inline, SideBySide };

// The view mode of an editor returned by openInlineDiffEditor: fully inline,
// or the baseline in a read only view side by side with the editable text.
void setInlineDiffViewMode(Core::IEditor *editor, InlineDiffViewMode mode);
InlineDiffViewMode inlineDiffViewMode(Core::IEditor *editor);

// Adds the entries the view's context menu holds at the given cursor, which is
// what a right click there would show. The view is one of the editor's, e.g.
// from inlineDiffEditorWidget(). The entries belong to the menu.
void fillInlineDiffContextMenu(Core::IEditor *editor,
                               TextEditor::TextEditorWidget *view,
                               QMenu *menu,
                               const QTextCursor &cursor);

} // namespace DiffEditor
