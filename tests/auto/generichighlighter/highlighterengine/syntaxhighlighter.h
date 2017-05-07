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

// Replaces the "real" syntaxhighlighter.h file. The scope of this test is restricted to the
// highlight definition's context engine. Using a mock derived from QSyntaxHighlighter as a
// base instead of the real TextEditor::SyntaxHighlighter should not affect it.

#include <QSyntaxHighlighter>
#include <texteditor/texteditorconstants.h>

#include <functional>

namespace TextEditor {

class SyntaxHighlighter : public QSyntaxHighlighter
{
public:
    SyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent) {}
    virtual ~SyntaxHighlighter() {}

protected:
    void formatSpaces(const QString &)
    {}
    void setTextFormatCategories(int, std::function<TextStyle(int)>)
    {}
    QTextCharFormat formatForCategory(int categoryIndex) const;

};

}
