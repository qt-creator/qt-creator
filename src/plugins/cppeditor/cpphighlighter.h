// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <texteditor/syntaxhighlighter.h>

#include <cplusplus/Token.h>

#ifdef WITH_TESTS
#include <QTextDocument>
#endif

namespace CppEditor {

class CPPEDITOR_EXPORT CppHighlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT

public:
    CppHighlighter(QTextDocument *document = nullptr);

    void setLanguageFeatures(const CPlusPlus::LanguageFeatures &languageFeatures);
    void highlightBlock(const QString &text) override;

private:
    void highlightWord(QStringView word, int position, int length);
    bool highlightRawStringLiteral(QStringView text, const CPlusPlus::Token &tk,
                                   const QString &inheritedSuffix);
    void highlightStringLiteral(QStringView text, const CPlusPlus::Token &tk);

    void highlightDoxygenComment(const QString &text, int position,
                                 int length);

    bool isPPKeyword(QStringView text) const;

private:
    CPlusPlus::LanguageFeatures m_languageFeatures = CPlusPlus::LanguageFeatures::defaultFeatures();
};

#ifdef WITH_TESTS
namespace Internal {
class CppHighlighterTest : public CppHighlighter
{
    Q_OBJECT

public:
    CppHighlighterTest();

private slots:
    void test_data();
    void test();
    void testParentheses_data();
    void testParentheses();

private:
    QTextDocument m_doc;
};
} // namespace Internal
#endif // WITH_TESTS

} // namespace CppEditor
