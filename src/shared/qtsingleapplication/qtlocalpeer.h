// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once


#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QScopedPointer>

namespace SharedTools {

class QtLocalPeer : public QObject
{
    Q_OBJECT

public:
    explicit QtLocalPeer(QObject *parent = 0, const QString &appId = QString());
    bool isClient();
    bool sendMessage(const QString &message, int timeout, bool block);
    QString applicationId() const
        { return id; }
    static QString appSessionId(const QString &appId);

Q_SIGNALS:
    void messageReceived(const QString &message, QObject *socket);

protected:
    void receiveConnection();

    QString id;
    QString socketName;
    QLocalServer* server{nullptr};
    QScopedPointer<QLockFile> lockFile;
};

} // namespace SharedTools
