// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <QObject>
#include <QUrl>

namespace QmlDesigner {

class FileDownloader;

class MultiFileDownloader : public QObject
{
    Q_OBJECT

    Q_PROPERTY(FileDownloader *downloader READ downloader WRITE setDownloader)
    Q_PROPERTY(bool finished READ finished NOTIFY finishedChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QUrl baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(QString targetDirPath READ targetDirPath WRITE setTargetDirPath NOTIFY targetDirPathChanged)

    Q_PROPERTY(QString nextUrl READ nextUrl NOTIFY nextUrlChanged)
    Q_PROPERTY(QString nextTargetPath READ nextTargetPath NOTIFY nextTargetPathChanged)
    Q_PROPERTY(QStringList files READ files WRITE setFiles NOTIFY filesChanged)

public:
    explicit MultiFileDownloader(QObject *parent = nullptr);

    ~MultiFileDownloader();

    void setBaseUrl(const QUrl &url);
    QUrl baseUrl() const;

    void setTargetDirPath(const QString &path);
    QString targetDirPath() const;
    void setDownloader(FileDownloader *downloader);
    FileDownloader *downloader();

    bool finished() const;
    int progress() const;

    QString nextUrl() const;
    QString nextTargetPath() const;

    void setFiles(const QStringList &files);
    QStringList files() const;
    void switchToNextFile();

    Q_INVOKABLE void start();
    Q_INVOKABLE void cancel();

signals:
    void finishedChanged();
    void baseUrlChanged();
    void progressChanged();
    void downloadFailed();

    void downloadStarting();
    void downloadCanceled();
    void targetDirPathChanged();
    void nextUrlChanged();
    void filesChanged();
    void nextTargetPathChanged();

private:
    QUrl m_baseUrl;
    bool m_finished = false;
    int m_progress = 0;
    QString m_targetDirPath;
    FileDownloader *m_downloader = nullptr;
    bool m_canceled = false;
    bool m_failed = false;
    QStringList m_files;
    int m_nextFile = 0;
};

} // namespace QmlDesigner

