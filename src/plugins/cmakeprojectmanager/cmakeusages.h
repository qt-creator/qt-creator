// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace TextEditor { class TextEditorWidget; }

namespace CMakeProjectManager::Internal {

// Where the symbol under the cursor is named: a function, macro, target or
// variable, wherever the project spells it out.
void findUsagesUnderCursor(TextEditor::TextEditorWidget *editorWidget);

// The same places, offered for a name of the user's choosing.
void renameSymbolUnderCursor(TextEditor::TextEditorWidget *editorWidget);

#ifdef WITH_TESTS
QObject *createCMakeUsagesTest();
#endif

} // CMakeProjectManager::Internal
