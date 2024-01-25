// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/project.h>

namespace AppManager::Internal {

QString getToolNameByDevice(const QString &baseName, const QSharedPointer<const ProjectExplorer::IDevice> &device = nullptr);
Utils::FilePath getToolFilePath(const QString &toolname, const ProjectExplorer::Kit *kit, const ProjectExplorer::IDevice::ConstPtr &device = nullptr);

} // AppManager::Internal
