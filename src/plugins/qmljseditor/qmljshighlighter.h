// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljseditor_global.h"

#include <qmljs/qmljsscanner.h>

#include <texteditor/textdocumentlayout.h>
#include <texteditor/syntaxhighlighter.h>

namespace QmlJSEditor {

class QMLJSEDITOR_EXPORT QmlJSHighlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT

public:
    QmlJSHighlighter(QTextDocument *parent = nullptr);
    ~QmlJSHighlighter() override;

    bool isQmlEnabled() const;
    void setQmlEnabled(bool duiEnabled);

protected:
    void highlightBlock(const QString &text) override;

    int onBlockStart();
    void onBlockEnd(int state);

    // The functions are notified whenever parentheses are encountered.
    // Custom behaviour can be added, for example storing info for indenting.
    void onOpeningParenthesis(QChar parenthesis, int pos, bool atStart);
    void onClosingParenthesis(QChar parenthesis, int pos, bool atEnd);

    bool maybeQmlKeyword(QStringView text) const;
    bool maybeQmlBuiltinType(QStringView text) const;

private:
    bool m_qmlEnabled;
    int m_braceDepth;
    int m_foldingIndent;
    bool m_inMultilineComment;

    QmlJS::Scanner m_scanner;
    TextEditor::Parentheses m_currentBlockParentheses;
};

} // namespace QmlJSEditor
