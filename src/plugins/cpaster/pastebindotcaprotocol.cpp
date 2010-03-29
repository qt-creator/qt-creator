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

#include "pastebindotcaprotocol.h"
#include "cgi.h"

#include <QtNetwork/QNetworkReply>

using namespace Core;
namespace CodePaster {
PasteBinDotCaProtocol::PasteBinDotCaProtocol()
{
    connect(&http, SIGNAL(requestFinished(int,bool)),
            this, SLOT(postRequestFinished(int,bool)));
}

void PasteBinDotCaProtocol::fetch(const QString &id)
{
    QString link = QLatin1String("http://pastebin.ca/raw/");
    link.append(id);
    QUrl url(link);
    QNetworkRequest r(url);

    reply = manager.get(r);
    connect(reply, SIGNAL(finished()), this, SLOT(fetchFinished()));
    fetchId = id;
}

void PasteBinDotCaProtocol::paste(const QString &text,
                                  ContentType /* ct */,
                                  const QString &username,
                                  const QString &comment,
                                  const QString &description)
{
    Q_UNUSED(comment);
    Q_UNUSED(description);
    QString data = "content=";
    data += CGI::encodeURL(fixNewLines(text));
    data += "&description=";
    data += CGI::encodeURL(description);
    data += "&type=1&expiry=1%20day&name=";
    data += CGI::encodeURL(username);
    QHttpRequestHeader header("POST", "/quiet-paste.php");
    header.setValue("host", "pastebin.ca" );
    header.setContentType("application/x-www-form-urlencoded");
    http.setHost("pastebin.ca", QHttp::ConnectionModeHttp);
    header.setValue("User-Agent", "CreatorPastebin");
    postId = http.request(header, data.toAscii());
}

void PasteBinDotCaProtocol::postRequestFinished(int id, bool error)
{
    QString link;
    if (id == postId) {
        if (!error) {
            QByteArray data = http.readAll();
            link = QString::fromLatin1("http://pastebin.ca/") + QString(data).remove("SUCCESS:");
        } else
            link = http.errorString();
        emit pasteDone(link);
    }
}

void PasteBinDotCaProtocol::fetchFinished()
{
    QString title;
    QString content;
    bool error = reply->error();
    if (error) {
        content = reply->errorString();
    } else {
        title = QString::fromLatin1("Pastebin.ca: %1").arg(fetchId);
        content = reply->readAll();
    }
    reply->deleteLater();
    reply = 0;
    emit fetchDone(title, content, error);
}
} // namespace CodePaster
