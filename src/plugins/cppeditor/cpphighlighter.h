// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <texteditor/syntaxhighlighter.h>

#include <cplusplus/Token.h>

namespace ExtensionSystem { class IPlugin; }

namespace CppEditor {

class CPPEDITOR_EXPORT CppHighlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT

public:
    CppHighlighter(QTextDocument *document = nullptr);

    void setLanguageFeaturesFlags(unsigned int flags) override;
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

namespace Internal {
void registerHighlighterTests(ExtensionSystem::IPlugin &plugin);
} // namespace Internal

} // namespace CppEditor
