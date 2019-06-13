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

#include "qdbmakedefaultappservice.h"

#include <projectexplorer/runconfigurationaspects.h>

using namespace ProjectExplorer;

namespace Qdb {
namespace Internal {

QdbMakeDefaultAppStep::QdbMakeDefaultAppStep(BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
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

Core::Id QdbMakeDefaultAppStep::stepId()
{
    return "Qdb.MakeDefaultAppStep";
}

QString QdbMakeDefaultAppStep::stepDisplayName()
{
    return QStringLiteral("Change default application");
}

} // namespace Internal
} // namespace Qdb
