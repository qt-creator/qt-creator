// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "acpsettings.h"

#include <utils/filepath.h>

#include <QWidget>

#include <functional>
#include <optional>

class QStackedWidget;
class QToolButton;
class QVBoxLayout;

namespace AcpClient::Internal {

class AgentOverview;
class RunningIndicator;
class SessionSwitcherButton;
class AcpChatTab;
class AcpInspector;
class AcpTermsWidget;

class AcpChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AcpChatWidget(QWidget *parent = nullptr);
    ~AcpChatWidget() override;

    void setInspector(AcpInspector *inspector);

    void setFocus();

    void addTab(AcpChatTab *tab);

    void startChat(const AcpSettings::ServerInfo &serverInfo,
                   const Utils::FilePath &workingDirectory, const QString &prompt,
                   const std::function<void(AcpChatTab *)> &onStarted = {});

signals:
    void navigateStateUpdate();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    AcpChatTab *addNewTab();
    void closeTab(int index);
    AcpChatTab *takeTab(int index);
    void handleTabRemoved();
    void moveCurrentTabToEditor();
    void setCurrentIndex(int index);
    void addNewTabFromUser();

    void setOverviewShown(bool shown);
    void updateContentVisibility();
    void updateOverview();
    void updateSwitcherButton();

    void updateTermsState();
    void createTermsWidget();

    QVBoxLayout *m_layout = nullptr;
    SessionSwitcherButton *m_switcherButton = nullptr;
    QToolButton *m_addButton = nullptr;
    QToolButton *m_moveToEditorButton = nullptr;

    QStackedWidget *m_stack = nullptr;
    QStackedWidget *m_toolBarWidgetStack = nullptr; // kept in sync with m_stack

    // Sibling of m_chatArea in m_layout, never a page of m_stack: the overview and
    // m_toolBarWidgetStack are index-parallel with m_stack.
    AcpTermsWidget *m_termsWidget = nullptr;

    // m_overview above m_stack.
    QWidget *m_chatArea = nullptr;
    AgentOverview *m_overview = nullptr;
    // Shared by all running chats, so that their indicators turn in step.
    RunningIndicator *m_runningIndicator = nullptr;
    bool m_overviewShown = false;

    AcpInspector *m_inspector = nullptr;
    bool m_blockIndexChanges = false;

    struct StartRequest
    {
        AcpSettings::ServerInfo serverInfo;
        Utils::FilePath workingDirectory;
        QString prompt;
        std::function<void(AcpChatTab *)> onStarted;
    };
    // A chat requested while the terms were pending, started once they are accepted.
    std::optional<StartRequest> m_pendingStart;
};

} // namespace AcpClient::Internal
