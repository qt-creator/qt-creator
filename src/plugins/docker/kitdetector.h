// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicefwd.h>
#include <utils/filepath.h>

#include <QObject>

namespace Docker::Internal {

class KitDetector : public QObject
{
    Q_OBJECT

public:
    explicit KitDetector(const ProjectExplorer::IDeviceConstPtr &device);
    ~KitDetector() override;

    void autoDetect(const QString &sharedId, const Utils::FilePaths &selectedPaths) const;
    void undoAutoDetect(const QString &sharedId) const;
    void listAutoDetected(const QString &sharedId) const;

signals:
    void logOutput(const QString &msg);

private:
    class KitDetectorPrivate *d = nullptr;
};

} // Docker::Internal
