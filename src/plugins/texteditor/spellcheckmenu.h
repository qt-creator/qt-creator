// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

QT_BEGIN_NAMESPACE
class QMenu;
class QTextCursor;
QT_END_NAMESPACE

namespace TextEditor {

class SyntaxHighlighter;

// Offers the corrections for the misspelled word that highlighter has marked at the
// position of cursor, if there is one, at the top of menu, along with the two ways to
// tell the dictionary that the word is one after all.
TEXTEDITOR_EXPORT void addSpellingActions(QMenu *menu, SyntaxHighlighter *highlighter,
                                          const QTextCursor &cursor);

} // namespace TextEditor
