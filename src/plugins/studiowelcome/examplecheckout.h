// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>

#include <QObject>
#include <QTimer>
#include <QtQml>

#include <memory>

struct ExampleCheckout
{
    static void registerTypes();
};

class FileExtractor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString targetPath READ targetPath NOTIFY targetPathChanged)
    Q_PROPERTY(QString archiveName READ archiveName WRITE setArchiveName)
    Q_PROPERTY(QString detailedText READ detailedText NOTIFY detailedTextChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(QString size READ size NOTIFY sizeChanged)
    Q_PROPERTY(QString count READ count NOTIFY sizeChanged)
    Q_PROPERTY(QString sourceFile READ sourceFile WRITE setSourceFile)
    Q_PROPERTY(bool finished READ finished NOTIFY finishedChanged)
    Q_PROPERTY(bool targetFolderExists READ targetFolderExists NOTIFY targetFolderExistsChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QDateTime birthTime READ birthTime NOTIFY birthTimeChanged)

public:
    explicit FileExtractor(QObject *parent = nullptr);
    ~FileExtractor();

    static void registerQmlType();

    QString targetPath() const;
    void setSourceFile(QString &sourceFilePath);
    void setArchiveName(QString &filePath);
    const QString detailedText();
    bool finished() const;
    QString currentFile() const;
    QString size() const;
    QString count() const;
    bool targetFolderExists() const;
    int progress() const;
    QDateTime birthTime() const;

    QString sourceFile() const;
    QString archiveName() const;

    Q_INVOKABLE void browse();
    Q_INVOKABLE void extract();

signals:
    void targetPathChanged();
    void detailedTextChanged();
    void finishedChanged();
    void currentFileChanged();
    void sizeChanged();
    void targetFolderExistsChanged();
    void progressChanged();
    void birthTimeChanged();

private:
    Utils::FilePath m_targetPath;
    Utils::FilePath m_sourceFile;
    QString m_detailedText;
    bool m_finished = false;
    QTimer m_timer;
    QString m_currentFile;
    QString m_size;
    QString m_count;
    QString m_archiveName;
    int m_progress = 0;
    QDateTime m_birthTime;
};

class FileDownloader : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl url READ url WRITE setUrl)
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
    static void registerQmlType();
    QString name() const;
    QString completeBaseName() const;
    int progress() const;
    QString tempFile() const;
    QDateTime lastModified() const;
    bool available() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void cancel();

signals:
    void finishedChanged();
    void errorChanged();
    void nameChanged();
    void progressChanged();
    void tempFileChanged();
    void downloadFailed();
    void lastModifiedChanged();
    void availableChanged();

    void downloadCanceled();

private:
    void probeUrl();

    QUrl m_url;
    bool m_finished = false;
    bool m_error = false;
    int m_progress = 0;
    QFile m_tempFile;
    QDateTime m_lastModified;
    bool m_available = false;

    QNetworkReply *m_reply = nullptr;
};

class DataModelDownloader : public QObject
{
    Q_OBJECT

public:
    explicit DataModelDownloader(QObject *parent = nullptr);
    bool start();
    bool exists() const;
    bool available() const;
    Utils::FilePath targetFolder() const;
    void setForceDownload(bool b);
    int progress() const;

signals:
    void finished();
    void availableChanged();
    void progressChanged();
    void downloadFailed();

private:
    FileDownloader m_fileDownloader;
    QDateTime m_birthTime;
    bool m_exists = false;
    bool m_available = false;
    bool m_forceDownload = false;
};
