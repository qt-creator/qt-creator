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
#ifndef GLSLHIGHLIGHTER_H
#define GLSLHIGHLIGHTER_H

#include <texteditor/syntaxhighlighter.h>

namespace GLSLEditor {
class GLSLTextEditorWidget;

namespace Internal {

class Highlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT

public:
    enum Formats {
        GLSLNumberFormat,
        GLSLStringFormat,
        GLSLTypeFormat,
        GLSLKeywordFormat,
        GLSLOperatorFormat,
        GLSLPreprocessorFormat,
        GLSLLabelFormat,
        GLSLCommentFormat,
        GLSLDoxygenCommentFormat,
        GLSLDoxygenTagFormat,
        GLSLVisualWhitespace,
        GLSLReservedKeyword,
        NumGLSLFormats
    };

    explicit Highlighter(TextEditor::BaseTextDocument *parent);
    virtual ~Highlighter();

    void setFormats(const QVector<QTextCharFormat> &formats);

protected:
    void highlightBlock(const QString &text);
    void highlightLine(const QString &text, int position, int length, const QTextCharFormat &format);
    bool isPPKeyword(const QStringRef &text) const;

private:
    QTextCharFormat m_formats[NumGLSLFormats];
};

} // namespace Internal
} // namespace GLSLEditor

#endif // GLSLHIGHLIGHTER_H
