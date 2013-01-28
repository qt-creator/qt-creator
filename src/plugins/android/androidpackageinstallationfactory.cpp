/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidpackageinstallationfactory.h"

#include "androidpackageinstallationstep.h"
#include "androidmanager.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidPackageInstallationFactory::AndroidPackageInstallationFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<Core::Id> AndroidPackageInstallationFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        return QList<Core::Id>();
    if (!AndroidManager::supportsAndroid(parent->target()))
        return QList<Core::Id>();
    if (parent->contains(AndroidPackageInstallationStep::Id))
        return QList<Core::Id>();
    return QList<Core::Id>() << AndroidPackageInstallationStep::Id;
}

QString AndroidPackageInstallationFactory::displayNameForId(const Core::Id id) const
{
    if (id == AndroidPackageInstallationStep::Id)
        return tr("Deploy to device");
    return QString();
}

bool AndroidPackageInstallationFactory::canCreate(BuildStepList *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

BuildStep *AndroidPackageInstallationFactory::create(BuildStepList *parent, const Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));
    Q_UNUSED(id);
    return new AndroidPackageInstallationStep(parent);
}

bool AndroidPackageInstallationFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
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
    return canCreate(parent, product->id());
}

BuildStep *AndroidPackageInstallationFactory::clone(BuildStepList *parent, BuildStep *product)
{
    Q_ASSERT(canClone(parent, product));
    return new AndroidPackageInstallationStep(parent, static_cast<AndroidPackageInstallationStep*>(product));
}

} // namespace Internal
} // namespace Android
