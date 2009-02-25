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

#include "fetcher.h"
#include "cgi.h"

#include <QCoreApplication>
#include <QByteArray>
#include <QDebug>

Fetcher::Fetcher(const QString &host)
    : QHttp(host)
{
    m_host = host;
    m_status = 0;
    m_hadError = false;
    connect(this, SIGNAL(requestFinished(int,bool)), SLOT(gotRequestFinished(int,bool)));
    connect(this, SIGNAL(readyRead(QHttpResponseHeader)), SLOT(gotReadyRead(QHttpResponseHeader)));
}

int Fetcher::fetch(const QString &url)
{
//    qDebug("Fetcher::fetch(%s)", qPrintable(url));
    return QHttp::get(url);
}

int Fetcher::fetch(int pasteID)
{
    return fetch("http://" + m_host + "/?format=raw&id=" + QString::number(pasteID));
}

void Fetcher::gotRequestFinished(int, bool error)
{
    m_hadError = error;
    QCoreApplication::exit(error ? -1 : 0); // ends event-loop
}

void Fetcher::gotReadyRead(const QHttpResponseHeader & /* resp */)
{
    m_body += QHttp::readAll();

    // Hackish check for No Such Paste, as codepaster doesn't send a HTTP code indicating such, or
    // sends a redirect to an url indicating failure...
    if (m_body.contains("<B>No such paste!</B>")) {
        m_body.clear();
        m_status = -1;
        m_hadError = true;
    }
}
