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
#include "glslcodecompletion.h"
#include "glsleditor.h"
#include <QtCore/QDebug>

using namespace GLSLEditor;

static const char *glsl_keywords[] =
{ // ### TODO: get the keywords from the lexer
  "attribute",
  "bool",
  "break",
  "bvec2",
  "bvec3",
  "bvec4",
  "case",
  "centroid",
  "const",
  "continue",
  "default",
  "discard",
  "dmat2",
  "dmat2x2",
  "dmat2x3",
  "dmat2x4",
  "dmat3",
  "dmat3x2",
  "dmat3x3",
  "dmat3x4",
  "dmat4",
  "dmat4x2",
  "dmat4x3",
  "dmat4x4",
  "do",
  "double",
  "dvec2",
  "dvec3",
  "dvec4",
  "else",
  "false",
  "flat",
  "float",
  "for",
  "highp",
  "if",
  "in",
  "inout",
  "int",
  "invariant",
  "isampler1D",
  "isampler1DArray",
  "isampler2D",
  "isampler2DArray",
  "isampler2DMS",
  "isampler2DMSArray",
  "isampler2DRect",
  "isampler3D",
  "isamplerBuffer",
  "isamplerCube",
  "isamplerCubeArray",
  "ivec2",
  "ivec3",
  "ivec4",
  "layout",
  "lowp",
  "mat2",
  "mat2x2",
  "mat2x3",
  "mat2x4",
  "mat3",
  "mat3x2",
  "mat3x3",
  "mat3x4",
  "mat4",
  "mat4x2",
  "mat4x3",
  "mat4x4",
  "mediump",
  "noperspective",
  "out",
  "patch",
  "precision",
  "return",
  "sample",
  "sampler1D",
  "sampler1DArray",
  "sampler1DArrayShadow",
  "sampler1DShadow",
  "sampler2D",
  "sampler2DArray",
  "sampler2DArrayShadow",
  "sampler2DMS",
  "sampler2DMSArray",
  "sampler2DRect",
  "sampler2DRectShadow",
  "sampler2DShadow",
  "sampler3D",
  "samplerBuffer",
  "samplerCube",
  "samplerCubeArray",
  "samplerCubeArrayShadow",
  "samplerCubeShadow",
  "smooth",
  "struct",
  "subroutine",
  "switch",
  "true",
  "uint",
  "uniform",
  "usampler1D",
  "usampler1DArray",
  "usampler2D",
  "usampler2DArray",
  "usampler2DMS",
  "usampler2DMSarray",
  "usampler2DRect",
  "usampler3D",
  "usamplerBuffer",
  "usamplerCube",
  "usamplerCubeArray",
  "uvec2",
  "uvec3",
  "uvec4",
  "varying",
  "vec2",
  "vec3",
  "vec4",
  "void",
  "while",
  0
};

CodeCompletion::CodeCompletion(QObject *parent)
    : ICompletionCollector(parent),
      m_editor(0),
      m_startPosition(-1),
      m_restartCompletion(false)
{
    for (const char **it = glsl_keywords; *it; ++it) {
        TextEditor::CompletionItem item(this);
        item.text = QString::fromLatin1(*it);
        m_keywordCompletions.append(item);
    }
}

CodeCompletion::~CodeCompletion()
{
}

TextEditor::ITextEditable *CodeCompletion::editor() const
{
    return m_editor;
}

int CodeCompletion::startPosition() const
{
    return m_startPosition;
}

bool CodeCompletion::supportsEditor(TextEditor::ITextEditable *editor)
{
    if (qobject_cast<GLSLTextEditor *>(editor->widget()) != 0)
        return true;

    return false;
}

bool CodeCompletion::triggersCompletion(TextEditor::ITextEditable *editor)
{
    Q_UNUSED(editor);
    return false;
}

int CodeCompletion::startCompletion(TextEditor::ITextEditable *editor)
{
    m_editor = editor;

    int pos = editor->position() - 1;
    QChar ch = editor->characterAt(pos);
    while (ch.isLetterOrNumber())
        ch = editor->characterAt(--pos);

    m_completions += m_keywordCompletions;

    m_startPosition = pos + 1;
    return m_startPosition;
}

void CodeCompletion::completions(QList<TextEditor::CompletionItem> *completions)
{
    const int length = m_editor->position() - m_startPosition;

    if (length == 0)
        *completions = m_completions;
    else if (length > 0) {
        const QString key = m_editor->textAt(m_startPosition, length);

        filter(m_completions, completions, key);

        if (completions->size() == 1) {
            if (key == completions->first().text)
                completions->clear();
        }
    }
}

bool CodeCompletion::typedCharCompletes(const TextEditor::CompletionItem &item, QChar typedChar)
{
    Q_UNUSED(item);
    Q_UNUSED(typedChar);
    return false;
}

void CodeCompletion::complete(const TextEditor::CompletionItem &item, QChar typedChar)
{
    Q_UNUSED(typedChar);

    QString toInsert = item.text;

    const int length = m_editor->position() - m_startPosition;
    m_editor->setCurPos(m_startPosition);
    m_editor->replace(length, toInsert);

    if (toInsert.endsWith(QLatin1Char('.')) || toInsert.endsWith(QLatin1Char('(')))
        m_restartCompletion = true;
}

bool CodeCompletion::partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems)
{
    return ICompletionCollector::partiallyComplete(completionItems);
}

void CodeCompletion::cleanup()
{
    m_editor = 0;
    m_completions.clear();
    m_restartCompletion = false;
    m_startPosition = -1;
}

