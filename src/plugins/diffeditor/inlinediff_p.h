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

// The closest line at or above lastLine (1-based) that starts a declaration, the
// way git picks the function name for a hunk header; 0 when there is none.
int inlineDiffDeclarationLine(const QTextDocument *document, int lastLine);

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
