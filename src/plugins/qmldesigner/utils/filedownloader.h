// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <QFile>
#include <QNetworkReply>
#include <QObject>
#include <QUrl>

namespace QmlDesigner {

class FileDownloader : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool downloadEnabled WRITE setDownloadEnabled READ downloadEnabled NOTIFY downloadEnabledChanged)
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(bool probeUrl READ probeUrl WRITE setProbeUrl NOTIFY probeUrlChanged)
    Q_PROPERTY(bool finished READ finished NOTIFY finishedChanged)
    Q_PROPERTY(bool error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString completeBaseName READ completeBaseName NOTIFY nameChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString tempFile READ tempFile NOTIFY tempFileChanged)
    Q_PROPERTY(QDateTime lastModified READ lastModified NOTIFY lastModifiedChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)

public:
    explicit FileDownloader(QObject *parent = nullptr);

    ~FileDownloader();

    void setUrl(const QUrl &url);
    QUrl url() const;
    bool finished() const;
    bool error() const;
    QString name() const;
    QString completeBaseName() const;
    int progress() const;
    QString tempFile() const;
    QDateTime lastModified() const;
    bool available() const;
    void setDownloadEnabled(bool value);
    bool downloadEnabled() const;

    void setProbeUrl(bool value);
    bool probeUrl() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void cancel();

signals:
    void finishedChanged();
    void errorChanged();
    void nameChanged();
    void urlChanged();
    void progressChanged();
    void tempFileChanged();
    void downloadFailed();
    void lastModifiedChanged();
    void availableChanged();
    void downloadEnabledChanged();

    void downloadStarting();
    void downloadCanceled();
    void probeUrlChanged();

private:
    void doProbeUrl();

    QUrl m_url;
    bool m_probeUrl = false;
    bool m_finished = false;
    bool m_error = false;
    int m_progress = 0;
    QFile m_tempFile;
    QDateTime m_lastModified;
    bool m_available = false;

    QNetworkReply *m_reply = nullptr;
    bool m_downloadEnabled = false;
};

} // namespace QmlDesigner

