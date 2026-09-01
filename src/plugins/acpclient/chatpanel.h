// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "acpelicitationhandler.h"

#include <acp/acpv2.h>

#include <utils/filepath.h>

#include <QImage>
#include <QJsonValue>
#include <QList>
#include <QPointer>
#include <QWidget>

#include <optional>

namespace Utils {
class InfoLabel;
class QtcButton;
class QtcComboBox;
class QtcIconButton;
class QtcProgressBar;
} // namespace Utils

class QHBoxLayout;
class QLabel;
class QLayout;
class QMenu;
class QTimer;
class QToolButton;

namespace AcpClient::Internal {

enum class TextContextScope {
    None,      // not remembered (transient)
    Session,   // remember for this agent's session id
    Agent,     // remember for this agent
    AllAgents, // remember for all agents
};

struct TextContext
{
    QString name;
    QString text;
    TextContextScope scope = TextContextScope::Session;
};

struct ImageContext
{
    QString name;
    QImage image;
};

class AcpMessageView;
class ChatInputEdit;
class SendButton;
class SessionPickerWidget;
class TextContextEditor;

class ChatPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPanel(QWidget *parent = nullptr);
    ~ChatPanel() override;

    AcpMessageView *messageView() const { return m_messageView; }
    ChatInputEdit *inputEdit() const { return m_inputEdit; }
    QWidget *toolBarWidget() const { return m_toolBarWidget; }

    void setAgentIcon(const QString &iconUrl = {});
    void setAgentId(const QString &agentId);
    void setSessionId(const QString &sessionId);
    void setPrompting(bool prompting);
    void setSendEnabled(bool enabled);
    void setCanCloseSession(bool canClose);
    void setImagePasteSupported(bool supported) { m_imagePasteSupported = supported; }

    void setConfigOptions(const QList<Acp::V2::SessionConfigOption> &configOptions);
    void setUsage(const Acp::V2::UsageUpdate &usage);
    void setTokenUsageVisible(bool visible);
    bool tokenUsageVisible() const { return m_showTokenUsage; }
    void clear();
    void clearConfigOptions();

    void updateAvailableCommands(const QList<Acp::V2::AvailableCommand> &commands);
    bool includeCurrentEditorContext() const { return m_includeCurrentEditorContext; }
    QList<Utils::FilePath> manualContextFiles() const { return m_manualContextFiles; }
    QList<TextContext> textContexts() const { return m_textContexts; }
    QList<ImageContext> imageContexts() const { return m_imageContexts; }
    void clearImageContexts();

    // Delegate to message view
    void addUserMessage(const QString &text);
    void appendAgentText(const QString &text);
    void appendAgentThought(const QString &text);
    void updateToolCall(const Acp::V2::ToolCallUpdate &update);
    void updateTerminal(const Acp::V2::TerminalUpdate &update);
    void appendTerminalOutput(const Acp::V2::TerminalOutputChunk &chunk);
    void addPlan(const Acp::V2::PlanUpdate &plan);
    void addErrorMessage(const QString &text);
    void finishAgentMessage();
    void addPermissionRequest(const QJsonValue &id,
                              const Acp::V2::RequestPermissionRequest &request);
    void cancelPermissionRequest(const QJsonValue &id);
    void addElicitationRequest(const QJsonValue &id, const ElicitationRequest &request);
    void cancelElicitationRequest(const QJsonValue &id);
    void completeElicitationRequest(const QJsonValue &id);

    void addAuthenticationRequest(const QList<Acp::V2::AuthMethod> &methods);
    void showAuthenticationError(const QString &error);
    void resolveAuthentication();

    SessionPickerWidget *addSessionPicker();

signals:
    void sendRequested(const QString &text);
    void cancelRequested();
    void configOptionChanged(const QString &configId, const QJsonValue &value);
    void permissionOptionSelected(const QJsonValue &id, const QString &optionId);
    void permissionCancelled(const QJsonValue &id);
    void elicitationAccepted(const QJsonValue &id, const QJsonObject &content);
    void elicitationDeclined(const QJsonValue &id);
    void elicitationCancelled(const QJsonValue &id);
    void authenticateRequested(const QString &methodId);
    void inspectRequested();
    void closeSessionRequested();

private:
    QList<Acp::V2::SessionConfigOption> m_configOptions;
    void showConfigMenu();

    QString m_modeConfigId;
    void updateModeButton();

    std::optional<Acp::V2::UsageUpdate> m_usage;
    std::optional<Acp::V2::UsageUpdate> m_usageAtPromptStart;
    bool m_showTokenUsage = true;
    void updateUsageDisplay();

    // Widget shown in AcpChatWidget's tool bar while this panel's tab is active.
    QPointer<QWidget> m_toolBarWidget;
    QToolButton *m_switchSessionButton = nullptr;

    // Message area
    AcpMessageView *m_messageView;

    // Input
    ChatInputEdit *m_inputEdit;
    Utils::QtcButton *m_sendButton;
    Utils::QtcIconButton *m_commandsButton;
    Utils::QtcIconButton *m_configButton = nullptr;
    Utils::QtcComboBox *m_modeCombo = nullptr;
    Utils::QtcProgressBar *m_usageBar = nullptr;
    QLabel *m_usageLabel = nullptr;
    QMenu *m_commandsMenu = nullptr;
    QWidget *m_contextBar = nullptr;
    QLayout *m_contextBarLayout = nullptr;
    bool m_includeCurrentEditorContext = true;
    QList<Utils::FilePath> m_manualContextFiles;
    QList<TextContext> m_textContexts;
    QList<ImageContext> m_imageContexts;

    void updateContextBar();
    void addContextFiles(const QList<Utils::FilePath> &files);
    void addImageContext(const QImage &image);
    void showTransientInputMessage(const QString &text);

    QString m_agentId;
    QString m_sessionId;
    void reloadPersistedTextContexts();
    void persistTextContexts();

    QList<TextContext> textContextHistory() const;
    void addTextContextToHistory(const QString &name, const QString &text);
    void removeTextContextHistoryAt(int index);

    TextContextEditor *m_textContextEditor = nullptr;
    int m_editingTextContextIndex = -1;
    void showTextContextEditor(int index);
    void hideTextContextEditor();

    bool m_prompting = false;
    bool m_imagePasteSupported = false;

    Utils::InfoLabel *m_inputInfoLabel = nullptr;
    QTimer *m_inputInfoTimer = nullptr;
};

} // namespace AcpClient::Internal
