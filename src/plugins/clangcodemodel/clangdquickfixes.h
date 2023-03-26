// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/cppquickfix.h>
#include <languageclient/languageclientquickfix.h>

namespace ClangCodeModel {
namespace Internal {
class ClangdClient;

class ClangdQuickFixFactory : public CppEditor::CppQuickFixFactory
{
public:
    ClangdQuickFixFactory();

    void match(const CppEditor::Internal::CppQuickFixInterface &interface,
               QuickFixOperations &result) override;
};

class ClangdQuickFixProvider : public LanguageClient::LanguageClientQuickFixProvider
{
public:
    ClangdQuickFixProvider(ClangdClient *client);

private:
    TextEditor::IAssistProcessor *createProcessor(
            const TextEditor::AssistInterface *) const override;
};

} // namespace Internal
} // namespace ClangCodeModel
