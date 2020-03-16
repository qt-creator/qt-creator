/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef ANDROIDSDKDOWNLOADER_H
#define ANDROIDSDKDOWNLOADER_H

#include "androidconfigurations.h"

#include <QNetworkReply>
#include <QObject>
#include <QProgressDialog>

namespace Android {
namespace Internal {

class AndroidSdkDownloader : public QObject
{
    Q_OBJECT

public:
    AndroidSdkDownloader();
    void downloadAndExtractSdk(const QString &jdkPath, const QString &sdkExtractPath);
    static QString dialogTitle();

    void cancel();

signals:
    void sdkPackageWriteFinished();
    void sdkExtracted();
    void sdkDownloaderError(const QString &error);

private:
    static QString getSaveFilename(const QUrl &url);
    bool saveToDisk(const QString &filename, QIODevice *data);
    static bool isHttpRedirect(QNetworkReply *m_reply);

    bool extractSdk(const QString &jdkPath, const QString &sdkExtractPath);
    bool verifyFileIntegrity();
    void cancelWithError(const QString &error);
    void logError(const QString &error);

    void downloadFinished(QNetworkReply *m_reply);
#if QT_CONFIG(ssl)
    void sslErrors(const QList<QSslError> &errors);
#endif

    QNetworkAccessManager m_manager;
    QNetworkReply *m_reply = nullptr;
    QString m_sdkFilename;
    QProgressDialog *m_progressDialog = nullptr;
    AndroidConfig m_androidConfig;
};

} // Internal
} // Android

#endif // ANDROIDSDKDOWNLOADER_H
