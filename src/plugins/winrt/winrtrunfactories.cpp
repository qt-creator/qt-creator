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

#include "winrtrunfactories.h"
#include "winrtrunconfiguration.h"
#include "winrtruncontrol.h"
#include "winrtconstants.h"
#include "winrtdebugsupport.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using QmakeProjectManager::QmakeProject;
using QmakeProjectManager::QmakeProFileNode;

namespace WinRt {
namespace Internal {

static bool isKitCompatible(Kit *kit)
{
    IDevice::ConstPtr device = DeviceKitInformation::device(kit);
    if (!device)
        return false;
    if (device->type() == Constants::WINRT_DEVICE_TYPE_LOCAL
            || device->type() == Constants::WINRT_DEVICE_TYPE_PHONE
            || device->type() == Constants::WINRT_DEVICE_TYPE_EMULATOR)
        return true;
    return false;
}

WinRtRunConfigurationFactory::WinRtRunConfigurationFactory()
{
}

QList<Core::Id> WinRtRunConfigurationFactory::availableCreationIds(Target *parent,
                                                                   CreationMode mode) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();

    QmakeProject *project = static_cast<QmakeProject *>(parent->project());
    QList<QmakeProFileNode *> nodes = project->applicationProFiles();
    if (mode == AutoCreate)
        nodes = QmakeProject::nodesWithQtcRunnable(nodes);
    return QmakeProject::idsForNodes(Core::Id(Constants::WINRT_RC_PREFIX), nodes);
}

QString WinRtRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    Q_UNUSED(id);
    return tr("Run App Package");
}

bool WinRtRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    Q_UNUSED(id);
    return canHandle(parent);
}

RunConfiguration *WinRtRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    return new WinRtRunConfiguration(parent, id);
}

bool WinRtRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;

    return idFromMap(map).toString().startsWith(QLatin1String(Constants::WINRT_RC_PREFIX));
}

RunConfiguration *WinRtRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    RunConfiguration *config = new WinRtRunConfiguration(parent, idFromMap(map));
    config->fromMap(map);
    return config;
}

bool WinRtRunConfigurationFactory::canClone(Target *parent, RunConfiguration *product) const
{
    Q_UNUSED(parent);
    Q_UNUSED(product);
    return false;
}

RunConfiguration *WinRtRunConfigurationFactory::clone(Target *parent, RunConfiguration *product)
{
    Q_UNUSED(parent);
    Q_UNUSED(product);
    return 0;
}

bool WinRtRunConfigurationFactory::canHandle(Target *parent) const
{
    if (!isKitCompatible(parent->kit()))
        return false;
    if (!qobject_cast<QmakeProject *>(parent->project()))
        return false;
    return true;
}

WinRtRunControlFactory::WinRtRunControlFactory()
{
}

bool WinRtRunControlFactory::canRun(RunConfiguration *runConfiguration,
        Core::Id mode) const
{
    if (!runConfiguration)
        return false;
    IDevice::ConstPtr device = DeviceKitInformation::device(runConfiguration->target()->kit());
    if (!device)
        return false;

    if (mode == ProjectExplorer::Constants::DEBUG_RUN_MODE
            || mode == ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN) {
        if (device->type() != Constants::WINRT_DEVICE_TYPE_LOCAL)
            return false;
        return qobject_cast<WinRtRunConfiguration *>(runConfiguration);
    }

    if (mode == ProjectExplorer::Constants::NORMAL_RUN_MODE)
        return qobject_cast<WinRtRunConfiguration *>(runConfiguration);

    return false;
}

RunControl *WinRtRunControlFactory::create(
        RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage)
{
    WinRtRunConfiguration *rc = qobject_cast<WinRtRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return 0);

    if (mode == ProjectExplorer::Constants::NORMAL_RUN_MODE)
        return new WinRtRunControl(rc, mode);

    if (mode == ProjectExplorer::Constants::DEBUG_RUN_MODE || mode == ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN)
        return WinRtDebugSupport::createDebugRunControl(rc, mode, errorMessage);

    *errorMessage = tr("Unsupported run mode %1.").arg(mode.toString());
    return 0;
}

QString WinRtRunControlFactory::displayName() const
{
    return tr("WinRT Run Control Factory");
}

} // namespace Internal
} // namespace WinRt
