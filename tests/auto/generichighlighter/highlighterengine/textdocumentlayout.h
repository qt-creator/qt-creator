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

#include <QTextBlockUserData>

// Replaces the "real" textdocumentlayout.h file.
namespace TextEditor {
struct Parenthesis;
typedef QVector<Parenthesis> Parentheses;

struct Parenthesis
{
    enum Type { Opened, Closed };

    inline Parenthesis() : type(Opened), pos(-1) {}
    inline Parenthesis(Type t, QChar c, int position)
        : type(t), chr(c), pos(position) {}
    Type type;
    QChar chr;
    int pos;
};

struct CodeFormatterData {};

struct TextBlockUserData : QTextBlockUserData
{
    TextBlockUserData() : m_data(0) {}
    virtual ~TextBlockUserData() {}

    void setFoldingStartIncluded(const bool) {}
    void setFoldingEndIncluded(const bool) {}
    void setFoldingIndent(const int) {}
    void setCodeFormatterData(CodeFormatterData *data) { m_data = data; }
    CodeFormatterData *codeFormatterData() { return m_data; }
    CodeFormatterData *m_data;
};

namespace TextDocumentLayout {
TextBlockUserData *userData(const QTextBlock &block);
void setParentheses(const QTextBlock &, const Parentheses &);
}

} // namespace TextEditor
