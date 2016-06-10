/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "textdocumentmanipulatorinterface.h"

namespace TextEditor {

class TextEditorWidget;

class TextDocumentManipulator final : public TextDocumentManipulatorInterface
{
public:
    TextDocumentManipulator(TextEditorWidget *textEditorWidget);

    int currentPosition() const final;
    int positionAt(TextPositionOperation textPositionOperation) const final;
    QChar characterAt(int position) const final;
    QString textAt(int position, int length) const final;
    QTextCursor textCursorAt(int position) const final;

    void setCursorPosition(int position) final;
    void setAutoCompleteSkipPosition(int position) final;
    bool replace(int position, int length, const QString &text) final;
    void insertCodeSnippet(int position, const QString &text) final;
    void paste() final;
    void encourageApply() final;
    void autoIndent(int position, int length);

private:
    bool textIsDifferentAt(int position, int length, const QString &text) const;
    void replaceWithoutCheck(int position, int length, const QString &text);

private:
    TextEditorWidget *m_textEditorWidget;
};

} // namespace TextEditor
