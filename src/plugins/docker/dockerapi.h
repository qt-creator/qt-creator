// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dockersettings.h"

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/result.h>
#include <utils/synchronizedvalue.h>

#include <QFuture>
#include <QMutex>
#include <QObject>

#include <QtTaskTree/QParallelTaskTreeRunner>

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
    explicit DockerApi(ContainerToolSettings *settings);
    ~DockerApi() override;

    static DockerApi *instance();
    static DockerApi *instance(Utils::Id typeId);

    bool canConnect();
    void checkCanConnect(bool async = true);
    static void recheckDockerDaemon();
    static void recheckDaemon(Utils::Id typeId);

    Utils::Result<QList<Network>> networks() const { return m_networks; }
    void refreshNetworks();

    bool isContainerRunning(const QString &containerId);
    bool imageExists(const QString &imageId);

signals:
    void dockerDaemonAvailableChanged();
    void networksChanged();

public:
    std::optional<bool> dockerDaemonAvailable(bool async = true);
    static std::optional<bool> isDockerDaemonAvailable(bool async = true);
    static std::optional<bool> isDockerDaemonAvailable(Utils::Id typeId, bool async = true);

    QString displayType() const;

private:
    Utils::FilePath dockerClient();

    ContainerToolSettings *m_settings = nullptr;
    std::optional<bool> m_dockerDaemonAvailable;
    QMutex m_daemonCheckGuard;

    Utils::SynchronizedValue<Utils::FilePath> m_dockerClientBinary;
    Utils::Result<QList<Network>> m_networks;
    QtTaskTree::QParallelTaskTreeRunner m_taskTreeRunner;
};

} // namespace Docker::Internal
