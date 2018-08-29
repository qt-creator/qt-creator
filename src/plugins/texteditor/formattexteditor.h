/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "texteditor_global.h"

#include "command.h"

#include <QPlainTextEdit>
#include <QPointer>

namespace TextEditor {

class TextEditorWidget;

class TEXTEDITOR_EXPORT FormatTask
{
public:
    FormatTask(QPlainTextEdit *_editor, const QString &_filePath, const QString &_sourceData,
               const Command &_command, int _startPos = -1, int _endPos = 0) :
        editor(_editor),
        filePath(_filePath),
        sourceData(_sourceData),
        command(_command),
        startPos(_startPos),
        endPos(_endPos) {}

    QPointer<QPlainTextEdit> editor;
    QString filePath;
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

} // namespace TextEditor
