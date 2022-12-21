// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/codeassist/assistinterface.h>

#include <cplusplus/Token.h>

namespace ClangCodeModel {
namespace Internal {

enum class CompletionType { FunctionHint, Other };

class ClangCompletionAssistInterface: public TextEditor::AssistInterface
{
public:
    ClangCompletionAssistInterface(const QByteArray &text,
                                   int position)
        : TextEditor::AssistInterface(text, position),
          languageFeatures_(CPlusPlus::LanguageFeatures::defaultFeatures())
    {}

    CompletionType type() const { return CompletionType::Other; }
    CPlusPlus::LanguageFeatures languageFeatures() const { return languageFeatures_; }

private:
    CPlusPlus::LanguageFeatures languageFeatures_;
};

} // namespace Internal
} // namespace ClangCodeModel
