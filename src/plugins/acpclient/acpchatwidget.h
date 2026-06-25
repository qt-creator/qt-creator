// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

class QComboBox;
class QStackedWidget;
class QTabBar;
class QToolButton;
class QVBoxLayout;

namespace AcpClient::Internal {

class AcpChatTab;
class AcpInspector;

class AcpChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AcpChatWidget(QWidget *parent = nullptr);
    ~AcpChatWidget() override;

    void setInspector(AcpInspector *inspector);

    void setFocus();

signals:
    void navigateStateUpdate();

private:
    void setUseTabs(bool useTabs);
    AcpChatTab *addNewTab();
    void closeTab(int index);
    void setCurrentIndex(int index);

    QToolButton *m_addButton = nullptr;
    QToolButton *m_closeChatButton = nullptr;

    // Both selectors always exist and stay in sync with m_stack; visibility follows
    // the "Use tabbed editors" setting (m_tabBar when on, m_switcher when off).
    QTabBar *m_tabBar = nullptr;
    QComboBox *m_switcher = nullptr;
    QStackedWidget *m_stack = nullptr;
    AcpInspector *m_inspector = nullptr;
    bool m_blockIndexChanges = false;
};

} // namespace AcpClient::Internal
