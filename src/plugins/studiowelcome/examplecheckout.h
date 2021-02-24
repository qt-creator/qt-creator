/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#pragma once

#include <utils/fileutils.h>

#include <QObject>
#include <QTimer>
#include <QtQml>

#include <memory>

QT_BEGIN_NAMESPACE
class QDialog;
QT_END_NAMESPACE

class ExampleCheckout : public QObject
{
    Q_OBJECT
public:
    explicit ExampleCheckout(QObject *parent = nullptr);

    Q_INVOKABLE void checkoutExample(const QUrl &url);

    QString extractionFolder() const;

    ~ExampleCheckout();

public slots:
    void handleCancel();
    void handleAccepted();

signals:
    void finishedSucessfully();

private:
    std::unique_ptr<QDialog> m_dialog;
    QObject *rootObject = nullptr;
    QString m_extrationFolder;
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
    Q_PROPERTY(QString tempFile READ tempFile NOTIFY finishedChanged)

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

    Q_INVOKABLE void start();

signals:
    void finishedChanged();
    void errorChanged();
    void nameChanged();
    void progressChanged();
    void downloadFailed();

private:
    QUrl m_url;
    bool m_finished = false;
    bool m_error = false;
    int m_progress = 0;
    QFile m_tempFile;
};
