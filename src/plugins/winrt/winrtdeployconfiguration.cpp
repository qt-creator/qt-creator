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

#include "winrtdeployconfiguration.h"
#include "winrtpackagedeploymentstep.h"
#include "winrtconstants.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtkitinformation.h>

#include <QCoreApplication>

using namespace ProjectExplorer;

namespace WinRt {
namespace Internal {

static const char appxDeployConfigurationC[] = "WinRTAppxDeployConfiguration";
static const char phoneDeployConfigurationC[] = "WinRTPhoneDeployConfiguration";
static const char emulatorDeployConfigurationC[] = "WinRTEmulatorDeployConfiguration";

static QString msgDeployConfigurationDisplayName(Core::Id id)
{
    if (id == appxDeployConfigurationC) {
        return QCoreApplication::translate("WinRt::Internal::WinRtDeployConfiguration",
                                           "Run windeployqt");
    }
    if (id == phoneDeployConfigurationC) {
        return QCoreApplication::translate("WinRt::Internal::WinRtDeployConfiguration",
                                           "Deploy to Windows Phone");
    }
    if (id == emulatorDeployConfigurationC) {
        return QCoreApplication::translate("WinRt::Internal::WinRtDeployConfiguration",
                                           "Deploy to Windows Phone Emulator");
    }
    return QString();
}

WinRtDeployConfigurationFactory::WinRtDeployConfigurationFactory(QObject *parent)
    : DeployConfigurationFactory(parent)
{
}

QString WinRtDeployConfigurationFactory::displayNameForId(Core::Id id) const
{
    return msgDeployConfigurationDisplayName(id);
}

QList<Core::Id> WinRtDeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    if (!parent->project()->supportsKit(parent->kit()))
        return QList<Core::Id>();

    IDevice::ConstPtr device = DeviceKitInformation::device(parent->kit());
    if (!device)
        return QList<Core::Id>();

    if (device->type() == Constants::WINRT_DEVICE_TYPE_LOCAL)
        return QList<Core::Id>() << Core::Id(appxDeployConfigurationC);

    if (device->type() == Constants::WINRT_DEVICE_TYPE_PHONE)
        return QList<Core::Id>() << Core::Id(phoneDeployConfigurationC);

    if (device->type() == Constants::WINRT_DEVICE_TYPE_EMULATOR)
        return QList<Core::Id>() << Core::Id(emulatorDeployConfigurationC);

    return QList<Core::Id>();
}

bool WinRtDeployConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

DeployConfiguration *WinRtDeployConfigurationFactory::create(Target *parent, Core::Id id)
{
    if (id == appxDeployConfigurationC
            || id == phoneDeployConfigurationC
            || id == emulatorDeployConfigurationC) {
        return new WinRtDeployConfiguration(parent, id);
    }
    return 0;
}

bool WinRtDeployConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

DeployConfiguration *WinRtDeployConfigurationFactory::restore(Target *parent,
                                                              const QVariantMap &map)
{
    WinRtDeployConfiguration *dc = new WinRtDeployConfiguration(parent, idFromMap(map));
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

bool WinRtDeployConfigurationFactory::canClone(Target *parent, DeployConfiguration *source) const
{
    return availableCreationIds(parent).contains(source->id());
}

DeployConfiguration *WinRtDeployConfigurationFactory::clone(Target *parent,
                                                            DeployConfiguration *source)
{
    if (source->id() == appxDeployConfigurationC
            || source->id() == phoneDeployConfigurationC
            || source->id() == emulatorDeployConfigurationC) {
        return new WinRtDeployConfiguration(parent, source);
    }
    return 0;
}

QList<BuildStepInfo> WinRtDeployStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_DEPLOY
            || !parent->target()->project()->supportsKit(parent->target()->kit())
            || parent->contains(Constants::WINRT_BUILD_STEP_DEPLOY))
        return {};

    return {{ Constants::WINRT_BUILD_STEP_DEPLOY,
              QCoreApplication::translate("WinRt::Internal::WinRtDeployStepFactory", "Run windeployqt"),
              BuildStepInfo::Unclonable }};
}

BuildStep *WinRtDeployStepFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_UNUSED(id)
    return new WinRtPackageDeploymentStep(parent);
}

BuildStep *WinRtDeployStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    Q_UNUSED(parent);
    Q_UNUSED(source);
    return 0;
}

WinRtDeployConfiguration::WinRtDeployConfiguration(Target *target, Core::Id id)
    : DeployConfiguration(target, id)
{
    setDefaultDisplayName(msgDeployConfigurationDisplayName(id));

    stepList()->insertStep(0, new WinRtPackageDeploymentStep(stepList()));
}

WinRtDeployConfiguration::WinRtDeployConfiguration(Target *target, DeployConfiguration *source)
    : DeployConfiguration(target, source)
{
    cloneSteps(source);
}

} // namespace Internal
} // namespace WinRt
