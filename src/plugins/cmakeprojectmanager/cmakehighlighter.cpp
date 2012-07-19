/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "cmakehighlighter.h"

#include <QRegExp>
#include <QColor>
#include <QTextDocument>
#include <QTextEdit>

using namespace CMakeProjectManager::Internal;


static bool isVariable(const QString &word)
{
    if (word.length() < 4) // must be at least "${.}"
        return false;
    return word.startsWith("${") && word.endsWith('}');
}


CMakeHighlighter::CMakeHighlighter(QTextDocument *document) :
    TextEditor::SyntaxHighlighter(document)
{
}


void CMakeHighlighter::highlightBlock(const QString &text)
{
    QString buf;
    bool inCommentMode = false;
    bool inStringMode = (previousBlockState() == 1);

    QTextCharFormat emptyFormat;
    int i=0;
    for (i=0; i < text.length(); i++) {
        const QChar c = text.at(i);
        if (inCommentMode) {
            setFormat(i, 1, m_formats[CMakeCommentFormat]);
        } else {
            if (c == '#') {
                if (!inStringMode) {
                    inCommentMode = true;
                    setFormat(i, 1, m_formats[CMakeCommentFormat]);
                    buf.clear();
                } else {
                    buf += c;
                }
            } else if (c == '(') {
                if (!inStringMode) {
                    if (!buf.isEmpty())
                        setFormat(i - buf.length(), buf.length(), m_formats[CMakeFunctionFormat]);
                    buf.clear();
                } else {
                    buf += c;
                }
            } else if (c.isSpace()) {
                if (!inStringMode)
                    buf.clear();
                else
                    buf += c;
            } else if (c == '\"') {
                buf += c;
                if (inStringMode) {
                    setFormat(i + 1 - buf.length(), buf.length(), m_formats[CMakeStringFormat]);
                    buf.clear();
                } else {
                    setFormat(i, 1, m_formats[CMakeStringFormat]);
                }
                inStringMode = !inStringMode;
            }
            else if (c == '\\') {
                setFormat(i, 1, emptyFormat);
                buf += c;
                i++;
                if (i < text.length()) {
                    text.at(i);
                    setFormat(i, 1, emptyFormat);
                    buf += c;
                }
            } else if (c == '$') {
                if (inStringMode)
                    setFormat(i - buf.length(), buf.length(), m_formats[CMakeStringFormat]);
                buf.clear();
                buf += c;
                setFormat(i, 1, emptyFormat);
            } else if (c == '}') {
                buf += c;
                if (isVariable(buf)) {
                    setFormat(i + 1 - buf.length(), buf.length(), m_formats[CMakeVariableFormat]);
                    buf.clear();
                }
            } else {
                buf += c;
                setFormat(i, 1, emptyFormat);
            }
        }
    }

    if (inStringMode) {
        setFormat(i - buf.length(), buf.length(), m_formats[CMakeStringFormat]);
        setCurrentBlockState(1);
    } else {
        setCurrentBlockState(0);
    }

    applyFormatToSpaces(text, m_formats[CMakeVisualWhiteSpaceFormat]);
}

