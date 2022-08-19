// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "dockersettings.h"

#include <utils/filepath.h>
#include <utils/guard.h>
#include <utils/optional.h>

#include <QMutex>
#include <QObject>

namespace Docker::Internal {

class DockerApi : public QObject
{
    Q_OBJECT

public:
    DockerApi(DockerSettings *settings);

    static DockerApi *instance();

    bool canConnect();
    void checkCanConnect(bool async = true);
    static void recheckDockerDaemon();

signals:
    void dockerDaemonAvailableChanged();

public:
    Utils::optional<bool> dockerDaemonAvailable(bool async = true);
    static Utils::optional<bool> isDockerDaemonAvailable(bool async = true);

private:
    Utils::FilePath dockerClient();

    Utils::optional<bool> m_dockerDaemonAvailable;
    QMutex m_daemonCheckGuard;
    DockerSettings *m_settings;
};

} // Docker::Internal
