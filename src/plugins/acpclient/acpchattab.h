// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

class QAbstractButton;
class QComboBox;
class QLabel;
class QScrollArea;
class QStackedWidget;
class QVBoxLayout;

namespace Utils {
class FilePath;
class InfoLabel;
}

namespace AcpClient::Internal {

class AcpChatController;
class AcpInspector;
class ChatPanel;
class SessionPickerWidget;

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
    QString toolTip() const;
    QWidget *toolBarWidget() const;

signals:
    void titleChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum ConfigPage { NoServers, Connect, Registry };

    void populateServerButtons();
    void showRegistryPage();
    void clearRegistryButtons();
    void populateRegistryButtons();
    void registryFetchFailed();
    void showSessionPicker();

    // Config page
    QStackedWidget *m_stack;
    QStackedWidget *m_configStack;
    QVBoxLayout *m_serverButtonsLayout = nullptr;
    Utils::InfoLabel *m_noServerLabel;
    Utils::InfoLabel *m_connectionErrorLabel = nullptr;
    QScrollArea *m_registryScrollArea = nullptr;
    QVBoxLayout *m_registryButtonsLayout = nullptr;
    Utils::InfoLabel *m_registryInfoLabel = nullptr;

    // Initializing page
    QLabel *m_initializingLabel = nullptr;

    // Auth page
    QComboBox *m_authMethodCombo = nullptr;
    QLabel *m_authDescriptionLabel = nullptr;
    Utils::InfoLabel *m_authErrorLabel = nullptr;

    // Chat page
    ChatPanel *m_chatPanel;

    AcpChatController *m_controller;
    SessionPickerWidget *m_activePicker = nullptr;
    bool m_sessionPending = false;
    QString m_pendingPrompt;
};

} // namespace AcpClient::Internal
