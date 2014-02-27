/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "winrtrunfactories.h"
#include "winrtrunconfiguration.h"
#include "winrtruncontrol.h"
#include "winrtconstants.h"

#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>

#include <debugger/debuggerrunner.h>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace WinRt {
namespace Internal {

static const char winrtConfigurationIdC[] = "WinRTConfigurationID";

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

QList<Core::Id> WinRtRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> result;
    if (isKitCompatible(parent->kit()))
        result.append(Core::Id(winrtConfigurationIdC));
    return result;
}

QString WinRtRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    Q_UNUSED(id);
    return tr("Run App Package");
}

bool WinRtRunConfigurationFactory::canCreate(Target *parent, const Core::Id id) const
{
    Q_UNUSED(parent);
    return id == winrtConfigurationIdC && isKitCompatible(parent->kit());
}

RunConfiguration *WinRtRunConfigurationFactory::doCreate(Target *parent, const Core::Id id)
{
    return new WinRtRunConfiguration(parent, id);
}

bool WinRtRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    Q_UNUSED(parent);
    return ProjectExplorer::idFromMap(map) == winrtConfigurationIdC && isKitCompatible(parent->kit());
}

RunConfiguration *WinRtRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    RunConfiguration *config = new WinRtRunConfiguration(parent, winrtConfigurationIdC);
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

WinRtRunControlFactory::WinRtRunControlFactory()
{
}

bool WinRtRunControlFactory::canRun(ProjectExplorer::RunConfiguration *runConfiguration,
        ProjectExplorer::RunMode mode) const
{
    if (mode != NormalRunMode)
        return false;
    return qobject_cast<WinRtRunConfiguration *>(runConfiguration);
}

ProjectExplorer::RunControl *WinRtRunControlFactory::create(
        RunConfiguration *runConfiguration, RunMode mode, QString *errorMessage)
{
    WinRtRunConfiguration *rc = qobject_cast<WinRtRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return 0);

    switch (mode) {
    case NormalRunMode:
        return new WinRtRunControl(rc, mode);
    default:
        break;
    }
    *errorMessage = tr("Unsupported run mode %1.").arg(mode);
    return 0;
}

QString WinRtRunControlFactory::displayName() const
{
    return tr("WinRT Run Control Factory");
}

} // namespace Internal
} // namespace WinRt
