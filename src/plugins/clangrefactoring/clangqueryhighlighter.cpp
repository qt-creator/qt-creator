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

#include "clangqueryhighlighter.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <QTextBlock>

namespace ClangRefactoring {

ClangQueryHighlighter::ClangQueryHighlighter()
    : m_marker(*this)
{
#ifndef UNIT_TESTS
    TextEditor::FontSettings fontSettings =  TextEditor::TextEditorSettings::fontSettings();

    m_marker.setTextFormats(fontSettings.toTextCharFormat(TextEditor::C_ERROR),
                            fontSettings.toTextCharFormat(TextEditor::C_ERROR_CONTEXT));
#endif

    setNoAutomaticHighlighting(true);
}

void ClangQueryHighlighter::setDiagnostics(
        const ClangBackEnd::DynamicASTMatcherDiagnosticContainers &diagnostics)
{
    using Messages = ClangBackEnd::DynamicASTMatcherDiagnosticMessageContainers;
    using Contexts = ClangBackEnd::DynamicASTMatcherDiagnosticContextContainers;

    Messages messages;
    Contexts contexts;

    for (const ClangBackEnd::DynamicASTMatcherDiagnosticContainer &diagnostic : diagnostics) {
        Messages newMessages = diagnostic.messages();
        Contexts newContexts = diagnostic.contexts();
        std::copy(newMessages.begin(), newMessages.end(), std::back_inserter(messages));
        std::copy(newContexts.begin(), newContexts.end(), std::back_inserter(contexts));
    }

    m_marker.setMessagesAndContexts(std::move(messages), std::move(contexts));

    rehighlight();
}

bool ClangQueryHighlighter::hasDiagnostics() const
{
    return m_marker.hasMessagesOrContexts();
}

ClangBackEnd::DynamicASTMatcherDiagnosticMessageContainers
ClangQueryHighlighter::messagesForLineAndColumn(uint line, uint column) const
{
    return m_marker.messagesForLineAndColumn(line, column);
}

ClangBackEnd::DynamicASTMatcherDiagnosticContextContainers
ClangQueryHighlighter::contextsForLineAndColumn(uint line, uint column) const
{
    return m_marker.contextsForLineAndColumn(line, column);
}

void ClangQueryHighlighter::highlightBlock(const QString &text)
{
    int currentLineNumber = currentBlock().blockNumber() + 1;
    m_marker.highlightBlock(uint(currentLineNumber), text);
}

} // namespace ClangRefactoring
