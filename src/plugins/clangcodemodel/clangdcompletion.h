
#include <languageclient/languageclientcompletionassist.h>
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <languageclient/languageclientcompletionassist.h>
#include <languageserverprotocol/clientcapabilities.h>

namespace TextEditor { class IAssistProcessor; }
namespace ClangCodeModel::Internal {
class ClangdClient;

class ClangdCompletionAssistProvider : public LanguageClient::LanguageClientCompletionAssistProvider
{
public:
    ClangdCompletionAssistProvider(ClangdClient *client);

private:
    TextEditor::IAssistProcessor *createProcessor(
            const TextEditor::AssistInterface *interface) const override;

    int activationCharSequenceLength() const override { return 3; }
    bool isActivationCharSequence(const QString &sequence) const override;
    bool isContinuationChar(const QChar &c) const override;

    bool isInCommentOrString(const TextEditor::AssistInterface *interface) const;

    ClangdClient * const m_client;
};

class ClangdCompletionCapabilities
        : public LanguageServerProtocol::TextDocumentClientCapabilities::CompletionCapabilities
{
public:
    explicit ClangdCompletionCapabilities(const JsonObject &object);
};

} // namespace ClangCodeModel::Internal
