/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "profilehighlighter.h"
#include "profilecompletionassist.h"

#include <extensionsystem/pluginmanager.h>

#include <QTextDocument>

using namespace TextEditor;

namespace QmakeProjectManager {
namespace Internal {

ProFileHighlighter::ProFileHighlighter(const Keywords &keywords)
    : m_keywords(keywords)
{
    static QVector<TextStyle> categories;
    if (categories.isEmpty())
        categories << C_TYPE << C_KEYWORD << C_COMMENT << C_VISUAL_WHITESPACE;
    setTextFormatCategories(categories);
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

    applyFormatToSpaces(text, formatForCategory(ProfileVisualWhitespaceFormat));
}

} // namespace Internal
} // namespace QmakeProjectManager
