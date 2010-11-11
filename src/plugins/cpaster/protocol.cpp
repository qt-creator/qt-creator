/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "protocol.h"

#include <cpptools/cpptoolsconstants.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/networkaccessmanager.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <QtCore/QUrl>
#include <QtCore/QDebug>
#include <QtCore/QVariant>

#include <QtGui/QMessageBox>
#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>

namespace CodePaster {

Protocol::Protocol()
        : QObject()
{
}

Protocol::~Protocol()
{
}

bool Protocol::hasSettings() const
{
    return false;
}

bool Protocol::checkConfiguration(QString *)
{
    return true;
}

Core::IOptionsPage *Protocol::settingsPage() const
{
    return 0;
}

void Protocol::list()
{
    qFatal("Base Protocol list() called");
}

Protocol::ContentType Protocol::contentType(const QString &mt)
{
    if (mt  == QLatin1String(CppTools::Constants::C_SOURCE_MIMETYPE)
        || mt == QLatin1String(CppTools::Constants::C_HEADER_MIMETYPE)
        || mt == QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)
        || mt == QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE)
        || mt == QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE))
        return C;
    if (mt == QLatin1String(QmlJSEditor::Constants::QML_MIMETYPE)
        || mt == QLatin1String(QmlJSEditor::Constants::JS_MIMETYPE))
        return JavaScript;
    if (mt == QLatin1String("text/x-patch"))
        return Diff;
    if (mt == QLatin1String("text/xml") || mt == QLatin1String("application/xml"))
        return Xml;
    return Text;
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

QString Protocol::textFromHtml(QString data)
{
    data.remove(QLatin1Char('\r'));
    data.replace(QLatin1String("&lt;"), QString(QLatin1Char('<')));
    data.replace(QLatin1String("&gt;"), QString(QLatin1Char('>')));
    data.replace(QLatin1String("&amp;"), QString(QLatin1Char('&')));
    data.replace(QLatin1String("&quot;"), QString(QLatin1Char('"')));
    return data;
}

bool Protocol::ensureConfiguration(Protocol *p, QWidget *parent)
{
    QString errorMessage;
    bool ok = false;
    while (true) {
        if (p->checkConfiguration(&errorMessage)) {
            ok = true;
            break;
        }
        // Cancel returns empty error message.
        if (errorMessage.isEmpty() || !showConfigurationError(p, errorMessage, parent))
            break;
    }
    return ok;
}

bool Protocol::showConfigurationError(const Protocol *p,
                                      const QString &message,
                                      QWidget *parent,
                                      bool showConfig)
{
    if (!p->settingsPage())
        showConfig = false;

    if (!parent)
        parent = Core::ICore::instance()->mainWindow();
    const QString title = tr("%1 - Configuration Error").arg(p->name());
    QMessageBox mb(QMessageBox::Warning, title, message, QMessageBox::Cancel, parent);
    QPushButton *settingsButton = 0;
    if (showConfig)
        settingsButton = mb.addButton(tr("Settings..."), QMessageBox::AcceptRole);
    mb.exec();
    bool rc = false;
    if (mb.clickedButton() == settingsButton)
        rc = Core::ICore::instance()->showOptionsDialog(p->settingsPage()->category(),
                                                        p->settingsPage()->id(),
                                                        parent);
    return rc;
}


// ------------ NetworkAccessManagerProxy
NetworkAccessManagerProxy::NetworkAccessManagerProxy()
{
}

NetworkAccessManagerProxy::~NetworkAccessManagerProxy()
{
}

QNetworkReply *NetworkAccessManagerProxy::httpGet(const QString &link)
{
    QUrl url(link);
    QNetworkRequest r(url);
    return networkAccessManager()->get(r);
}

QNetworkReply *NetworkAccessManagerProxy::httpPost(const QString &link, const QByteArray &data)
{
    QUrl url(link);
    QNetworkRequest r(url);
    // Required for Qt 4.8
    r.setHeader(QNetworkRequest::ContentTypeHeader,
                QVariant(QByteArray("application/x-www-form-urlencoded")));
    return networkAccessManager()->post(r, data);
}

QNetworkAccessManager *NetworkAccessManagerProxy::networkAccessManager()
{
    if (m_networkAccessManager.isNull())
        m_networkAccessManager.reset(new Core::NetworkAccessManager);
    return m_networkAccessManager.data();
}

// --------- NetworkProtocol

NetworkProtocol::NetworkProtocol(const NetworkAccessManagerProxyPtr &nw) :
    m_networkAccessManager(nw)
{
}

NetworkProtocol::~NetworkProtocol()
{
}

bool NetworkProtocol::httpStatus(QString url, QString *errorMessage)
{
    // Connect to host and display a message box, using its event loop.
    errorMessage->clear();
    const QString httpPrefix = QLatin1String("http://");
    if (!url.startsWith(httpPrefix)) {
        url.prepend(httpPrefix);
        url.append(QLatin1Char('/'));
    }
    QScopedPointer<QNetworkReply> reply(httpGet(url));
    QMessageBox box(QMessageBox::Information,
                    tr("Checking connection"),
                    tr("Connecting to %1...").arg(url),
                    QMessageBox::Cancel,
                    Core::ICore::instance()->mainWindow());
    connect(reply.data(), SIGNAL(finished()), &box, SLOT(close()));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    box.exec();
    QApplication::restoreOverrideCursor();
    // User canceled, discard and be happy.
    if (!reply->isFinished()) {
        QNetworkReply *replyPtr = reply.take();
        connect(replyPtr, SIGNAL(finished()), replyPtr, SLOT(deleteLater()));
        return false;
    }
    // Passed
    if (reply->error() == QNetworkReply::NoError)
        return true;
    // Error.
    *errorMessage = reply->errorString();
    return false;
}

} //namespace CodePaster
