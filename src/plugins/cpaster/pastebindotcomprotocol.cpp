/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "pastebindotcomprotocol.h"
#include "pastebindotcomsettings.h"
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/messageoutputwindow.h>

#include <QDebug>
#include <QtNetwork/QHttp>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>

using namespace Core;

PasteBinDotComProtocol::PasteBinDotComProtocol()
{
    settings = new PasteBinDotComSettings();
    connect(&http, SIGNAL(requestFinished(int,bool)),
            this, SLOT(postRequestFinished(int,bool)));
    connect(&http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
            this, SLOT(readPostResponseHeader(const QHttpResponseHeader&)));
}

void PasteBinDotComProtocol::fetch(const QString &id)
{
    QString link = QLatin1String("http://");
    if (!settings->hostPrefix().isEmpty())
        link.append(QString("%1.").arg(settings->hostPrefix()));
    link.append("pastebin.com/pastebin.php?dl=");
    link.append(id);
    QUrl url(link);
    QNetworkRequest r(url);

    reply = manager.get(r);
    connect(reply, SIGNAL(finished()), this, SLOT(fetchFinished()));
    fetchId = id;
}

void PasteBinDotComProtocol::paste(const QString &text,
                                   const QString &username,
                                   const QString &comment,
                                   const QString &description)
{
    Q_UNUSED(comment);
    Q_UNUSED(description);
    QString data = "code2=";
    data += text;
    data += "&parent_pid=&format=text&expiry=d&poster=";
    data += username;
    data += "&paste=Send";
    QHttpRequestHeader header("POST", "/pastebin.php");
    header.setValue("host", "qt.pastebin.com" );
    header.setContentType("application/x-www-form-urlencoded");
    http.setHost("qt.pastebin.com", QHttp::ConnectionModeHttp);
    header.setValue("User-Agent", "CreatorPastebin");
    postId = http.request(header, data.toAscii());
}

void PasteBinDotComProtocol::readPostResponseHeader(const QHttpResponseHeader &header)
{
    switch (header.statusCode())
    {
        // If we receive any of those, everything is bon.
    case 200:
    case 301:
    case 303:
    case 307:
        break;

    case 302: {
        QString link = header.value("Location");
        emit pasteDone(link);
        break;
    }
    default:
        emit pasteDone(tr("Error during paste"));
    }
}

void PasteBinDotComProtocol::postRequestFinished(int id, bool error)
{
    if (id == postId && error)
        emit pasteDone(http.errorString());
}

void PasteBinDotComProtocol::fetchFinished()
{
    QString title;
    QString content;
    bool error = reply->error();
    if (error) {
        content = reply->errorString();
    } else {
        title = QString::fromLatin1("Pastebin.com: %1").arg(fetchId);
        content = reply->readAll();
    }
    reply->deleteLater();
    reply = 0;
    emit fetchDone(title, content, error);
}

Core::IOptionsPage* PasteBinDotComProtocol::settingsPage()
{
    return settings;
}
