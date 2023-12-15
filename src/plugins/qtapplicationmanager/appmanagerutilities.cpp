// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerutilities.h"

#include "appmanagerconstants.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/hostosinfo.h>
#include <utils/osspecificaspects.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace AppManager {
namespace Internal {

static QString getToolPathByQtVersion(const QtVersion *qtVersion,
                                      const QString &toolname = QString(Constants::APPMAN_PACKAGER))
{
    if (qtVersion) {
        const auto toolExistsInDir = [&](const QDir &dir) {
            const FilePath toolFilePath = FilePath::fromString(dir.absolutePath())
                                              .pathAppended(getToolNameByDevice(toolname));
            return toolFilePath.isFile();
        };
        const QDir qtHostBinsDir(qtVersion->hostBinPath().toString());
        if (toolExistsInDir(qtHostBinsDir))
            return qtHostBinsDir.absolutePath();
        const QDir qtBinDir(qtVersion->binPath().toString());
        if (toolExistsInDir(qtBinDir))
            return qtBinDir.absolutePath();
    }
    return QString();
}

QString getToolFilePath(const QString &toolname, const Kit *kit, const IDevice::ConstPtr &device)
{
    const bool local = !device || device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
    const auto path = local ?
                getToolPathByQtVersion(QtKitAspect::qtVersion(kit)) :
                QString(Constants::REMOTE_DEFAULT_BIN_PATH);
    const auto name = getToolNameByDevice(toolname, device);
    return !path.isEmpty() ?
                QDir(path).absoluteFilePath(name) :
                name;
}

QString getToolNameByDevice(const QString &baseName, const QSharedPointer<const IDevice> &device)
{
    return OsSpecificAspects::withExecutableSuffix(device ? device->osType() : HostOsInfo::hostOs(), baseName);
}

} // namespace Internal
} // namespace AppManager
