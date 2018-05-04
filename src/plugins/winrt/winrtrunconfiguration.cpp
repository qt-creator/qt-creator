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

#include <coreplugin/icore.h>

#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/detailswidget.h>

#include <QFormLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace WinRt {
namespace Internal {

// UninstallAfterStopAspect

UninstallAfterStopAspect::UninstallAfterStopAspect(RunConfiguration *rc)
    : BaseBoolAspect(rc, "WinRtRunConfigurationUninstallAfterStopId")
{
    setLabel(WinRtRunConfiguration::tr("Uninstall package after application stops"));
}

// WinRtRunConfiguration

WinRtRunConfiguration::WinRtRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    setDisplayName(tr("Run App Package"));
    addExtraAspect(new ArgumentsAspect(this, "WinRtRunConfigurationArgumentsId"));
    addExtraAspect(new UninstallAfterStopAspect(this));
}

QWidget *WinRtRunConfiguration::createConfigurationWidget()
{
    auto widget = new QWidget;
    auto fl = new QFormLayout(widget);

    extraAspect<ArgumentsAspect>()->addToConfigurationLayout(fl);
    extraAspect<UninstallAfterStopAspect>()->addToConfigurationLayout(fl);

    auto wrapped = wrapWidget(widget);
    auto detailsWidget = qobject_cast<DetailsWidget *>(wrapped);
    QTC_ASSERT(detailsWidget, return wrapped);
    detailsWidget->setState(DetailsWidget::Expanded);
    detailsWidget->setSummaryText(tr("Launch App"));
    return detailsWidget;
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
