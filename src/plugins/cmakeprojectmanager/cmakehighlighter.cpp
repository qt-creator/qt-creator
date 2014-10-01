/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cmakehighlighter.h"

#include <QRegExp>
#include <QTextDocument>

using namespace CMakeProjectManager::Internal;


static bool isVariable(const QByteArray &word)
{
    if (word.length() < 4) // must be at least "${.}"
        return false;
    return word.startsWith("${") && word.endsWith('}');
}


CMakeHighlighter::CMakeHighlighter(QTextDocument *document) :
    TextEditor::SyntaxHighlighter(document)
{
    static QVector<TextEditor::TextStyle> categories;
    if (categories.isEmpty()) {
        categories << TextEditor::C_LABEL  // variables
                   << TextEditor::C_KEYWORD   // functions
                   << TextEditor::C_COMMENT
                   << TextEditor::C_STRING
                   << TextEditor::C_VISUAL_WHITESPACE;
    }
    setTextFormatCategories(categories);
}


void CMakeHighlighter::highlightBlock(const QString &text)
{
    QByteArray buf;
    bool inCommentMode = false;
    bool inStringMode = (previousBlockState() == 1);

    QTextCharFormat emptyFormat;
    int i=0;
    for (i=0; i < text.length(); i++) {
        char c = text.at(i).toLatin1();
        if (inCommentMode) {
            setFormat(i, 1, formatForCategory(CMakeCommentFormat));
        } else {
            if (c == '#') {
                if (!inStringMode) {
                    inCommentMode = true;
                    setFormat(i, 1, formatForCategory(CMakeCommentFormat));
                    buf.clear();
                } else {
                    buf += c;
                }
            } else if (c == '(') {
                if (!inStringMode) {
                    if (!buf.isEmpty())
                        setFormat(i - buf.length(), buf.length(), formatForCategory(CMakeFunctionFormat));
                    buf.clear();
                } else {
                    buf += c;
                }
            } else if (text.at(i).isSpace()) {
                if (!inStringMode)
                    buf.clear();
                else
                    buf += c;
            } else if (c == '\"') {
                buf += c;
                if (inStringMode) {
                    setFormat(i + 1 - buf.length(), buf.length(), formatForCategory(CMakeStringFormat));
                    buf.clear();
                } else {
                    setFormat(i, 1, formatForCategory(CMakeStringFormat));
                }
                inStringMode = !inStringMode;
            } else if (c == '\\') {
                setFormat(i, 1, emptyFormat);
                buf += c;
                i++;
                if (i < text.length()) {
                    setFormat(i, 1, emptyFormat);
                    buf += c;
                }
            } else if (c == '$') {
                if (inStringMode)
                    setFormat(i - buf.length(), buf.length(), formatForCategory(CMakeStringFormat));
                buf.clear();
                buf += c;
                setFormat(i, 1, emptyFormat);
            } else if (c == '}') {
                buf += c;
                if (isVariable(buf)) {
                    setFormat(i + 1 - buf.length(), buf.length(), formatForCategory(CMakeVariableFormat));
                    buf.clear();
                }
            } else {
                buf += c;
                setFormat(i, 1, emptyFormat);
            }
        }
    }

    if (inStringMode) {
        setFormat(i - buf.length(), buf.length(), formatForCategory(CMakeStringFormat));
        setCurrentBlockState(1);
    } else {
        setCurrentBlockState(0);
    }

    applyFormatToSpaces(text, formatForCategory(CMakeVisualWhiteSpaceFormat));
}

