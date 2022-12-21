// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <languageserverprotocol/completion.h>
#include <texteditor/codeassist/assistproposaliteminterface.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>

#include <QPointer>
#include <QScopedPointer>

#include <functional>
#include <optional>

namespace TextEditor {
class IAssistProposal;
class TextDocumentManipulatorInterface;
}

namespace LanguageClient {

class Client;

class LANGUAGECLIENT_EXPORT LanguageClientCompletionAssistProvider
    : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    LanguageClientCompletionAssistProvider(Client *client);

    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const override;

    int activationCharSequenceLength() const override;
    bool isActivationCharSequence(const QString &sequence) const override;
    bool isContinuationChar(const QChar &) const override { return true; }

    void setTriggerCharacters(const std::optional<QList<QString>> triggerChars);

    void setSnippetsGroup(const QString &group) { m_snippetsGroup = group; }

protected:
    Client *client() const { return m_client; }

private:
    QList<QString> m_triggerChars;
    QString m_snippetsGroup;
    int m_activationCharSequenceLength = 0;
    Client *m_client = nullptr; // not owned
};

class LANGUAGECLIENT_EXPORT LanguageClientCompletionAssistProcessor
    : public TextEditor::IAssistProcessor
{
public:
    LanguageClientCompletionAssistProcessor(Client *client,
                                            const TextEditor::IAssistProvider *provider,
                                            const QString &snippetsGroup);
    ~LanguageClientCompletionAssistProcessor() override;
    TextEditor::IAssistProposal *perform() override;
    bool running() override;
    bool needsRestart() const override { return true; }
    void cancel() override;

protected:
    QTextDocument *document() const;
    Utils::FilePath filePath() const { return m_filePath; }
    int basePos() const { return m_basePos; }
    virtual QList<TextEditor::AssistProposalItemInterface *> generateCompletionItems(
        const QList<LanguageServerProtocol::CompletionItem> &items) const;

private:
    void handleCompletionResponse(const LanguageServerProtocol::CompletionRequest::Response &response);

    Utils::FilePath m_filePath;
    QPointer<Client> m_client;
    QPointer<const TextEditor::IAssistProvider> m_provider;
    std::optional<LanguageServerProtocol::MessageId> m_currentRequest;
    QMetaObject::Connection m_postponedUpdateConnection;
    const QString m_snippetsGroup;
    int m_pos = -1;
    int m_basePos = -1;
};

class LANGUAGECLIENT_EXPORT LanguageClientCompletionItem
    : public TextEditor::AssistProposalItemInterface
{
public:
    LanguageClientCompletionItem(LanguageServerProtocol::CompletionItem item);

    // AssistProposalItemInterface interface
    QString text() const override;
    QString filterText() const override;
    bool implicitlyApplies() const override;
    bool prematurelyApplies(const QChar &typedCharacter) const override;
    void apply(TextEditor::TextDocumentManipulatorInterface &manipulator,
               int basePosition) const override;
    QIcon icon() const override;
    QString detail() const override;
    bool isSnippet() const override;
    bool isValid() const override;
    quint64 hash() const override;

    LanguageServerProtocol::CompletionItem item() const;
    QChar triggeredCommitCharacter() const;

    const QString &sortText() const;
    bool hasSortText() const;

    bool operator <(const LanguageClientCompletionItem &other) const;

    bool isPerfectMatch(int pos, QTextDocument *doc) const;
    bool isDeprecated() const;

private:
    LanguageServerProtocol::CompletionItem m_item;
    mutable QChar m_triggeredCommitCharacter;
    mutable QString m_sortText;
    mutable QString m_filterText;
};

} // namespace LanguageClient
