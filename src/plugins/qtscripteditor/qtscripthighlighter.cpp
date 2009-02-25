/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "qtscripthighlighter.h"

#include <utils/qtcassert.h>

namespace QtScriptEditor {
namespace Internal {

QtScriptHighlighter::QtScriptHighlighter(QTextDocument *parent) :
     SharedTools::QScriptHighlighter(parent)
{
    m_currentBlockParentheses.reserve(20);
    m_braceDepth = 0;
}

int QtScriptHighlighter::onBlockStart()
{
    m_currentBlockParentheses.clear();
    m_braceDepth = 0;

    int state = 0;
    int previousState = previousBlockState();
    if (previousState != -1) {
        state = previousState & 0xff;
        m_braceDepth = previousState >> 8;
    }

    return state;
}

void QtScriptHighlighter::onOpeningParenthesis(QChar parenthesis, int pos)
{
    if (parenthesis == QLatin1Char('{'))
        ++m_braceDepth;
     m_currentBlockParentheses.push_back(Parenthesis(Parenthesis::Opened, parenthesis, pos));
}

void QtScriptHighlighter::onClosingParenthesis(QChar parenthesis, int pos)
{
    if (parenthesis == QLatin1Char('}'))
        --m_braceDepth;
    m_currentBlockParentheses.push_back(Parenthesis(Parenthesis::Closed, parenthesis, pos));
}

void QtScriptHighlighter::onBlockEnd(int state, int firstNonSpace)
{
    typedef TextEditor::TextBlockUserData TextEditorBlockData;

    setCurrentBlockState((m_braceDepth << 8) | state);

    // Set block data parentheses. Force creation of block data unless empty
    TextEditorBlockData *blockData = 0;

    if (QTextBlockUserData *userData = currentBlockUserData())
        blockData = static_cast<TextEditorBlockData *>(userData);

    if (!blockData && !m_currentBlockParentheses.empty()) {
        blockData = new TextEditorBlockData;
        setCurrentBlockUserData(blockData);
    }
    if (blockData) {
        blockData->setParentheses(m_currentBlockParentheses);
        blockData->setClosingCollapseMode(TextEditor::TextBlockUserData::NoClosingCollapse);
        blockData->setCollapseMode(TextEditor::TextBlockUserData::NoCollapse);
    }
    if (!m_currentBlockParentheses.isEmpty()) {
        QTC_ASSERT(blockData, return);
        int collapse = Parenthesis::collapseAtPos(m_currentBlockParentheses);
        if (collapse >= 0) {
            if (collapse == firstNonSpace)
                blockData->setCollapseMode(TextEditor::TextBlockUserData::CollapseThis);
            else
                blockData->setCollapseMode(TextEditor::TextBlockUserData::CollapseAfter);
        }
        if (Parenthesis::hasClosingCollapse(m_currentBlockParentheses))
            blockData->setClosingCollapseMode(TextEditor::TextBlockUserData::NoClosingCollapse);
    }
}

} // namespace Internal
} // namespace QtScriptEditor
