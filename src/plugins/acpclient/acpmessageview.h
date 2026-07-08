// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <acp/acp.h>

#include <QHash>
#include <QJsonValue>
#include <QList>
#include <QPair>
#include <QScrollArea>

QT_BEGIN_NAMESPACE
class QElapsedTimer;
class QLabel;
class QTimer;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils { class ProgressIndicator; }

namespace AcpClient::Internal {

class AgentMessageWidget;
class AuthenticationWidget;
class SessionPickerWidget;
class ThoughtWidget;
class ToolCallDetailWidget;
class ToolCallGroupWidget;

class AcpMessageView : public QScrollArea
{
    Q_OBJECT

public:
    explicit AcpMessageView(QWidget *parent = nullptr);

    void setAgentIconUrl(const QString &iconUrl);
    void setThoughtsVisible(bool visible);
    bool thoughtsVisible() const { return m_thoughtsVisible; }

    void setPrompting(bool prompting);

    void clear();
    void addUserMessage(const QString &text);
    void appendAgentText(const QString &text);
    void appendAgentThought(const QString &text);
    void addToolCall(const Acp::ToolCall &toolCall);
    void updateToolCall(const Acp::ToolCallUpdate &update);
    void addPlan(const Acp::Plan &plan);
    void addStatusMessage(const QString &text);
    void addErrorMessage(const QString &text);
    void finishAgentMessage();

    void addPermissionRequest(const QJsonValue &id,
                              const Acp::RequestPermissionRequest &request);
    void cancelPermissionRequest(const QJsonValue &id);

    void addAuthenticationRequest(const QList<Acp::AuthMethod> &methods);
    void showAuthenticationError(const QString &error);
    void resolveAuthentication();

    SessionPickerWidget *addSessionPicker();

signals:
    void permissionOptionSelected(const QJsonValue &id, const QString &optionId);
    void permissionCancelled(const QJsonValue &id);
    void authenticateRequested(const QString &methodId);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateElapsedTimeLabel();
    void scrollToBottom();
    QWidget *wrapWithSpacer(QWidget *widget, Qt::Alignment side);
    ToolCallGroupWidget *ensureToolCallGroup();
    void finishToolCallGroup();
    void addWidget(QWidget *widget);
    int contentMaxWidth() const;

    QWidget *m_container = nullptr;
    QVBoxLayout *m_layout = nullptr;
    QLabel *m_elapsedLabel = nullptr;
    Utils::ProgressIndicator *m_progressIndicator = nullptr;
    QElapsedTimer *m_elapsedTimer = nullptr;
    QTimer *m_progressUpdateTimer = nullptr;
    AgentMessageWidget *m_currentAgentWidget = nullptr;
    ThoughtWidget *m_currentThoughtWidget = nullptr;
    QList<ThoughtWidget *> m_thoughtWidgets;
    bool m_thoughtsVisible = true;
    ToolCallGroupWidget *m_currentToolCallGroup = nullptr;
    QHash<QString, ToolCallDetailWidget *> m_toolCallDetailWidgets;
    QList<QPair<QJsonValue, ToolCallDetailWidget *>> m_pendingPermissionRequests;
    QHash<QString, ToolCallGroupWidget *> m_toolCallGroups; // toolCallId -> owning group
    AuthenticationWidget *m_currentAuthWidget = nullptr;
    QString m_agentIconUrl;
    bool m_autoScroll = true;
};

} // namespace AcpClient::Internal
