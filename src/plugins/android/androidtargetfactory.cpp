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

#include "androidtargetfactory.h"
#include "qt4projectmanager/qt4project.h"
#include "qt4projectmanager/qt4projectmanagerconstants.h"
#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidpackagecreationstep.h"
#include "androidrunconfiguration.h"
#include "androidtarget.h"
#include "androiddeployconfiguration.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qt4projectmanager/buildconfigurationinfo.h>
#include <qtsupport/customexecutablerunconfiguration.h>

#include <qtsupport/qtversionmanager.h>

using namespace Qt4ProjectManager;
using namespace Android::Internal;
using ProjectExplorer::idFromMap;

// -------------------------------------------------------------------------
// Qt4AndroidTargetFactory
// -------------------------------------------------------------------------
AndroidTargetFactory::AndroidTargetFactory(QObject *parent) :
    Qt4BaseTargetFactory(parent)
{
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SIGNAL(canCreateTargetIdsChanged()));
}

AndroidTargetFactory::~AndroidTargetFactory()
{
}

bool AndroidTargetFactory::supportsTargetId(const Core::Id id) const
{
    return id == Core::Id(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID);
}

QSet<QString> AndroidTargetFactory::targetFeatures(const Core::Id id) const
{
    Q_UNUSED(id);
    QSet<QString> features;
    features << QLatin1String(Qt4ProjectManager::Constants::MOBILE_TARGETFEATURE_ID);
    features << QLatin1String(Qt4ProjectManager::Constants::SHADOWBUILD_TARGETFEATURE_ID);
    return features;
}

QList<Core::Id> AndroidTargetFactory::supportedTargetIds() const
{
    return QList<Core::Id>() << Core::Id(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID);
}

QString AndroidTargetFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        return AndroidTarget::defaultDisplayName();
    return QString();
}

QIcon AndroidTargetFactory::iconForId(const Core::Id id) const
{
    Q_UNUSED(id)
    return QIcon(QLatin1String(Constants::ANDROID_SETTINGS_CATEGORY_ICON));
}

bool AndroidTargetFactory::canCreate(ProjectExplorer::Project *parent, const Core::Id id) const
{
    if (!qobject_cast<Qt4Project *>(parent))
            return false;
    if (!supportsTargetId(id))
            return false;
    return QtSupport::QtVersionManager::instance()->supportsTargetId(id);
}

bool AndroidTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

Qt4BaseTarget *AndroidTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    const Core::Id id = idFromMap(map);
    AndroidTarget *target = 0;
    Qt4Project *qt4project = static_cast<Qt4Project *>(parent);
    if (id == Core::Id(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        target = new AndroidTarget(qt4project, id);
    if (target && target->fromMap(map))
        return target;
    delete target;
    return 0;
}

Qt4BaseTarget *AndroidTargetFactory::create(ProjectExplorer::Project *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    QList<QtSupport::BaseQtVersion *> knownVersions = QtSupport::QtVersionManager::instance()->versionsForTargetId(id);
    if (knownVersions.isEmpty())
        return 0;

    QtSupport::BaseQtVersion *qtVersion = knownVersions.first();
    bool buildAll = qtVersion->isValid() && (qtVersion->defaultBuildConfig() & QtSupport::BaseQtVersion::BuildAll);
    QtSupport::BaseQtVersion::QmakeBuildConfigs config = buildAll ? QtSupport::BaseQtVersion::BuildAll : QtSupport::BaseQtVersion::QmakeBuildConfig(0);

    QList<Qt4ProjectManager::BuildConfigurationInfo> infos;
    infos.append(Qt4ProjectManager::BuildConfigurationInfo(qtVersion->uniqueId(), config, QString(), QString()));
    infos.append(Qt4ProjectManager::BuildConfigurationInfo(qtVersion->uniqueId(), config ^ QtSupport::BaseQtVersion::DebugBuild, QString(), QString()));

    return create(parent, id, infos);
}

Qt4BaseTarget *AndroidTargetFactory::create(ProjectExplorer::Project *parent, const Core::Id id,
                                            const QList<Qt4ProjectManager::BuildConfigurationInfo> &infos)
{
    if (!canCreate(parent, id))
        return 0;

    AndroidTarget *target = 0;
    if (id == Core::Id(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        target = new AndroidTarget(static_cast<Qt4Project *>(parent), id);
    Q_ASSERT(target);

    foreach (const Qt4ProjectManager::BuildConfigurationInfo &info, infos) {
        QString displayName = info.version()->displayName() + QLatin1Char(' ');
        displayName += (info.buildConfig & QtSupport::BaseQtVersion::DebugBuild) ? tr("Debug") : tr("Release");
        target->addQt4BuildConfiguration(displayName, QString(),
                                    info.version(),
                                    info.buildConfig,
                                    info.additionalArguments,
                                    info.directory,
                                    info.importing);
    }

    target->addDeployConfiguration(target->createDeployConfiguration(Core::Id(ANDROID_DEPLOYCONFIGURATION_ID)));

    target->createApplicationProFiles(false);
    if (target->runConfigurations().isEmpty())
        target->addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(target));
    return target;
}

QString AndroidTargetFactory::buildNameForId(const Core::Id id) const
{
    if (id == Core::Id(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
        return QLatin1String("android");
    return QString();
}
