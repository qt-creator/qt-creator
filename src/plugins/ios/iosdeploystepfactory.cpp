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

QList<Core::Id> IosDeployStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        return QList<Core::Id>();
    if (!IosManager::supportsIos(parent->target()))
        return QList<Core::Id>();
    if (parent->contains(IosDeployStep::Id))
        return QList<Core::Id>();
    return QList<Core::Id>() << IosDeployStep::Id;
}

QString IosDeployStepFactory::displayNameForId(Core::Id id) const
{
    if (id == IosDeployStep::Id)
        return tr("Deploy to iOS device or emulator");
    return QString();
}

bool IosDeployStepFactory::canCreate(BuildStepList *parent, Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

BuildStep *IosDeployStepFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));
    Q_UNUSED(id);
    return new IosDeployStep(parent);
}

bool IosDeployStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *IosDeployStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));
    IosDeployStep * const step = new IosDeployStep(parent);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool IosDeployStepFactory::canClone(BuildStepList *parent, BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *IosDeployStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    Q_ASSERT(canClone(parent, product));
    return new IosDeployStep(parent, static_cast<IosDeployStep *>(product));
}

} // namespace Internal
} // namespace Ios
