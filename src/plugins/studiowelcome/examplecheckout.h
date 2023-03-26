// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>
#include <qmldesigner/utils/filedownloader.h>

#include <QObject>
#include <QTimer>
#include <QtQml>

#include <memory>

struct ExampleCheckout
{
    static void registerTypes();
};

class DataModelDownloader : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE void usageStatisticsDownloadExample(const QString &name);
    Q_INVOKABLE bool downloadEnabled() const;
    Q_INVOKABLE QString targetPath() const;

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
    void targetPathMustChange(const QString &newPath);

private:
    void onAvailableChanged();
    QmlDesigner::FileDownloader m_fileDownloader;
    QDateTime m_birthTime;
    bool m_exists = false;
    bool m_available = false;
    bool m_forceDownload = false;
    bool m_started = false;
};
