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
#include "winrtrunconfigurationwidget.h"
#include "winrtconstants.h"

#include <coreplugin/icore.h>

#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <qmakeprojectmanager/qmakeproject.h>

namespace WinRt {
namespace Internal {

static const char uninstallAfterStopIdC[] = "WinRtRunConfigurationUninstallAfterStopId";

static QString pathFromId(Core::Id id)
{
    return id.suffixAfter(Constants::WINRT_RC_PREFIX);
}

WinRtRunConfiguration::WinRtRunConfiguration(ProjectExplorer::Target *target)
    : RunConfiguration(target)
{
    setDisplayName(tr("Run App Package"));
    addExtraAspect(new ProjectExplorer::ArgumentsAspect(this, "WinRtRunConfigurationArgumentsId"));
}

void WinRtRunConfiguration::initialize(Core::Id id)
{
    m_proFilePath = pathFromId(id);
}

QWidget *WinRtRunConfiguration::createConfigurationWidget()
{
    return new WinRtRunConfigurationWidget(this);
}

QVariantMap WinRtRunConfiguration::toMap() const
{
    QVariantMap map = RunConfiguration::toMap();
    map.insert(QLatin1String(uninstallAfterStopIdC), m_uninstallAfterStop);
    return map;
}

bool WinRtRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;
    setUninstallAfterStop(map.value(QLatin1String(uninstallAfterStopIdC)).toBool());
    return true;
}

QString WinRtRunConfiguration::arguments() const
{
    return extraAspect<ProjectExplorer::ArgumentsAspect>()->arguments();
}

void WinRtRunConfiguration::setUninstallAfterStop(bool b)
{
    m_uninstallAfterStop = b;
    emit uninstallAfterStopChanged(m_uninstallAfterStop);
}

QString WinRtRunConfiguration::buildSystemTarget() const
{
    return static_cast<QmakeProjectManager::QmakeProject *>(target()->project())
            ->mapProFilePathToTarget(Utils::FileName::fromString(m_proFilePath));
}

} // namespace Internal
} // namespace WinRt
