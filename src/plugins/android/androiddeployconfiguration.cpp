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

#include "androiddeploystep.h"
#include "androidpackageinstallationstep.h"
#include "androidpackagecreationstep.h"
#include "androiddeployconfiguration.h"
#include "androidtarget.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>

#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qt4projectmanager/qt4project.h>

using namespace Android::Internal;

AndroidDeployConfiguration::AndroidDeployConfiguration(ProjectExplorer::Target *parent)
    :DeployConfiguration(parent, Core::Id(ANDROID_DEPLOYCONFIGURATION_ID))
{
    setDisplayName(tr("Deploy to Android device"));
    setDefaultDisplayName(displayName());
}

AndroidDeployConfiguration::~AndroidDeployConfiguration()
{

}

AndroidDeployConfiguration::AndroidDeployConfiguration(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source)
    :DeployConfiguration(parent, source)
{

}

AndroidDeployConfigurationFactory::AndroidDeployConfigurationFactory(QObject *parent) :
    ProjectExplorer::DeployConfigurationFactory(parent)
{ }

bool AndroidDeployConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const Core::Id id) const
{
    AndroidTarget *t = qobject_cast<AndroidTarget *>(parent);
    if (!t || t->id() != Core::Id(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID)
            || !id.toString().startsWith(QLatin1String(ANDROID_DEPLOYCONFIGURATION_ID)))
        return false;
    return true;
}

ProjectExplorer::DeployConfiguration *AndroidDeployConfigurationFactory::create(ProjectExplorer::Target *parent, const Core::Id id)
{
    Q_UNUSED(id);
    AndroidDeployConfiguration *dc = new AndroidDeployConfiguration(parent);
    if (!dc)
        return 0;
    dc->stepList()->insertStep(0, new AndroidPackageInstallationStep(dc->stepList()));
    dc->stepList()->insertStep(1, new AndroidPackageCreationStep(dc->stepList()));
    dc->stepList()->insertStep(2, new AndroidDeployStep(dc->stepList()));
    return dc;
}

bool AndroidDeployConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::DeployConfiguration *AndroidDeployConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    AndroidTarget *t = static_cast<AndroidTarget *>(parent);
    AndroidDeployConfiguration *dc = new AndroidDeployConfiguration(t);
    if (dc->fromMap(map))
        return dc;

    delete dc;
    return 0;
}

bool AndroidDeployConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source) const
{
    if (!qobject_cast<AndroidTarget *>(parent))
        return false;
    return source->id() == Core::Id(ANDROID_DEPLOYCONFIGURATION_ID);
}

ProjectExplorer::DeployConfiguration *AndroidDeployConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    AndroidTarget *t = static_cast<AndroidTarget *>(parent);
    return new AndroidDeployConfiguration(t, source);
}

QList<Core::Id> AndroidDeployConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    AndroidTarget *target = qobject_cast<AndroidTarget *>(parent);
    if (!target ||
        target->id() != Core::Id(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        return QList<Core::Id>();

    QList<Core::Id> result;
    foreach (const QString &id, target->qt4Project()->applicationProFilePathes(QLatin1String(ANDROID_DC_PREFIX)))
        result << Core::Id(id.toUtf8().constData());
    return result;
}

QString AndroidDeployConfigurationFactory::displayNameForId(const Core::Id/*id*/) const
{
    return tr("Deploy on Android");
}
