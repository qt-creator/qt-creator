// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QContiguousCache>

namespace Qdb::Internal {

class QdbWatcher;

class QdbMessageTracker : public QObject
{
    Q_OBJECT
public:
    QdbMessageTracker(QObject *parent = nullptr);

    void start();
    void stop();

signals:
    void trackerError(QString errorMessage);

private:
    void handleWatchMessage(const QJsonDocument &document);

    QdbWatcher *m_qdbWatcher = nullptr;
    QContiguousCache<QString> m_messageCache = QContiguousCache<QString>(10);
};

} // Qdb::Internal
