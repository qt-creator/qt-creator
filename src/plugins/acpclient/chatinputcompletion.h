// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/codeassist/completionassistprovider.h>

#include <QList>
#include <QString>

namespace AcpClient::Internal {

struct CommandInfo {
    QString name;
    QString description;
    QString hint; // input hint; empty if the command takes no input
};

class ChatInputCompletionProvider final : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    explicit ChatInputCompletionProvider(QObject *parent = nullptr);

    void setAvailableCommands(const QList<CommandInfo> &commands);

    TextEditor::IAssistProcessor *createProcessor(
        const TextEditor::AssistInterface *interface) const override;

    int activationCharSequenceLength() const override;
    bool isActivationCharSequence(const QString &sequence) const override;

private:
    TextEditor::IAssistProcessor *createCommandProcessor(
        const TextEditor::AssistInterface *interface) const;

    QList<CommandInfo> m_commands;
};

} // namespace AcpClient::Internal
