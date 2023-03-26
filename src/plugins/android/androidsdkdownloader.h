// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ANDROIDSDKDOWNLOADER_H
#define ANDROIDSDKDOWNLOADER_H

#include "androidconfigurations.h"

#include <QNetworkReply>
#include <QObject>
#include <QProgressDialog>

namespace Utils {
class Archive;
class FilePath;
}

namespace Android {
namespace Internal {

class AndroidSdkDownloader : public QObject
{
    Q_OBJECT

public:
    AndroidSdkDownloader();
    ~AndroidSdkDownloader();
    void downloadAndExtractSdk();
    static QString dialogTitle();

    void cancel();

signals:
    void sdkPackageWriteFinished();
    void sdkExtracted();
    void sdkDownloaderError(const QString &error);

private:
    static Utils::FilePath getSaveFilename(const QUrl &url);
    bool saveToDisk(const Utils::FilePath &filename, QIODevice *data);
    static bool isHttpRedirect(QNetworkReply *m_reply);

    bool verifyFileIntegrity();
    void cancelWithError(const QString &error);
    void logError(const QString &error);

    void downloadFinished(QNetworkReply *m_reply);
#if QT_CONFIG(ssl)
    void sslErrors(const QList<QSslError> &errors);
#endif

    QNetworkAccessManager m_manager;
    QNetworkReply *m_reply = nullptr;
    Utils::FilePath m_sdkFilename;
    QProgressDialog *m_progressDialog = nullptr;
    AndroidConfig &m_androidConfig;
    std::unique_ptr<Utils::Archive> m_archive;
};

} // Internal
} // Android

#endif // ANDROIDSDKDOWNLOADER_H
