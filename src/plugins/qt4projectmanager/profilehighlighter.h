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

#ifndef PROFILEHIGHLIGHTER_H
#define PROFILEHIGHLIGHTER_H

#include <texteditor/syntaxhighlighter.h>
#include <texteditor/codeassist/keywordscompletionassist.h>
#include <QtAlgorithms>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

namespace Qt4ProjectManager {
namespace Internal {

class ProFileHighlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT
public:
    enum ProfileFormats {
        ProfileVariableFormat,
        ProfileFunctionFormat,
        ProfileCommentFormat,
        ProfileVisualWhitespaceFormat,
        NumProfileFormats
    };

    ProFileHighlighter(QTextDocument *document = 0);
    void highlightBlock(const QString &text);

    // Set formats from a sequence of type QTextCharFormat
    void setFormats(const QVector<QTextCharFormat> &formats)
    {
        qCopy(formats.begin(), formats.end(), m_formats);
    }

private:
    QTextCharFormat m_formats[NumProfileFormats];
    TextEditor::Keywords m_keywords;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROFILEHIGHLIGHTER_H
