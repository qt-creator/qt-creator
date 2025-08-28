// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include "command.h"

#include <utils/filepath.h>

namespace Utils { class PlainTextEdit; }

namespace TextEditor {

class TextEditorWidget;

TEXTEDITOR_EXPORT void formatCurrentFile(const TextEditor::Command &command, int startPos = -1, int endPos = 0);
TEXTEDITOR_EXPORT void formatEditor(TextEditorWidget *editor, const TextEditor::Command &command,
                  int startPos = -1, int endPos = 0);
TEXTEDITOR_EXPORT void formatEditorAsync(TextEditorWidget *editor, const TextEditor::Command &command,
                       int startPos = -1, int endPos = 0);
TEXTEDITOR_EXPORT void updateEditorText(Utils::PlainTextEdit *editor, const QString &text);

} // namespace TextEditor
