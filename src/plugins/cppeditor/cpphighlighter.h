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

#ifndef CPPHIGHLIGHTER_H
#define CPPHIGHLIGHTER_H

#include "cppeditorenums.h"

#include <texteditor/syntaxhighlighter.h>

#include <QTextCharFormat>

namespace CppEditor {

namespace Internal {

class CPPEditorWidget;

class CppHighlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT

public:
    CppHighlighter(QTextDocument *document = 0);

    virtual void highlightBlock(const QString &text);

    // Set formats from a sequence of type QTextCharFormat
    template <class InputIterator>
        void setFormats(InputIterator begin, InputIterator end) {
            qCopy(begin, end, m_formats);
        }

private:
    void highlightWord(QStringRef word, int position, int length);
    void highlightLine(const QString &line, int position, int length,
                       const QTextCharFormat &format);

    void highlightDoxygenComment(const QString &text, int position,
                                 int length);

    bool isPPKeyword(const QStringRef &text) const;

    QTextCharFormat m_formats[NumCppFormats];
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPHIGHLIGHTER_H
