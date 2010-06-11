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

#include "definitiondownloader.h"

#include <QtCore/QLatin1Char>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

using namespace TextEditor;
using namespace Internal;

DefinitionDownloader::DefinitionDownloader(const QUrl &url, const QString &localPath) :
    m_url(url), m_localPath(localPath)
{}

void DefinitionDownloader::start()
{
    QNetworkReply *reply;
    QNetworkAccessManager manager;

    int currentAttempt = 0;
    const int maxAttempts = 5;
    while (currentAttempt < maxAttempts) {
        reply = getData(&manager);
        if (reply->error() != QNetworkReply::NoError)
            break;

        ++currentAttempt;
        QVariant variant = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (variant.isValid() && currentAttempt < maxAttempts) {
            m_url = variant.toUrl();
            delete reply;
        } else if (!variant.isValid()) {
            saveData(reply);
            break;
        }
    }

    delete reply;
}

QNetworkReply *DefinitionDownloader::getData(QNetworkAccessManager *manager) const
{
    QNetworkRequest request(m_url);
    QNetworkReply *reply = manager->get(request);

    QEventLoop eventLoop;
    connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    eventLoop.exec();

    return reply;
}

void DefinitionDownloader::saveData(QNetworkReply *reply) const
{
    const QString &urlPath = m_url.path();
    const QString &fileName =
        urlPath.right(urlPath.length() - urlPath.lastIndexOf(QLatin1Char('/')) - 1);
    QFile file(m_localPath + fileName);
    if (!file.open(QIODevice::Text | QIODevice::WriteOnly))
        return;
    file.write(reply->readAll());
    file.close();
}
