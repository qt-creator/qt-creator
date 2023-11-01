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
    ~AuthWidget() override;

    void updateClient(const Utils::FilePath &nodeJs, const Utils::FilePath &agent);

private:
    void setState(const QString &buttonText, const QString &errorText, bool working);
    void checkStatus();


    void signIn();
    void signOut();

private:
    Status m_status = Status::Unknown;
    QPushButton *m_button = nullptr;
    QLabel *m_statusLabel = nullptr;
    Utils::ProgressIndicator *m_progressIndicator = nullptr;
    Internal::CopilotClient *m_client = nullptr;
};

} // namespace Copilot
