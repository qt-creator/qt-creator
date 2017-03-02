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

#include "javacompletionassistprovider.h"
#include "androidconstants.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/keywordscompletionassist.h>
#include <coreplugin/id.h>

using namespace Android;
using namespace Android::Internal;

static const char *const keywords[] = {
    "abstract",
    "continue",
    "for",
    "new",
    "switch",
    "assert",
    "default",
    "goto",
    "package",
    "synchronized",
    "boolean",
    "do",
    "if",
    "private",
    "this",
    "break",
    "double",
    "implements",
    "protected",
    "throw",
    "byte",
    "else",
    "import",
    "public",
    "throws",
    "case",
    "enum",
    "instanceof",
    "return",
    "transient",
    "catch",
    "extends",
    "int",
    "short",
    "try",
    "char",
    "final",
    "interface",
    "static",
    "void",
    "class",
    "finally",
    "long",
    "strictfp",
    "volatile",
    "const",
    "float",
    "native",
    "super",
    "while",
    0
};

JavaCompletionAssistProvider::JavaCompletionAssistProvider()
{
}

JavaCompletionAssistProvider::~JavaCompletionAssistProvider()
{

}

void JavaCompletionAssistProvider::init() const
{
    for (uint i = 0; i < sizeof keywords / sizeof keywords[0] - 1; i++)
        m_keywords.append(QLatin1String(keywords[i]));
}

bool JavaCompletionAssistProvider::supportsEditor(Core::Id editorId) const
{
    return editorId == Constants::JAVA_EDITOR_ID;
}

TextEditor::IAssistProcessor *JavaCompletionAssistProvider::createProcessor() const
{
    if (m_keywords.isEmpty())
        init();
    TextEditor::Keywords keywords = TextEditor::Keywords(m_keywords,
                                                         QStringList(),
                                                         QMap<QString, QStringList>());
    return new TextEditor::KeywordsCompletionAssistProcessor(keywords);
}
