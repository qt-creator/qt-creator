/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "clangqueryhoverhandler.h"

#include "clangqueryhighlighter.h"

#include <dynamicastmatcherdiagnosticmessagecontainer.h>

#include <texteditor/texteditor.h>

namespace ClangRefactoring {

ClangQueryHoverHandler::ClangQueryHoverHandler(ClangQueryHighlighter *highligher)
    : m_highligher(highligher)
{
}

void ClangQueryHoverHandler::identifyMatch(TextEditor::TextEditorWidget *editorWidget, int position)
{
    using Messages = ClangBackEnd::DynamicASTMatcherDiagnosticMessageContainers;
    using Contexts = ClangBackEnd::DynamicASTMatcherDiagnosticContextContainers;

    QTextCursor textCursor = editorWidget->textCursor();
    textCursor.setPosition(position);
    int line = textCursor.blockNumber() + 1;
    int column = textCursor.columnNumber() + 1;

    Messages messages = m_highligher->messagesForLineAndColumn(uint(line), uint(column));
    Contexts contexts = m_highligher->contextsForLineAndColumn(uint(line), uint(column));

    if (!messages.empty())
        setToolTip(QString("%1: %2").arg(QString(messages[0].errorTypeText())).arg(QString(messages[0].arguments().join(", "))));
    else if (!contexts.empty())
        setToolTip(QString("%1: %2").arg(QString(contexts[0].contextTypeText())).arg(QString(contexts[0].arguments().join(", "))));
}

} // namespace ClangRefactoring
