/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "kdepasteprotocol.h"
#ifdef CPASTER_PLUGIN_GUI
#include "authenticationdialog.h"
#endif

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QByteArray>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QNetworkReply>

#include <algorithm>

enum { debug = 0 };

static inline QByteArray expiryParameter(int daysRequested)
{
    // Obtained by 'pastebin.kde.org/api/json/parameter/expire' on 26.03.2014
    static const int expiryTimesSec[] = {1800, 21600, 86400, 604800, 2592000, 31536000};
    const int *end = expiryTimesSec + sizeof(expiryTimesSec) / sizeof(expiryTimesSec[0]);
    // Find the first element >= requested span, search up to n - 1 such that 'end' defaults to last value.
    const int *match = std::lower_bound(expiryTimesSec, end - 1, 24 * 60 * 60 * daysRequested);
    return QByteArray("expire=") + QByteArray::number(*match);
}

namespace CodePaster {

void StickyNotesPasteProtocol::setHostUrl(const QString &hostUrl)
{
    m_hostUrl = hostUrl;
    if (!m_hostUrl.endsWith(QLatin1Char('/')))
        m_hostUrl.append(QLatin1Char('/'));
}

unsigned StickyNotesPasteProtocol::capabilities() const
{
    return ListCapability | PostDescriptionCapability;
}

bool StickyNotesPasteProtocol::checkConfiguration(QString *errorMessage)
{
    if (m_hostChecked)  // Check the host once.
        return true;
    const bool ok = httpStatus(m_hostUrl, errorMessage, true);
    if (ok)
        m_hostChecked = true;
    return ok;
}

// Query 'http://pastebin.kde.org/api/json/parameter/language' to obtain the valid values.
static inline QByteArray pasteLanguage(Protocol::ContentType ct)
{
    switch (ct) {
    case Protocol::Text:
        break;
    case Protocol::C:
        return "language=c";
    case Protocol::Cpp:
        return "language=cpp-qt";
    case Protocol::JavaScript:
        return "language=javascript";
    case Protocol::Diff:
        return "language=diff";
    case Protocol::Xml:
        return "language=xml";
    }
    return QByteArray("language=text");
}

void StickyNotesPasteProtocol::paste(const QString &text,
                                   ContentType ct, int expiryDays,
                                   const QString &username,
                                   const QString &comment,
                                   const QString &description)
{
    enum { maxDescriptionLength = 30 }; // Length of description is limited.

    Q_UNUSED(username)
    Q_UNUSED(comment);
    QTC_ASSERT(!m_pasteReply, return);

    // Format body
    QByteArray pasteData = "&data=";
    pasteData += QUrl::toPercentEncoding(fixNewLines(text));
    pasteData += '&';
    pasteData += pasteLanguage(ct);
    pasteData += '&';
    pasteData += expiryParameter(expiryDays);
    if (!description.isEmpty()) {
        pasteData += "&title=";
        pasteData += QUrl::toPercentEncoding(description.left(maxDescriptionLength));
    }

    m_pasteReply = httpPost(m_hostUrl + QLatin1String("api/json/create"), pasteData, true);
    connect(m_pasteReply, &QNetworkReply::finished, this, &StickyNotesPasteProtocol::pasteFinished);
    if (debug)
        qDebug() << "paste: sending " << m_pasteReply << pasteData;
}

// Parse for an element and return its contents
static QString parseElement(QIODevice *device, const QString &elementName)
{
    const QJsonDocument doc = QJsonDocument::fromJson(device->readAll());
    if (doc.isEmpty() || !doc.isObject())
        return QString();

    QJsonObject obj= doc.object();
    const QString resultKey = QLatin1String("result");

    if (obj.contains(resultKey)) {
        QJsonValue value = obj.value(resultKey);
        if (value.isObject()) {
            obj = value.toObject();
            if (obj.contains(elementName)) {
                value = obj.value(elementName);
                return value.toString();
            }
        } else if (value.isArray()) {
            qWarning() << "JsonArray not expected.";
        }
    }

    return QString();
}

void StickyNotesPasteProtocol::pasteFinished()
{
    if (m_pasteReply->error()) {
        qWarning("%s protocol error: %s", qPrintable(name()),qPrintable(m_pasteReply->errorString()));
    } else {
        // Parse id from '<result><id>143204</id><hash></hash></result>'
        // No useful error reports have been observed.
        const QString id = parseElement(m_pasteReply, QLatin1String("id"));
        if (id.isEmpty())
            qWarning("%s protocol error: Could not send entry.", qPrintable(name()));
        else
            emit pasteDone(m_hostUrl + id);
    }

    m_pasteReply->deleteLater();
    m_pasteReply = 0;
}

void StickyNotesPasteProtocol::fetch(const QString &id)
{
    QTC_ASSERT(!m_fetchReply, return);

    // Did we get a complete URL or just an id?
    m_fetchId = id;
    const int lastSlashPos = m_fetchId.lastIndexOf(QLatin1Char('/'));
    if (lastSlashPos != -1)
        m_fetchId.remove(0, lastSlashPos + 1);
    QString url = m_hostUrl + QLatin1String("api/json/show/") + m_fetchId;
    if (debug)
        qDebug() << "fetch: sending " << url;

    m_fetchReply = httpGet(url);
    connect(m_fetchReply, &QNetworkReply::finished,
            this, &StickyNotesPasteProtocol::fetchFinished);
}

// Parse: '<result><id>143228</id><author>foo</author><timestamp>1320661026</timestamp><language>text</language>
//  <data>bar</data></result>'

void StickyNotesPasteProtocol::fetchFinished()
{
    const QString title = name() + QLatin1String(": ") + m_fetchId;
    QString content;
    const bool error = m_fetchReply->error();
    if (error) {
        content = m_fetchReply->errorString();
        if (debug)
            qDebug() << "fetchFinished: error" << m_fetchId << content;
    } else {
        content = parseElement(m_fetchReply, QLatin1String("data"));
        content.remove(QLatin1Char('\r'));
    }
    m_fetchReply->deleteLater();
    m_fetchReply = 0;
    emit fetchDone(title, content, error);
}

void StickyNotesPasteProtocol::list()
{
    QTC_ASSERT(!m_listReply, return);

    // Trailing slash is important to prevent redirection.
    QString url = m_hostUrl + QLatin1String("api/json/list");
    m_listReply = httpGet(url);
    connect(m_listReply, &QNetworkReply::finished,
            this, &StickyNotesPasteProtocol::listFinished);
    if (debug)
        qDebug() << "list: sending " << url << m_listReply;
}

// Parse 'result><pastes><paste>id1</paste><paste>id2</paste>...'
static inline QStringList parseList(QIODevice *device)
{
    QStringList result;
    const QJsonDocument doc = QJsonDocument::fromJson(device->readAll());
    if (doc.isEmpty() || !doc.isObject())
        return result;

    QJsonObject obj= doc.object();
    const QString resultKey = QLatin1String("result");
    const QString pastesKey = QLatin1String("pastes");

    if (obj.contains(resultKey)) {
        QJsonValue value = obj.value(resultKey);
        if (value.isObject()) {
            obj = value.toObject();
            if (obj.contains(pastesKey)) {
                value = obj.value(pastesKey);
                if (value.isArray()) {
                    QJsonArray array = value.toArray();
                    foreach (const QJsonValue &val, array)
                        result.append(val.toString());
                }
            }
        }
    }
    return result;
}

void StickyNotesPasteProtocol::listFinished()
{
    const bool error = m_listReply->error();
    if (error) {
        if (debug)
            qDebug() << "listFinished: error" << m_listReply->errorString();
    } else {
        emit listDone(name(), parseList(m_listReply));
    }
    m_listReply->deleteLater();
    m_listReply = nullptr;
}

KdePasteProtocol::KdePasteProtocol()
{
    setHostUrl(QLatin1String("https://pastebin.kde.org/"));
    connect(this, &KdePasteProtocol::authenticationFailed, this, [this] () {
        m_loginFailed = true;
        paste(m_text, m_contentType, m_expiryDays, QString(), QString(), m_description);
    });
}

void KdePasteProtocol::paste(const QString &text, Protocol::ContentType ct, int expiryDays,
                             const QString &username, const QString &comment,
                             const QString &description)
{
    Q_UNUSED(username);
    Q_UNUSED(comment);
    // KDE paster needs authentication nowadays
#ifdef CPASTER_PLUGIN_GUI
    QString details = tr("Pasting to KDE paster needs authentication.<br/>"
                         "Enter your KDE Identity credentials to continue.");
    if (m_loginFailed)
        details.prepend(tr("<span style='background-color:LightYellow;color:red'>Login failed</span><br/><br/>"));

    AuthenticationDialog authDialog(details, Core::ICore::dialogParent());
    authDialog.setWindowTitle("Authenticate for KDE paster");
    if (authDialog.exec() != QDialog::Accepted) {
        m_loginFailed = false;
        return;
    }
    const QString user = authDialog.userName();
    const QString passwd = authDialog.password();
#else
    // FIXME get the credentials for the cmdline cpaster somehow
    const QString user;
    const QString passwd;
    qDebug() << "KDE needs credentials for pasting";
    return;
#endif
    // store input data as members to be able to use them after the authentication succeeded
    m_text = text;
    m_contentType = ct;
    m_expiryDays = expiryDays;
    m_description = description;
    authenticate(user, passwd);
}

QString KdePasteProtocol::protocolName()
{
    return QLatin1String("Paste.KDE.Org");
}

void KdePasteProtocol::authenticate(const QString &user, const QString &passwd)
{
    QTC_ASSERT(!m_authReply, return);

    // first we need to obtain the hidden form token for logging in
    m_authReply = httpGet(hostUrl() + "user/login");
    connect(m_authReply, &QNetworkReply::finished, this, [this, user, passwd] () {
        onPreAuthFinished(user, passwd);
    });
}

void KdePasteProtocol::onPreAuthFinished(const QString &user, const QString &passwd)
{
    if (m_authReply->error() != QNetworkReply::NoError) {
        m_authReply->deleteLater();
        m_authReply = nullptr;
        return;
    }
    const QByteArray page = m_authReply->readAll();
    m_authReply->deleteLater();
    const QRegularExpression regex("name=\"_token\"\\s+type=\"hidden\"\\s+value=\"(.*?)\">");
    const QRegularExpressionMatch match = regex.match(QLatin1String(page));
    if (!match.hasMatch()) {
        m_authReply = nullptr;
        return;
    }
    const QString token = match.captured(1);

    QByteArray data("username=" + QUrl::toPercentEncoding(user)
                    + "&password=" + QUrl::toPercentEncoding(passwd)
                    + "&_token=" + QUrl::toPercentEncoding(token));
    m_authReply = httpPost(hostUrl() + "user/login", data, true);
    connect(m_authReply, &QNetworkReply::finished, this, &KdePasteProtocol::onAuthFinished);
}

void KdePasteProtocol::onAuthFinished()
{
    if (m_authReply->error() != QNetworkReply::NoError) {
        m_authReply->deleteLater();
        m_authReply = nullptr;
        return;
    }
    const QVariant attribute = m_authReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    m_redirectUrl = redirectUrl(attribute.toUrl().toString(), m_redirectUrl);
    if (!m_redirectUrl.isEmpty()) { // we need to perform a redirect
        QUrl url(m_redirectUrl);
        if (url.path().isEmpty())
            url.setPath("/");   // avoid issue inside cookiesForUrl()
        m_authReply->deleteLater();
        m_authReply = httpGet(url.url(), true);
        connect(m_authReply, &QNetworkReply::finished, this, &KdePasteProtocol::onAuthFinished);
    } else { // auth should be done now
        const QByteArray page = m_authReply->readAll();
        m_authReply->deleteLater();
        m_authReply = nullptr;
        if (page.contains("https://identity.kde.org")) // we're back on the login page
            emit authenticationFailed();
        else {
            m_loginFailed = false;
            StickyNotesPasteProtocol::paste(m_text, m_contentType, m_expiryDays, QString(),
                                            QString(), m_description);
        }
    }
}

QString KdePasteProtocol::redirectUrl(const QString &redirect, const QString &oldRedirect) const
{
    QString redirectUrl;
    if (!redirect.isEmpty() && redirect != oldRedirect)
        redirectUrl = redirect;
    return redirectUrl;
}

} // namespace CodePaster
