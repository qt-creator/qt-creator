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

#include "profilehighlighter.h"
#include "profilecompletionassist.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QTextDocument>

using namespace TextEditor;

namespace QmakeProjectManager {
namespace Internal {

static TextStyle styleForFormat(int format)
{
    const auto f = ProFileHighlighter::ProfileFormats(format);
    switch (f) {
    case ProFileHighlighter::ProfileVariableFormat: return C_TYPE;
    case ProFileHighlighter::ProfileFunctionFormat: return C_KEYWORD;
    case ProFileHighlighter::ProfileCommentFormat: return C_COMMENT;
    case ProFileHighlighter::ProfileVisualWhitespaceFormat: return C_VISUAL_WHITESPACE;
    case ProFileHighlighter::NumProfileFormats:
        QTC_CHECK(false); // should never get here
        return C_TEXT;
    }
    QTC_CHECK(false); // should never get here
    return C_TEXT;
}

ProFileHighlighter::ProFileHighlighter()
    : m_keywords(qmakeKeywords())
{
    setTextFormatCategories(NumProfileFormats, styleForFormat);
}

void ProFileHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty())
        return;

    QString buf;
    bool inCommentMode = false;

    QTextCharFormat emptyFormat;
    int i = 0;
    for (;;) {
        const QChar c = text.at(i);
        if (inCommentMode) {
            setFormat(i, 1, formatForCategory(ProfileCommentFormat));
        } else {
            if (c.isLetter() || c == QLatin1Char('_') || c == QLatin1Char('.') || c.isDigit()) {
                buf += c;
                setFormat(i - buf.length()+1, buf.length(), emptyFormat);
                if (!buf.isEmpty() && m_keywords.isFunction(buf))
                    setFormat(i - buf.length()+1, buf.length(), formatForCategory(ProfileFunctionFormat));
                else if (!buf.isEmpty() && m_keywords.isVariable(buf))
                    setFormat(i - buf.length()+1, buf.length(), formatForCategory(ProfileVariableFormat));
            } else if (c == QLatin1Char('(')) {
                if (!buf.isEmpty() && m_keywords.isFunction(buf))
                    setFormat(i - buf.length(), buf.length(), formatForCategory(ProfileFunctionFormat));
                buf.clear();
            } else if (c == QLatin1Char('#')) {
                inCommentMode = true;
                setFormat(i, 1, formatForCategory(ProfileCommentFormat));
                buf.clear();
            } else {
                if (!buf.isEmpty() && m_keywords.isVariable(buf))
                    setFormat(i - buf.length(), buf.length(), formatForCategory(ProfileVariableFormat));
                buf.clear();
            }
        }
        i++;
        if (i >= text.length())
            break;
    }

    formatSpaces(text);
}

} // namespace Internal
} // namespace QmakeProjectManager
