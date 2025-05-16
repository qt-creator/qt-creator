// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/guard.h>
#include <utils/result.h>

#include <QFuture>
#include <QMutex>
#include <QObject>

#include <optional>

namespace Docker::Internal {

struct Network
{
    QString id;
    QString name;
    QString driver;
    QString scope;
    bool internal;
    bool ipv6;
    QDateTime createdAt;
    QString labels;

    QString toString() const;
};

class DockerApi : public QObject
{
    Q_OBJECT

public:
    DockerApi();

    static DockerApi *instance();

    bool canConnect();
    void checkCanConnect(bool async = true);
    static void recheckDockerDaemon();
    QFuture<Utils::Result<QList<Network>>> networks();

    bool isContainerRunning(const QString &containerId);
    bool imageExists(const QString &imageId);

signals:
    void dockerDaemonAvailableChanged();

public:
    std::optional<bool> dockerDaemonAvailable(bool async = true);
    static std::optional<bool> isDockerDaemonAvailable(bool async = true);

private:
    Utils::FilePath dockerClient();

    std::optional<bool> m_dockerDaemonAvailable;
    QMutex m_daemonCheckGuard;
};

} // Docker::Internal
