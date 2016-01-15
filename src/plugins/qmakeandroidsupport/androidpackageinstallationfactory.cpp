/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidpackageinstallationfactory.h"
#include "androidpackageinstallationstep.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <android/androidmanager.h>

using namespace ProjectExplorer;

namespace QmakeAndroidSupport {
namespace Internal {

AndroidPackageInstallationFactory::AndroidPackageInstallationFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<Core::Id> AndroidPackageInstallationFactory::availableCreationIds(BuildStepList *parent) const
{
    Q_UNUSED(parent);
    return QList<Core::Id>();
}

QString AndroidPackageInstallationFactory::displayNameForId(Core::Id id) const
{
    if (id == AndroidPackageInstallationStep::Id)
        return tr("Deploy to device");
    return QString();
}

bool AndroidPackageInstallationFactory::canCreate(BuildStepList *parent, Core::Id id) const
{
    Q_UNUSED(parent);
    Q_UNUSED(id);
    return false;
}

BuildStep *AndroidPackageInstallationFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_UNUSED(parent);
    Q_UNUSED(id);
    return 0;
}

bool AndroidPackageInstallationFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return false;
    if (!Android::AndroidManager::supportsAndroid(parent->target()))
        return false;
    if (parent->contains(AndroidPackageInstallationStep::Id))
        return false;
    return ProjectExplorer::idFromMap(map) == AndroidPackageInstallationStep::Id;
}

BuildStep *AndroidPackageInstallationFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));
    AndroidPackageInstallationStep * const step = new AndroidPackageInstallationStep(parent);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool AndroidPackageInstallationFactory::canClone(BuildStepList *parent, BuildStep *product) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return false;
    if (!Android::AndroidManager::supportsAndroid(parent->target()))
        return false;
    if (product->id() != AndroidPackageInstallationStep::Id)
        return false;
    if (parent->contains(AndroidPackageInstallationStep::Id))
        return false;
    return true;
}

BuildStep *AndroidPackageInstallationFactory::clone(BuildStepList *parent, BuildStep *product)
{
    Q_ASSERT(canClone(parent, product));
    return new AndroidPackageInstallationStep(parent, static_cast<AndroidPackageInstallationStep*>(product));
}

} // namespace Internal
} // namespace Android
