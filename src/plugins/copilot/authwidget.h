// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "copilotclient.h"

#include <utils/progressindicator.h>

#include <QLabel>
#include <QPushButton>
#include <QWidget>

namespace LanguageClient {
class Client;
}

namespace Copilot {

class AuthWidget : public QWidget
{
    Q_OBJECT

    enum class Status { SignedIn, SignedOut, Unknown };

public:
    explicit AuthWidget(QWidget *parent = nullptr);

private:
    void onClientAdded(LanguageClient::Client *client);
    void setState(const QString &buttonText, bool working);
    void checkStatus(Internal::CopilotClient *client);

    void signIn();
    void signOut();

private:
    Status m_status = Status::Unknown;
    QPushButton *m_button = nullptr;
    QLabel *m_statusLabel = nullptr;
    Utils::ProgressIndicator *m_progressIndicator = nullptr;
};

} // namespace Copilot
