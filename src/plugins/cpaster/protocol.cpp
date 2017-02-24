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

#include "protocol.h"

#include <utils/networkaccessmanager.h>

#include <cpptools/cpptoolsconstants.h>
#include <designer/designerconstants.h>
#include <glsleditor/glsleditorconstants.h>
#include <qmljstools/qmljstoolsconstants.h>
#include <resourceeditor/resourceeditorconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <QNetworkRequest>
#include <QNetworkReply>

#include <QUrl>
#include <QDebug>
#include <QVariant>

#include <QMessageBox>
#include <QApplication>
#include <QPushButton>

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
    if (mt == QLatin1String(CppTools::Constants::C_SOURCE_MIMETYPE)
        || mt == QLatin1String(CppTools::Constants::C_HEADER_MIMETYPE)
        || mt == QLatin1String(GlslEditor::Constants::GLSL_MIMETYPE)
        || mt == QLatin1String(GlslEditor::Constants::GLSL_MIMETYPE_VERT)
        || mt == QLatin1String(GlslEditor::Constants::GLSL_MIMETYPE_FRAG)
        || mt == QLatin1String(GlslEditor::Constants::GLSL_MIMETYPE_VERT_ES)
        || mt == QLatin1String(GlslEditor::Constants::GLSL_MIMETYPE_FRAG_ES))
        return C;
    if (mt == QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)
        || mt == QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE)
        || mt == QLatin1String(CppTools::Constants::OBJECTIVE_C_SOURCE_MIMETYPE)
        || mt == QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE))
        return Cpp;
    if (mt == QLatin1String(QmlJSTools::Constants::QML_MIMETYPE)
        || mt == QLatin1String(QmlJSTools::Constants::QMLPROJECT_MIMETYPE)
        || mt == QLatin1String(QmlJSTools::Constants::QBS_MIMETYPE)
        || mt == QLatin1String(QmlJSTools::Constants::JS_MIMETYPE)
        || mt == QLatin1String(QmlJSTools::Constants::JSON_MIMETYPE))
        return JavaScript;
    if (mt == QLatin1String("text/x-patch"))
        return Diff;
    if (mt == QLatin1String("text/xml")
        || mt == QLatin1String("application/xml")
        || mt == QLatin1String(ResourceEditor::Constants::C_RESOURCE_MIMETYPE)
        || mt == QLatin1String(Designer::Constants::FORM_MIMETYPE))
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
        parent = Core::ICore::mainWindow();
    const QString title = tr("%1 - Configuration Error").arg(p->name());
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

// --------- NetworkProtocol

QNetworkReply *NetworkProtocol::httpGet(const QString &link)
{
    QUrl url(link);
    QNetworkRequest r(url);
    return Utils::NetworkAccessManager::instance()->get(r);
}

QNetworkReply *NetworkProtocol::httpPost(const QString &link, const QByteArray &data)
{
    QUrl url(link);
    QNetworkRequest r(url);
    r.setHeader(QNetworkRequest::ContentTypeHeader,
                QVariant(QByteArray("application/x-www-form-urlencoded")));
    return Utils::NetworkAccessManager::instance()->post(r, data);
}

NetworkProtocol::~NetworkProtocol()
{
}

bool NetworkProtocol::httpStatus(QString url, QString *errorMessage, bool useHttps)
{
    // Connect to host and display a message box, using its event loop.
    errorMessage->clear();
    const QString httpPrefix = QLatin1String("http://");
    const QString httpsPrefix = QLatin1String("https://");
    if (!url.startsWith(httpPrefix) && !url.startsWith(httpsPrefix)) {
        url.prepend(useHttps ? httpsPrefix : httpPrefix);
        url.append(QLatin1Char('/'));
    }
    QScopedPointer<QNetworkReply> reply(httpGet(url));
    QMessageBox box(QMessageBox::Information,
                    tr("Checking connection"),
                    tr("Connecting to %1...").arg(url),
                    QMessageBox::Cancel,
                    Core::ICore::mainWindow());
    connect(reply.data(), &QNetworkReply::finished, &box, &QWidget::close);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    box.exec();
    QApplication::restoreOverrideCursor();
    // User canceled, discard and be happy.
    if (!reply->isFinished()) {
        QNetworkReply *replyPtr = reply.take();
        connect(replyPtr, &QNetworkReply::finished, replyPtr, &QNetworkReply::deleteLater);
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
