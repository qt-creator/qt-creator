/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "androidpackagecreationfactory.h"

#include "androidpackagecreationstep.h"
#include "androidmanager.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qtsupport/qtprofileinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <QCoreApplication>

using ProjectExplorer::BuildStepList;
using ProjectExplorer::BuildStep;

namespace Android {
namespace Internal {

AndroidPackageCreationFactory::AndroidPackageCreationFactory(QObject *parent)
    : ProjectExplorer::IBuildStepFactory(parent)
{
}

QList<Core::Id> AndroidPackageCreationFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() != Core::Id(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY))
        return QList<Core::Id>();
    if (!AndroidManager::supportsAndroid(parent->target()))
        return QList<Core::Id>();
    if (parent->contains(AndroidPackageCreationStep::CreatePackageId))
        return QList<Core::Id>();
    return QList<Core::Id>() << AndroidPackageCreationStep::CreatePackageId;
}

QString AndroidPackageCreationFactory::displayNameForId(const Core::Id id) const
{
    if (id == AndroidPackageCreationStep::CreatePackageId)
        return QCoreApplication::translate("Qt4ProjectManager::Internal::AndroidPackageCreationFactory",
                                           "Create Android (.apk) Package");
    return QString();
}

bool AndroidPackageCreationFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

BuildStep *AndroidPackageCreationFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));
    return new AndroidPackageCreationStep(parent);
}

bool AndroidPackageCreationFactory::canRestore(ProjectExplorer::BuildStepList *parent,
                                               const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

BuildStep *AndroidPackageCreationFactory::restore(ProjectExplorer::BuildStepList *parent,
                                                  const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));
    AndroidPackageCreationStep *const step
        = new AndroidPackageCreationStep(parent);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool AndroidPackageCreationFactory::canClone(ProjectExplorer::BuildStepList *parent,
                                             ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *AndroidPackageCreationFactory::clone(ProjectExplorer::BuildStepList *parent,
                                                ProjectExplorer::BuildStep *product)
{
    Q_ASSERT(canClone(parent, product));
    return new AndroidPackageCreationStep(parent, static_cast<AndroidPackageCreationStep *>(product));
}

} // namespace Internal
} // namespace Android
