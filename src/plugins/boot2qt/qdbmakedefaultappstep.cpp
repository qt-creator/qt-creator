/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qdbmakedefaultappstep.h"

#include "qdbconstants.h"
#include "qdbmakedefaultappservice.h"

#include <projectexplorer/runconfigurationaspects.h>

#include <remotelinux/abstractremotelinuxdeploystep.h>

using namespace ProjectExplorer;

namespace Qdb {
namespace Internal {

class QdbMakeDefaultAppStep final : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
    Q_DECLARE_TR_FUNCTIONS(Qdb::Internal::QdbMakeDefaultAppStep)

public:
    QdbMakeDefaultAppStep(BuildStepList *bsl, Utils::Id id);

    static QString stepDisplayName() { return tr("Change default application"); }
};

QdbMakeDefaultAppStep::QdbMakeDefaultAppStep(BuildStepList *bsl, Utils::Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    setDefaultDisplayName(stepDisplayName());

    auto service = createDeployService<QdbMakeDefaultAppService>();

    auto selection = addAspect<BaseSelectionAspect>();
    selection->setSettingsKey("QdbMakeDefaultDeployStep.MakeDefault");
    selection->addOption(tr("Set this application to start by default"));
    selection->addOption(tr("Reset default application"));

    setInternalInitializer([service, selection] {
        service->setMakeDefault(selection->value() == 0);
        return service->isDeploymentPossible();
    });
}

// QdbMakeDefaultAppStepFactory

QdbMakeDefaultAppStepFactory::QdbMakeDefaultAppStepFactory()
{
    registerStep<QdbMakeDefaultAppStep>(Constants::QdbMakeDefaultAppStepId);
    setDisplayName(QdbMakeDefaultAppStep::stepDisplayName());
    setSupportedDeviceType(Qdb::Constants::QdbLinuxOsType);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // namespace Internal
} // namespace Qdb
