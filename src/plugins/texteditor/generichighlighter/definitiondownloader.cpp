/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
#include <QRegExp>

#include <utils/networkaccessmanager.h>

namespace TextEditor {
namespace Internal {

static QNetworkReply *getData(const QUrl &url)
{
    Utils::NetworkAccessManager *manager = Utils::NetworkAccessManager::instance();

    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);

    QEventLoop eventLoop;
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    return reply;
}

DefinitionDownloader::DefinitionDownloader(const QUrl &url, const QString &localPath) :
    m_url(url), m_localPath(localPath), m_status(Unknown)
{}

void DefinitionDownloader::run()
{
    int currentAttempt = 0;
    const int maxAttempts = 5;
    while (currentAttempt < maxAttempts) {
        QScopedPointer<QNetworkReply> reply(getData(m_url));
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

void DefinitionDownloader::saveData(QNetworkReply *reply)
{
    const QString &urlPath = m_url.path();
    const QString &fileName =
        urlPath.right(urlPath.length() - urlPath.lastIndexOf(QLatin1Char('/')) - 1);
    Utils::FileSaver saver(m_localPath + fileName, QIODevice::Text);
    const QByteArray data = reply->readAll();
    saver.write(data);
    m_status = saver.finalize() ? Ok: WriteError;
    QString content = QString::fromUtf8(data);
    QRegExp reference(QLatin1String("context\\s*=\\s*\"[^\"]*##([^\"]+)\""));
    int index = -1;
    forever {
        index = reference.indexIn(content, index + 1);
        if (index == -1)
            break;
        emit foundReferencedDefinition(reference.cap(1));
    }
}

DefinitionDownloader::Status DefinitionDownloader::status() const
{ return m_status; }

} // namespace Internal
} // namespace TextEditor
