// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "authwidget.h"

#include "copilotclient.h"
#include "copilottr.h"

#include <utils/layoutbuilder.h>
#include <utils/stringutils.h>

#include <languageclient/languageclientmanager.h>

#include <QDesktopServices>
#include <QMessageBox>

using namespace LanguageClient;
using namespace Copilot::Internal;

namespace Copilot {

bool isCopilotClient(Client *client)
{
    return dynamic_cast<CopilotClient *>(client) != nullptr;
}

CopilotClient *asCoPilotClient(Client *client)
{
    return static_cast<CopilotClient *>(client);
}

Internal::CopilotClient *findClient()
{
    CopilotClient *client = Internal::CopilotClient::instance();
    return client && client->reachable() ? client : nullptr;
}

AuthWidget::AuthWidget(QWidget *parent)
    : QWidget(parent)
{
    using namespace Utils::Layouting;

    m_button = new QPushButton(Tr::tr("Sign in"));
    m_button->setEnabled(false);
    m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Small);
    m_progressIndicator->setVisible(false);
    m_statusLabel = new QLabel();
    m_statusLabel->setVisible(false);

    // clang-format off
    Column {
        Row {
            m_button, m_progressIndicator, st
        },
        m_statusLabel
    }.attachTo(this);
    // clang-format on

    connect(LanguageClientManager::instance(),
            &LanguageClientManager::clientAdded,
            this,
            &AuthWidget::onClientAdded);

    connect(m_button, &QPushButton::clicked, this, [this]() {
        if (m_status == Status::SignedIn)
            signOut();
        else if (m_status == Status::SignedOut)
            signIn();
    });

    CopilotClient *client = findClient();

    if (client)
        checkStatus(client);
}

void AuthWidget::setState(const QString &buttonText, bool working)
{
    m_button->setText(buttonText);
    m_button->setVisible(true);
    m_progressIndicator->setVisible(working);
    m_statusLabel->setVisible(!m_statusLabel->text().isEmpty());
    m_button->setEnabled(!working);
}

void AuthWidget::onClientAdded(Client *client)
{
    if (isCopilotClient(client)) {
        auto coPilotClient = asCoPilotClient(client);
        if (coPilotClient->reachable()) {
            checkStatus(coPilotClient);
        } else {
            connect(client, &Client::initialized, this, [this, coPilotClient] {
                checkStatus(coPilotClient);
            });
        }
    }
}

void AuthWidget::checkStatus(CopilotClient *client)
{
    setState("Checking status ...", true);

    client->requestCheckStatus(false, [this](const CheckStatusRequest::Response &response) {
        if (response.error()) {
            setState("failed: " + response.error()->message(), false);
            return;
        }
        const CheckStatusResponse result = *response.result();

        if (result.user().isEmpty()) {
            setState("Sign in", false);
            m_status = Status::SignedOut;
            return;
        }

        setState("Sign out " + result.user(), false);
        m_status = Status::SignedIn;
    });
}

void AuthWidget::signIn()
{
    qCritical() << "Not implemented";
    auto client = findClient();
    QTC_ASSERT(client, return);

    setState("Signing in ...", true);

    client->requestSignInInitiate([this, client](const SignInInitiateRequest::Response &response) {
        QTC_ASSERT(!response.error(), return);

        Utils::setClipboardAndSelection(response.result()->userCode());

        QDesktopServices::openUrl(QUrl(response.result()->verificationUri()));

        m_statusLabel->setText(Tr::tr("A browser window will open, enter the code %1 when "
                                      "asked.\nThe code has been copied to your clipboard.")
                                   .arg(response.result()->userCode()));
        m_statusLabel->setVisible(true);

        client->requestSignInConfirm(response.result()->userCode(),
                                     [this](const SignInConfirmRequest::Response &response) {
                                         m_statusLabel->setText("");

                                         if (response.error()) {
                                             QMessageBox::critical(this,
                                                                   Tr::tr("Login failed"),
                                                                   Tr::tr(
                                                                       "The login request failed: ")
                                                                       + response.error()->message());
                                             setState("Sign in", false);
                                             return;
                                         }

                                         setState("Sign Out " + response.result()->user(), false);
                                     });
    });
}

void AuthWidget::signOut()
{
    auto client = findClient();
    QTC_ASSERT(client, return);

    setState("Signing out ...", true);

    client->requestSignOut([this, client](const SignOutRequest::Response &response) {
        QTC_ASSERT(!response.error(), return);
        QTC_ASSERT(response.result()->status() == "NotSignedIn", return);

        checkStatus(client);
    });
}

} // namespace Copilot
