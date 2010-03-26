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
#include "cgi.h"

#include <coreplugin/icore.h>

#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtNetwork/QNetworkReply>

using namespace Core;

enum { debug = 0 };

static const char phpScriptpC[] = "api_public.php";

namespace CodePaster {
PasteBinDotComProtocol::PasteBinDotComProtocol()
{
    settings = new PasteBinDotComSettings();
    connect(&http, SIGNAL(requestFinished(int,bool)),
            this, SLOT(postRequestFinished(int,bool)));
    connect(&http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
            this, SLOT(readPostResponseHeader(const QHttpResponseHeader&)));
}

QString PasteBinDotComProtocol::hostName() const
{

    QString rc = settings->hostPrefix();
    if (!rc.isEmpty())
        rc.append(QLatin1Char('.'));
    rc.append(QLatin1String("pastebin.com"));
    return rc;
}

void PasteBinDotComProtocol::fetch(const QString &id)
{
    QString link;
    QTextStream(&link) << "http://" << hostName() << '/' << phpScriptpC << "?dl=" << id;
    if (debug)
        qDebug() << "fetch: sending " << link;
    QUrl url(link);
    QNetworkRequest r(url);

    reply = manager.get(r);
    connect(reply, SIGNAL(finished()), this, SLOT(fetchFinished()));
    fetchId = id;
}

void PasteBinDotComProtocol::paste(const QString &text,
                                   const QString &username,
                                   const QString & /* comment */,
                                   const QString & /* description */)
{
    QString data;
    QTextStream str(&data);
    str << "paste_code=" << CGI::encodeURL(text) << "&paste_name="
            << CGI::encodeURL(username);
    QHttpRequestHeader header(QLatin1String("POST"), QLatin1String(phpScriptpC));

    const QString host = hostName();
    header.setValue(QLatin1String("host"), host);
    header.setContentType(QLatin1String("application/x-www-form-urlencoded"));
    http.setHost(host, QHttp::ConnectionModeHttp);
    header.setValue(QLatin1String("User-Agent"), QLatin1String("CreatorPastebin"));
    postId = http.request(header, data.toAscii());
    if (debug)
        qDebug() << "paste" << data << postId << host;
}

void PasteBinDotComProtocol::readPostResponseHeader(const QHttpResponseHeader &header)
{
    const int code = header.statusCode();
    if (debug)
        qDebug() << "readPostResponseHeader" << code << header.toString() << header.values();
    switch (code) {
        // If we receive any of those, everything is bon.
    case 301:
    case 303:
    case 307:
    case 200:
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
    if (id == postId && error) {
        const QString errorMessage = http.errorString();
        if (debug)
            qDebug() << "postRequestFinished" << id << errorMessage;
        emit pasteDone(errorMessage);
    }
}

void PasteBinDotComProtocol::fetchFinished()
{
    QString title;
    QString content;
    const bool error = reply->error();
    if (error) {
        content = reply->errorString();
        if (debug)
            qDebug() << "fetchFinished: error" << fetchId << content;
    } else {
        title = QString::fromLatin1("Pastebin.com: %1").arg(fetchId);
        content = QString::fromAscii(reply->readAll());
        if (debug)
            qDebug() << "fetchFinished: " << content.size();
    }
    reply->deleteLater();
    reply = 0;
    emit fetchDone(title, content, error);
}

Core::IOptionsPage* PasteBinDotComProtocol::settingsPage()
{
    return settings;
}
} // namespace CodePaster
