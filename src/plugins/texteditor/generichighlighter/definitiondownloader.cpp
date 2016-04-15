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

static QNetworkReply *getData(QNetworkAccessManager *manager, const QUrl &url)
{
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
    Utils::NetworkAccessManager manager;

    int currentAttempt = 0;
    const int maxAttempts = 5;
    while (currentAttempt < maxAttempts) {
        QScopedPointer<QNetworkReply> reply(getData(&manager, m_url));
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
