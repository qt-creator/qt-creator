/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "appmanagerrunconfiguration.h"
#include "appmanagerproject.h"

#include "../appmanagerconstants.h"

#include <projectexplorer/runnables.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/kitinformation.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtsupportconstants.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>

#include <utils/environment.h>

#include <QDir>
#include <QFileInfo>
#include <QVariantMap>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager {

AppManagerRunConfiguration::AppManagerRunConfiguration(ProjectExplorer::Target *parent, Core::Id id)
    : RunConfiguration(parent, id)
{
    setDisplayName(target()->project()->displayName());
}

QWidget *AppManagerRunConfiguration::createConfigurationWidget()
{
    return nullptr;
}

ProjectExplorer::Runnable AppManagerRunConfiguration::runnable() const
{
    StandardRunnable result;

    auto kit = target()->kit();
    auto project = target()->project();

    if (auto version = QtSupport::QtKitInformation::qtVersion(kit))
        if (auto dev = DeviceKitInformation::device(kit))
            if (dev->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
                result.executable = version->qmakeProperty("QT_INSTALL_BINS") + "/";

    result.executable += Constants::C_APPMANAGER_COMMAND;
    result.workingDirectory = QStringLiteral("/tmp/deploy-") + project->displayName();
    result.commandLineArguments = Constants::C_APPMANAGER_COMMAND_BASE_ARGUMENTS + " " + result.workingDirectory + "/appman-test.qml";
    return result;
}

QVariantMap AppManagerRunConfiguration::toMap() const
{
    QVariantMap map = RunConfiguration::toMap();
    map[Constants::C_APPMANAGERRUNCONFIGURATION_CODE_KEY] = QStringLiteral("appman-test.qml");
    return map;
}

bool AppManagerRunConfiguration::fromMap(const QVariantMap &map)
{
    return RunConfiguration::fromMap(map);
}

} // namespace AppManager
