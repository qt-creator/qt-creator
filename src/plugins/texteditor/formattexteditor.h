// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include "command.h"

#include <utils/filepath.h>

#include <QPlainTextEdit>
#include <QPointer>

namespace TextEditor {

class TextEditorWidget;

class TEXTEDITOR_EXPORT FormatTask
{
public:
    FormatTask(QPlainTextEdit *_editor, const Utils::FilePath &_filePath, const QString &_sourceData,
               const Command &_command, int _startPos = -1, int _endPos = 0) :
        editor(_editor),
        filePath(_filePath),
        sourceData(_sourceData),
        command(_command),
        startPos(_startPos),
        endPos(_endPos)
    {}

    QPointer<QPlainTextEdit> editor;
    Utils::FilePath filePath;
    QString sourceData;
    TextEditor::Command command;
    int startPos = -1;
    int endPos = 0;
    QString formattedData;
    QString error;
};

TEXTEDITOR_EXPORT void formatCurrentFile(const TextEditor::Command &command, int startPos = -1, int endPos = 0);
TEXTEDITOR_EXPORT void formatEditor(TextEditorWidget *editor, const TextEditor::Command &command,
                  int startPos = -1, int endPos = 0);
TEXTEDITOR_EXPORT void formatEditorAsync(TextEditorWidget *editor, const TextEditor::Command &command,
                       int startPos = -1, int endPos = 0);
TEXTEDITOR_EXPORT void updateEditorText(QPlainTextEdit *editor, const QString &text);

} // namespace TextEditor
