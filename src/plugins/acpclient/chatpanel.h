// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <acp/acp.h>

#include <QJsonValue>
#include <QMap>
#include <QWidget>

namespace Utils {
class ElidingLabel;
class ProgressIndicator;
class StyledBar;
} // namespace Utils

class QComboBox;
class QHBoxLayout;
class QLabel;
class QTimer;
class QToolButton;

namespace AcpClient::Internal {

class AcpMessageView;
class ChatInputEdit;
class SendButton;

class ChatPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPanel(QWidget *parent = nullptr);

    AcpMessageView *messageView() const { return m_messageView; }
    ChatInputEdit *inputEdit() const { return m_inputEdit; }

    void setAgentInfo(const QString &name, const QString &version);
    void setPrompting(bool prompting);
    void setSendEnabled(bool enabled);
    bool isPrompting() const { return m_prompting; }

    void updateConfigOptions(const QList<Acp::SessionConfigOption> &configOptions);
    void clearConfigOptions();

    void updateAvailableCommands(const QList<Acp::AvailableCommand> &commands);

    // Delegate to message view
    void addUserMessage(const QString &text);
    void appendAgentText(const QString &text);
    void appendAgentThought(const QString &text);
    void addToolCall(const Acp::ToolCall &toolCall);
    void updateToolCall(const Acp::ToolCallUpdate &update);
    void addPlan(const Acp::Plan &plan);
    void addErrorMessage(const QString &text);
    void finishAgentMessage();
    void addPermissionRequest(const QJsonValue &id,
                              const Acp::RequestPermissionRequest &request);

    void addAuthenticationRequest(const QList<Acp::AuthMethod> &methods);
    void showAuthenticationError(const QString &error);
    void resolveAuthentication();

signals:
    void sendRequested(const QString &text);
    void cancelRequested();
    void disconnectRequested();
    void configOptionChanged(const QString &configId, const QString &value);
    void permissionOptionSelected(const QJsonValue &id, const QString &optionId);
    void permissionCancelled(const QJsonValue &id);
    void authenticateRequested(const QString &methodId);

private:
    // Session toolbar
    QLabel *m_agentLabel;
    QHBoxLayout *m_configOptionsLayout;
    QMap<QString, QComboBox*> m_configCombos; // configId -> combo
    QLabel *m_elapsedLabel;
    QTimer *m_elapsedTimer;
    qint64 m_promptStartTime = 0;

    // Streaming indicator
    Utils::ProgressIndicator *m_progressIndicator = nullptr;

    // Message area
    AcpMessageView *m_messageView;

    // Input
    ChatInputEdit *m_inputEdit;
    SendButton *m_sendButton;
    QToolButton *m_commandsButton;

    bool m_prompting = false;
};

} // namespace AcpClient::Internal
