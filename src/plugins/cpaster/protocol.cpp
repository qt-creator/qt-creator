// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "protocol.h"

#include "cpastertr.h"

#include <coreplugin/icore.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/networkaccessmanager.h>

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QUrl>
#include <QVariant>

namespace CodePaster {

Protocol::Protocol(const ProtocolData &data)
    : m_protocolData(data)
{}

Protocol::~Protocol() = default;

void Protocol::list()
{
    qFatal("Base Protocol list() called");
}

QString Protocol::fixNewLines(QString data)
{
    // Copied from cpaster. Otherwise lineendings will screw up
    // HTML requires "\r\n".
    if (data.contains(QLatin1String("\r\n")))
        return data;
    if (data.contains(QLatin1Char('\n'))) {
        data.replace(QLatin1Char('\n'), QLatin1String("\r\n"));
        return data;
    }
    if (data.contains(QLatin1Char('\r')))
        data.replace(QLatin1Char('\r'), QLatin1String("\r\n"));
    return data;
}

// Show a configuration error and point user to settings.
// Return true when settings changed.
static bool showConfigurationError(const Protocol *p, const QString &message, QWidget *parent)
{
    const bool showConfig = p->settingsPage();

    if (!parent)
        parent = Core::ICore::dialogParent();
    const QString title = Tr::tr("%1 - Configuration Error").arg(p->name());
    QMessageBox mb(QMessageBox::Warning, title, message, QMessageBox::Cancel, parent);
    QPushButton *settingsButton = nullptr;
    if (showConfig)
        settingsButton = mb.addButton(Core::ICore::msgShowOptionsDialog(), QMessageBox::AcceptRole);
    mb.exec();
    bool rc = false;
    if (mb.clickedButton() == settingsButton)
        rc = Core::ICore::showOptionsDialog(p->settingsPage()->id(), parent);
    return rc;
}

bool Protocol::ensureConfiguration(Protocol *p, QWidget *parent)
{
    while (true) {
        const auto res = p->checkConfiguration();
        if (res)
            return true;
        // Cancel returns empty error message.
        if (res.error().isEmpty() || !showConfigurationError(p, res.error(), parent))
            break;
    }
    return false;
}

// --------- NetworkProtocol

static void addCookies(QNetworkRequest &request)
{
    auto accessMgr = Utils::NetworkAccessManager::instance();
    const QList<QNetworkCookie> cookies = accessMgr->cookieJar()->cookiesForUrl(request.url());
    for (const QNetworkCookie &cookie : cookies)
        request.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(cookie));
}

QNetworkReply *httpGet(const QString &link, bool handleCookies)
{
    QUrl url(link);
    QNetworkRequest r(url);
    if (handleCookies)
        addCookies(r);
    return Utils::NetworkAccessManager::instance()->get(r);
}

QNetworkReply *httpPost(const QString &link, const QByteArray &data,
                                         bool handleCookies)
{
    QUrl url(link);
    QNetworkRequest r(url);
    if (handleCookies)
        addCookies(r);
    r.setHeader(QNetworkRequest::ContentTypeHeader,
                QVariant(QByteArray("application/x-www-form-urlencoded")));
    return Utils::NetworkAccessManager::instance()->post(r, data);
}

} // CodePaster
