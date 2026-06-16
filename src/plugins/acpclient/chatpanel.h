// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <acp/acp.h>

#include <utils/filepath.h>

#include <QJsonValue>
#include <QList>
#include <QWidget>

namespace Utils {
class QtcButton;
class QtcIconButton;
} // namespace Utils

class QHBoxLayout;
class QLayout;
class QMenu;
class QToolButton;

namespace AcpClient::Internal {

class AcpMessageView;
class ChatInputEdit;
class SendButton;
class SessionPickerWidget;

class ChatPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPanel(QWidget *parent = nullptr);

    AcpMessageView *messageView() const { return m_messageView; }
    ChatInputEdit *inputEdit() const { return m_inputEdit; }

    void setAgentIcon(const QString &iconUrl = {});
    void setPrompting(bool prompting);
    void setSendEnabled(bool enabled);

    void setConfigOptions(const QList<Acp::SessionConfigOption> &configOptions);
    void setSessionModes(const QList<Acp::SessionMode> &modes, const QString &currentModeId);
    void setCurrentMode(const QString &modeId);
    void clear();
    void clearConfigOptions();

    void updateAvailableCommands(const QList<Acp::AvailableCommand> &commands);
    bool includeCurrentEditorContext() const { return m_includeCurrentEditorContext; }
    QList<Utils::FilePath> manualContextFiles() const { return m_manualContextFiles; }

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

    SessionPickerWidget *addSessionPicker();

signals:
    void sendRequested(const QString &text);
    void cancelRequested();
    void configOptionChanged(const QString &configId, const QString &value);
    void modeChanged(const QString &modeId);
    void permissionOptionSelected(const QJsonValue &id, const QString &optionId);
    void permissionCancelled(const QJsonValue &id);
    void authenticateRequested(const QString &methodId);

private:
    QList<Acp::SessionConfigOption> m_configOptions;
    void showConfigMenu();

    void updateConfigOptions();

    QList<Acp::SessionMode> m_sessionModes;
    QString m_currentModeId;
    void showModeMenu();
    void updateModeButton();

    // Message area
    AcpMessageView *m_messageView;

    // Input
    ChatInputEdit *m_inputEdit;
    Utils::QtcButton *m_sendButton;
    Utils::QtcIconButton *m_commandsButton;
    Utils::QtcIconButton *m_configButton = nullptr;
    Utils::QtcButton *m_modeButton = nullptr;
    QMenu *m_commandsMenu = nullptr;
    QWidget *m_contextBar = nullptr;
    QLayout *m_contextBarLayout = nullptr;
    bool m_includeCurrentEditorContext = true;
    QList<Utils::FilePath> m_manualContextFiles;

    void updateContextBar();

    bool m_prompting = false;
};

} // namespace AcpClient::Internal
