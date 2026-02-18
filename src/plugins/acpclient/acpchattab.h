// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;

namespace Utils { class PathChooser; }

namespace AcpClient::Internal {

class AcpChatController;
class AcpInspector;
class ChatPanel;

class AcpChatTab : public QWidget
{
    Q_OBJECT

public:
    explicit AcpChatTab(QWidget *parent = nullptr);
    ~AcpChatTab() override;

    void setInspector(AcpInspector *inspector);

    bool hasFocus() const;
    void setFocus();

    QString title() const;

signals:
    void titleChanged();

private:
    void connectToAgent();
    void disconnectFromAgent();
    void populateServerCombo();
    void updateTitle();

    // Config page
    QStackedWidget *m_stack;
    QComboBox *m_serverCombo;
    Utils::PathChooser *m_cwdEdit;
    QLabel *m_noServerLabel;
    QLabel *m_connectionErrorLabel = nullptr;
    QPushButton *m_connectButton;

    // Auth page
    QComboBox *m_authMethodCombo = nullptr;
    QLabel *m_authDescriptionLabel = nullptr;
    QLabel *m_authErrorLabel = nullptr;

    // Chat page
    ChatPanel *m_chatPanel;

    AcpChatController *m_controller;
    QString m_title;
};

} // namespace AcpClient::Internal
