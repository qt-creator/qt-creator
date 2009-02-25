/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "poster.h"
#include "cgi.h"

#include <QCoreApplication>
#include <QByteArray>
#include <QDebug>

Poster::Poster(const QString &host)
    : QHttp(host)
{
    m_status = 0;
    m_hadError = false;
    connect(this, SIGNAL(requestFinished(int,bool)), SLOT(gotRequestFinished(int,bool)));
    connect(this, SIGNAL(responseHeaderReceived(QHttpResponseHeader)), SLOT(gotResponseHeaderReceived(QHttpResponseHeader)));
}

void Poster::post(const QString &description, const QString &comment,
                  const QString &text, const QString &user)
{

    QByteArray data = "command=processcreate&submit=submit&highlight_type=0&description=";
    data += CGI::encodeURL(description).toLatin1();
    data += "&comment=";
    data += CGI::encodeURL(comment).toLatin1();
    data += "&code=";
    data += CGI::encodeURL(text).toLatin1();
    data += "&poster=";
    data += CGI::encodeURL(user).toLatin1();
//    qDebug("POST [%s]", data.constData());

    QHttp::post("/", data);
}

void Poster::gotRequestFinished(int, bool error)
{
    m_hadError = error;
    QCoreApplication::exit(error ? -1 : 0); // ends event-loop
}

void Poster::gotResponseHeaderReceived(const QHttpResponseHeader &resp)
{
    m_url = resp.value("location");
}
