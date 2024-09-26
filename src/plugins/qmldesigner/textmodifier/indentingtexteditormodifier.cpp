// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "indentingtexteditormodifier.h"

#include <qmljstools/qmljscodestylepreferences.h>
#include <qmljstools/qmljsindenter.h>
#include <qmljstools/qmljstoolssettings.h>

namespace QmlDesigner {

IndentingTextEditModifier::IndentingTextEditModifier(QTextDocument *document,
                                                     const QTextCursor &textCursor)
    : NotIndentingTextEditModifier{document, textCursor}
{
    m_tabSettings = QmlJSTools::QmlJSToolsSettings::globalCodeStyle()->tabSettings();
}

void IndentingTextEditModifier::indent(int offset, int length)
{
    if (length == 0 || offset < 0 || offset + length >= text().length())
        return;

    int startLine = getLineInDocument(textDocument(), offset);
    int endLine = getLineInDocument(textDocument(), offset + length);

    if (startLine > -1 && endLine > -1)
        indentLines(startLine, endLine);
}

void IndentingTextEditModifier::indentLines(int startLine, int endLine)
{
    QmlJSEditor::indentQmlJs(textDocument(), startLine, endLine, m_tabSettings);
}

} // namespace QmlDesigner
