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

#include "winrtrunconfiguration.h"
#include "winrtconstants.h"

#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

using namespace ProjectExplorer;

namespace WinRt {
namespace Internal {

// UninstallAfterStopAspect

UninstallAfterStopAspect::UninstallAfterStopAspect()
    : BaseBoolAspect("WinRtRunConfigurationUninstallAfterStopId")
{
    setLabel(WinRtRunConfiguration::tr("Uninstall package after application stops"),
             LabelPlacement::AtCheckBox);
}

// LoopbackExemptClientAspect

LoopbackExemptClientAspect::LoopbackExemptClientAspect()
    : BaseBoolAspect("WinRtRunConfigurationLoopbackExemptClient")
{
    setLabel(WinRtRunConfiguration::tr("Enable localhost communication for clients"),
             LabelPlacement::AtCheckBox);
}

// LoopbackExemptServerAspect

LoopbackExemptServerAspect::LoopbackExemptServerAspect()
    : BaseBoolAspect("WinRtRunConfigurationLoopbackExemptServer")
{
    setLabel(WinRtRunConfiguration::tr("Enable localhost communication for "
                                       "servers (requires elevated rights)"),
             LabelPlacement::AtCheckBox);
}

// WinRtRunConfiguration

WinRtRunConfiguration::WinRtRunConfiguration(Target *target, Utils::Id id)
    : RunConfiguration(target, id)
{
    setDisplayName(tr("Run App Package"));
    addAspect<ArgumentsAspect>();
    addAspect<UninstallAfterStopAspect>();

    const QtSupport::BaseQtVersion *qt
            = QtSupport::QtKitAspect::qtVersion(target->kit());
    if (qt && qt->qtVersion() >= QtSupport::QtVersionNumber(5, 12, 0)) {
        addAspect<LoopbackExemptClientAspect>();
        addAspect<LoopbackExemptServerAspect>();
    }
}

// WinRtRunConfigurationFactory

WinRtRunConfigurationFactory::WinRtRunConfigurationFactory()
{
    registerRunConfiguration<WinRtRunConfiguration>("WinRt.WinRtRunConfiguration:");
    addSupportedTargetDeviceType(Constants::WINRT_DEVICE_TYPE_LOCAL);
    addSupportedTargetDeviceType(Constants::WINRT_DEVICE_TYPE_PHONE);
    addSupportedTargetDeviceType(Constants::WINRT_DEVICE_TYPE_EMULATOR);
}

} // namespace Internal
} // namespace WinRt
