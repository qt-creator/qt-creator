// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

class QTabWidget;
class QToolButton;

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
    AcpChatTab *addNewTab();
    void closeTab(int index);
    AcpChatTab *currentTab() const;

    QTabWidget *m_tabWidget;
    AcpInspector *m_inspector = nullptr;
};

} // namespace AcpClient::Internal
