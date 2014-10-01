/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidpackagecreationfactory.h"

#include "androidpackagecreationstep.h"
#include "androidmanager.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidPackageCreationFactory::AndroidPackageCreationFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<Core::Id> AndroidPackageCreationFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->id() != Constants::BUILDSTEPS_DEPLOY)
        return QList<Core::Id>();
    if (!AndroidManager::supportsAndroid(parent->target()))
        return QList<Core::Id>();
    if (parent->contains(AndroidPackageCreationStep::CreatePackageId))
        return QList<Core::Id>();
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(parent->target()->kit());
    if (qtVersion && qtVersion->qtVersion() >= QtSupport::QtVersionNumber(5, 2, 0))
        return QList<Core::Id>();
    return QList<Core::Id>() << AndroidPackageCreationStep::CreatePackageId;
}

QString AndroidPackageCreationFactory::displayNameForId(Core::Id id) const
{
    if (id == AndroidPackageCreationStep::CreatePackageId)
        return tr("Create Android (.apk) Package");
    return QString();
}

bool AndroidPackageCreationFactory::canCreate(BuildStepList *parent, Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

BuildStep *AndroidPackageCreationFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));
    Q_UNUSED(id);
    return new AndroidPackageCreationStep(parent);
}

bool AndroidPackageCreationFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *AndroidPackageCreationFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));
    AndroidPackageCreationStep *const step = new AndroidPackageCreationStep(parent);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool AndroidPackageCreationFactory::canClone(BuildStepList *parent, BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *AndroidPackageCreationFactory::clone(BuildStepList *parent, BuildStep *product)
{
    Q_ASSERT(canClone(parent, product));
    return new AndroidPackageCreationStep(parent, static_cast<AndroidPackageCreationStep *>(product));
}

} // namespace Internal
} // namespace Android
