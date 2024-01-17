// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerutilities.h"

#include "appmanagerconstants.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/hostosinfo.h>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace AppManager::Internal {

static FilePath getToolPathByQtVersion(const QtVersion *qtVersion,
                                       const QString &toolname = QString(Constants::APPMAN_PACKAGER))
{
    if (qtVersion) {
        const auto toolExistsInDir = [&toolname](const FilePath &dir) {
            return dir.pathAppended(getToolNameByDevice(toolname)).isFile();
        };

        const FilePath qtHostBinsDir = qtVersion->hostBinPath();
        if (toolExistsInDir(qtHostBinsDir))
            return qtHostBinsDir;

        const FilePath qtBinDir = qtVersion->binPath();
        if (toolExistsInDir(qtBinDir))
            return qtBinDir;
    }
    return {};
}

QString getToolFilePath(const QString &toolname, const Kit *kit, const IDevice::ConstPtr &device)
{
    const bool local = !device || device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
    const FilePath path = local ? getToolPathByQtVersion(QtKitAspect::qtVersion(kit))
                                : FilePath(Constants::REMOTE_DEFAULT_BIN_PATH);
    const QString name = getToolNameByDevice(toolname, device);
    return !path.isEmpty() ? path.pathAppended(name).toString() : name;
}

QString getToolNameByDevice(const QString &baseName, const QSharedPointer<const IDevice> &device)
{
    return OsSpecificAspects::withExecutableSuffix(device ? device->osType() : HostOsInfo::hostOs(), baseName);
}

} // AppManager::Internal
