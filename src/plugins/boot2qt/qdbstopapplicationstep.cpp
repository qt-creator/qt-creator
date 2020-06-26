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

#include "qdbstopapplicationstep.h"

#include "qdbconstants.h"
#include "qdbstopapplicationservice.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <remotelinux/abstractremotelinuxdeploystep.h>

using namespace ProjectExplorer;

namespace Qdb {
namespace Internal {

class QdbStopApplicationStep final : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
    Q_DECLARE_TR_FUNCTIONS(Qdb::Internal::QdbStopApplicationStep)

public:
    QdbStopApplicationStep(BuildStepList *bsl, Utils::Id id);

    static QString stepDisplayName() { return tr("Stop already running application"); }
};

QdbStopApplicationStep::QdbStopApplicationStep(BuildStepList *bsl, Utils::Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    auto service = createDeployService<QdbStopApplicationService>();

    setDefaultDisplayName(stepDisplayName());
    setWidgetExpandedByDefault(false);

    setInternalInitializer([service] { return service->isDeploymentPossible(); });
}

// QdbStopApplicationStepFactory

QdbStopApplicationStepFactory::QdbStopApplicationStepFactory()
{
    registerStep<QdbStopApplicationStep>(Constants::QdbStopApplicationStepId);
    setDisplayName(QdbStopApplicationStep::stepDisplayName());
    setSupportedDeviceType(Constants::QdbLinuxOsType);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // namespace Internal
} // namespace Qdb
