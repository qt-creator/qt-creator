/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <utils/qtcassert.h>

#include "githighlighters.h"

namespace Git {
namespace Internal {

// Retrieve the comment char format from the text editor.
static QTextCharFormat commentFormat()
{
    const TextEditor::FontSettings settings = TextEditor::TextEditorSettings::instance()->fontSettings();
    return settings.toTextCharFormat(TextEditor::C_COMMENT);
}

GitSubmitHighlighter::GitSubmitHighlighter(QTextEdit * parent) :
    TextEditor::SyntaxHighlighter(parent)
{
    initialize();
}

GitSubmitHighlighter::GitSubmitHighlighter(TextEditor::BaseTextDocument *parent) :
    TextEditor::SyntaxHighlighter(parent)
{
    initialize();
}

void GitSubmitHighlighter::initialize()
{
    m_commentFormat = commentFormat();
    m_keywordPattern.setPattern(QLatin1String("^[\\w-]+:"));
    m_hashChar = QLatin1Char('#');
    QTC_CHECK(m_keywordPattern.isValid());
}

void GitSubmitHighlighter::highlightBlock(const QString &text)
{
    // figure out current state
    State state = static_cast<State>(previousBlockState());
    if (text.isEmpty()) {
        if (state == Header)
            state = Other;
        setCurrentBlockState(state);
        return;
    } else if (text.startsWith(m_hashChar)) {
        setFormat(0, text.size(), m_commentFormat);
        setCurrentBlockState(state);
        return;
    } else if (state == None) {
        state = Header;
    }

    setCurrentBlockState(state);
    // Apply format.
    switch (state) {
    case None:
        break;
    case Header: {
        QTextCharFormat charFormat = format(0);
        charFormat.setFontWeight(QFont::Bold);
        setFormat(0, text.size(), charFormat);
        break;
    }
    case Other:
        // Format key words ("Task:") italic
        if (m_keywordPattern.indexIn(text, 0, QRegExp::CaretAtZero) == 0) {
            QTextCharFormat charFormat = format(0);
            charFormat.setFontItalic(true);
            setFormat(0, m_keywordPattern.matchedLength(), charFormat);
        }
        break;
    }
}

} // namespace Internal
} // namespace Git
