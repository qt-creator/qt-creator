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

#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>

using namespace ProjectExplorer;
using QmakeProjectManager::QmakeProject;
using QmakeProjectManager::QmakeProFile;

namespace WinRt {
namespace Internal {

WinRtRunConfigurationFactory::WinRtRunConfigurationFactory()
{
    registerRunConfiguration<WinRtRunConfiguration>();
    setSupportedProjectType<QmakeProject>();
    setSupportedTargetDeviceTypes({Constants::WINRT_DEVICE_TYPE_LOCAL,
                                   Constants::WINRT_DEVICE_TYPE_PHONE,
                                   Constants::WINRT_DEVICE_TYPE_EMULATOR});
}

QList<Core::Id> WinRtRunConfigurationFactory::availableCreationIds(Target *parent,
                                                                   CreationMode mode) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();

    QmakeProject *project = static_cast<QmakeProject *>(parent->project());
    return project->creationIds(Constants::WINRT_RC_PREFIX, mode);
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

bool WinRtRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;

    return idFromMap(map).toString().startsWith(QLatin1String(Constants::WINRT_RC_PREFIX));
}

bool WinRtRunConfigurationFactory::canClone(Target *parent, RunConfiguration *product) const
{
    Q_UNUSED(parent);
    Q_UNUSED(product);
    return false;
}

} // namespace Internal
} // namespace WinRt
