/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#include "glslhighlighter.h"
#include <glsl/glsllexer.h>
#include <glsl/glslparser.h>

#include <QtCore/QDebug>

using namespace GLSLEditor;
using namespace GLSLEditor::Internal;

Highlighter::Highlighter(QTextDocument *parent)
    : TextEditor::SyntaxHighlighter(parent)
{
}

Highlighter::~Highlighter()
{

}

void Highlighter::setFormats(const QVector<QTextCharFormat> &formats)
{
    qCopy(formats.begin(), formats.end(), m_formats);
}

void Highlighter::highlightBlock(const QString &text)
{
    const QByteArray data = text.toLatin1();
    GLSL::Lexer lex(/*engine=*/ 0, data.constData(), data.size());
    lex.setState(qMax(0, previousBlockState()));
    lex.setScanKeywords(false);
    lex.setScanComments(true);
    const int variant = GLSL::Lexer::Variant_GLSL_Qt | // ### FIXME: hardcoded
                        GLSL::Lexer::Variant_VertexShader |
                        GLSL::Lexer::Variant_FragmentShader;
    lex.setVariant(variant);
    GLSL::Token tk;
    do {
        lex.yylex(&tk);

        if (tk.is(GLSL::Parser::T_NUMBER))
            setFormat(tk.position, tk.length, m_formats[GLSLNumberFormat]);
        else if (tk.is(GLSL::Parser::T_COMMENT))
            setFormat(tk.position, tk.length, Qt::darkGreen); // ### FIXME: m_formats[GLSLCommentFormat]);
        else if (tk.is(GLSL::Parser::T_IDENTIFIER)) {
            int kind = lex.findKeyword(data.constData() + tk.position, tk.length);
            if (kind == GLSL::Parser::T_RESERVED)
                setFormat(tk.position, tk.length, m_formats[GLSLReservedKeyword]);
            else if (kind != GLSL::Parser::T_IDENTIFIER)
                setFormat(tk.position, tk.length, m_formats[GLSLKeywordFormat]);
        }
    } while (tk.isNot(GLSL::Parser::EOF_SYMBOL));
    setCurrentBlockState(lex.state());
}
