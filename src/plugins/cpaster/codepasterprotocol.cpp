/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "codepasterprotocol.h"
#include "codepastersettings.h"
#include "cpasterplugin.h"
#include "cgi.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/messageoutputwindow.h>

using namespace CodePaster;
using namespace Core;

CodePasterProtocol::CodePasterProtocol()
{
    m_page = new CodePaster::CodePasterSettingsPage();
    connect(&http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
            this, SLOT(readPostResponseHeader(const QHttpResponseHeader&)));
}

CodePasterProtocol::~CodePasterProtocol()
{
}

QString CodePasterProtocol::name() const
{
    return "CodePaster";
}

bool CodePasterProtocol::canList() const
{
    return true;
}

bool CodePasterProtocol::isValidHostName(const QString& hostName)
{
    if (hostName.isEmpty()) {
        ICore::instance()->messageManager()->printToOutputPane(
#ifdef Q_OS_MAC
                       tr("No Server defined in the CodePaster preferences!"),
#else
                       tr("No Server defined in the CodePaster options!"),
#endif
                       true /*error*/);
        ICore::instance()->messageManager()->showOutputPane();
        return false;
    }
    return true;
}

void CodePasterProtocol::fetch(const QString &id)
{
    QString hostName = m_page->hostName();
    if (!isValidHostName(hostName))
        return;
    QString link = "http://";
    link.append(hostName);
    link.append("/?format=raw&id=");
    link.append(id);
    QUrl url(link);
    QNetworkRequest r(url);

    reply = manager.get(r);
    connect(reply, SIGNAL(finished()), this, SLOT(fetchFinished()));
    fetchId = id;
}

void CodePasterProtocol::list(QListWidget *listWidget)
{
    QString hostName = m_page->hostName();
    if (!isValidHostName(hostName))
        return;
    this->listWidget = listWidget;
    QString link = QLatin1String("http://");
    link += hostName;
    link += QLatin1String("/?command=browse&format=raw");
    QUrl url(link);
    QNetworkRequest r(url);
    listReply = manager.get(r);
    connect(listReply, SIGNAL(finished()), this, SLOT(listFinished()));
}

void CodePasterProtocol::paste(const QString &text,
                       const QString &username,
                       const QString &comment,
                       const QString &description)
{
    QString hostName = m_page->hostName();
    if (!isValidHostName(hostName))
        return;

    QByteArray data = "command=processcreate&submit=submit&highlight_type=0&description=";
    data += CGI::encodeURL(description).toLatin1();
    data += "&comment=";
    data += CGI::encodeURL(comment).toLatin1();
    data += "&code=";
    data += CGI::encodeURL(text).toLatin1();
    data += "&poster=";
    data += CGI::encodeURL(username).toLatin1();

    http.setHost(hostName);
    http.post("/", data);
}

bool CodePasterProtocol::hasSettings() const
{
    return true;
}

Core::IOptionsPage* CodePasterProtocol::settingsPage()
{
    return m_page;
}

void CodePasterProtocol::fetchFinished()
{
    QString title;
    QString content;
    bool error = reply->error();
    if (error) {
        content = reply->errorString();
    } else {
        content = reply->readAll();
        if (content.contains("<B>No such paste!</B>")) {
            content = tr("No such paste");
            error = true;
        }
        title = QString::fromLatin1("Codepaster: %1").arg(fetchId);
    }
    reply->deleteLater();
    reply = 0;
    emit fetchDone(title, content, error);
}

void CodePasterProtocol::listFinished()
{
    if (listReply->error()) {
        ICore::instance()->messageManager()->printToOutputPane(listReply->errorString(), true);
    } else {
        QByteArray data = listReply->readAll();
        listWidget->clear();
        QStringList lines = QString(data).split(QLatin1Char('\n'));
        listWidget->addItems(lines);
        listWidget = 0;
    }
    listReply->deleteLater();
    listReply = 0;
}

void CodePasterProtocol::readPostResponseHeader(const QHttpResponseHeader &header)
{
    QString link = header.value("location");
    if (!link.isEmpty())
        emit pasteDone(link);
}
