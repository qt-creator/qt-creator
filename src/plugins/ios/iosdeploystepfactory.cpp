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

#include "iosdeploystepfactory.h"

#include "iosdeploystep.h"
#include "iosmanager.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtkitinformation.h>

#include <QCoreApplication>

using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

IosDeployStepFactory::IosDeployStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<BuildStepInfo> IosDeployStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_DEPLOY
            && IosManager::supportsIos(parent->target())
            && !parent->contains(IosDeployStep::Id))
        return {{IosDeployStep::Id, tr("Deploy to iOS device or emulator")}};
    return {};
}

BuildStep *IosDeployStepFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_UNUSED(id);
    return new IosDeployStep(parent);
}

BuildStep *IosDeployStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    return new IosDeployStep(parent, static_cast<IosDeployStep *>(product));
}

} // namespace Internal
} // namespace Ios
