/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "definitiondownloader.h"

#include <utils/fileutils.h>

#include <QLatin1Char>
#include <QEventLoop>
#include <QScopedPointer>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <utils/networkaccessmanager.h>

using namespace TextEditor;
using namespace Internal;

DefinitionDownloader::DefinitionDownloader(const QUrl &url, const QString &localPath) :
    m_url(url), m_localPath(localPath), m_status(Unknown)
{}

void DefinitionDownloader::run()
{
    Utils::NetworkAccessManager *manager = Utils::NetworkAccessManager::instance();

    int currentAttempt = 0;
    const int maxAttempts = 5;
    while (currentAttempt < maxAttempts) {
        QScopedPointer<QNetworkReply> reply(getData(manager));
        if (reply->error() != QNetworkReply::NoError) {
            m_status = NetworkError;
            return;
        }

        ++currentAttempt;
        QVariant variant = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (variant.isValid() && currentAttempt < maxAttempts) {
            m_url = variant.toUrl();
        } else if (!variant.isValid()) {
            saveData(reply.data());
            return;
        }
    }
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

void DefinitionDownloader::saveData(QNetworkReply *reply)
{
    const QString &urlPath = m_url.path();
    const QString &fileName =
        urlPath.right(urlPath.length() - urlPath.lastIndexOf(QLatin1Char('/')) - 1);
    Utils::FileSaver saver(m_localPath + fileName, QIODevice::Text);
    saver.write(reply->readAll());
    m_status = saver.finalize() ? Ok: WriteError;
}

DefinitionDownloader::Status DefinitionDownloader::status() const
{ return m_status; }
