/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qt4maemotargetfactory.h"
#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "remotelinuxrunconfiguration.h"
#include "qt4maemodeployconfiguration.h"
#include "qt4maemotarget.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <qt4projectmanager/buildconfigurationinfo.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/qtcassert.h>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Constants;
using ProjectExplorer::idFromMap;

namespace RemoteLinux {
namespace Internal {

// -------------------------------------------------------------------------
// Qt4MaemoTargetFactory
// -------------------------------------------------------------------------
Qt4MaemoTargetFactory::Qt4MaemoTargetFactory(QObject *parent) :
    Qt4BaseTargetFactory(parent)
{
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>)),
            this, SIGNAL(supportedTargetIdsChanged()));
}

Qt4MaemoTargetFactory::~Qt4MaemoTargetFactory()
{
}

bool Qt4MaemoTargetFactory::supportsTargetId(const QString &id) const
{
    return MaemoGlobal::isMaemoTargetId(id);
}

QStringList Qt4MaemoTargetFactory::supportedTargetIds(ProjectExplorer::Project *parent) const
{
    QStringList targetIds;
    if (parent && !qobject_cast<Qt4Project *>(parent))
        return targetIds;
    if (!QtSupport::QtVersionManager::instance()->versionsForTargetId(QLatin1String(MAEMO5_DEVICE_TARGET_ID)).isEmpty())
        targetIds << QLatin1String(MAEMO5_DEVICE_TARGET_ID);
    if (!QtSupport::QtVersionManager::instance()->versionsForTargetId(QLatin1String(HARMATTAN_DEVICE_TARGET_ID)).isEmpty())
        targetIds << QLatin1String(HARMATTAN_DEVICE_TARGET_ID);
    if (!QtSupport::QtVersionManager::instance()->versionsForTargetId(QLatin1String(MEEGO_DEVICE_TARGET_ID)).isEmpty())
        targetIds << QLatin1String(MEEGO_DEVICE_TARGET_ID);
    return targetIds;
}

QString Qt4MaemoTargetFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(MAEMO5_DEVICE_TARGET_ID))
        return Qt4Maemo5Target::defaultDisplayName();
    else if (id == QLatin1String(HARMATTAN_DEVICE_TARGET_ID))
        return Qt4HarmattanTarget::defaultDisplayName();
    else if (id == QLatin1String(MEEGO_DEVICE_TARGET_ID))
        return Qt4MeegoTarget::defaultDisplayName();
    return QString();
}

QIcon Qt4MaemoTargetFactory::iconForId(const QString &id) const
{
    Q_UNUSED(id)
    return QIcon(":/projectexplorer/images/MaemoDevice.png");
}

bool Qt4MaemoTargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return false;
    return supportsTargetId(id);
}

bool Qt4MaemoTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

ProjectExplorer::Target *Qt4MaemoTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    const QString id = idFromMap(map);
    AbstractQt4MaemoTarget *target = 0;
    Qt4Project *qt4project = static_cast<Qt4Project *>(parent);
    if (id == QLatin1String(MAEMO5_DEVICE_TARGET_ID))
        target = new Qt4Maemo5Target(qt4project, QLatin1String("transient ID"));
    else if (id == QLatin1String(HARMATTAN_DEVICE_TARGET_ID))
        target = new Qt4HarmattanTarget(qt4project, QLatin1String("transient ID"));
    else if (id == QLatin1String(MEEGO_DEVICE_TARGET_ID))
        target = new Qt4MeegoTarget(qt4project, QLatin1String("transient ID"));
    if (target->fromMap(map))
        return target;
    delete target;
    return 0;
}

QString Qt4MaemoTargetFactory::buildNameForId(const QString &id) const
{
    if (id == QLatin1String(MAEMO5_DEVICE_TARGET_ID))
        return QLatin1String("maemo");
    else if (id == QLatin1String(HARMATTAN_DEVICE_TARGET_ID))
        return QLatin1String("harmattan");
    else if (id == QLatin1String(MEEGO_DEVICE_TARGET_ID))
        return QLatin1String("meego");
    else
        return QString();
}

QString Qt4MaemoTargetFactory::shadowBuildDirectory(const QString &profilePath, const QString &id, const QString &suffix)
{
#if defined(Q_OS_WIN)
    // No shadowbuilding for windows!
    Q_UNUSED(id);
    Q_UNUSED(suffix);
    return QFileInfo(profilePath).absolutePath();
#else
    return Qt4BaseTargetFactory::shadowBuildDirectory(profilePath, id, suffix);
#endif
}

QSet<QString> Qt4MaemoTargetFactory::targetFeatures(const QString & /*id*/) const
{
    QSet<QString> features;
    features << Qt4ProjectManager::Constants::MOBILE_TARGETFEATURE_ID;
#ifndef Q_OS_WIN
    features << Qt4ProjectManager::Constants::SHADOWBUILD_TARGETFEATURE_ID;
#endif
    // how to check check whether the component set is really installed?
    features << Qt4ProjectManager::Constants::QTQUICKCOMPONENTS_MEEGO_TARGETFEATURE_ID;
    return features;
}

ProjectExplorer::Target *Qt4MaemoTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    QList<QtSupport::BaseQtVersion *> knownVersions = QtSupport::QtVersionManager::instance()->versionsForTargetId(id);
    if (knownVersions.isEmpty())
        return 0;

    QtSupport::BaseQtVersion *qtVersion = knownVersions.first();
    QtSupport::BaseQtVersion::QmakeBuildConfigs config = qtVersion->defaultBuildConfig();

    QList<BuildConfigurationInfo> infos;
    infos.append(BuildConfigurationInfo(qtVersion, config, QString(), QString()));
    infos.append(BuildConfigurationInfo(qtVersion, config ^ QtSupport::BaseQtVersion::DebugBuild, QString(), QString()));

    return create(parent, id, infos);
}

ProjectExplorer::Target *Qt4MaemoTargetFactory::create(ProjectExplorer::Project *parent,
    const QString &id, const QList<BuildConfigurationInfo> &infos)
{
    if (!canCreate(parent, id))
        return 0;

    AbstractQt4MaemoTarget *target = 0;
    QStringList deployConfigIds;
    if (id == QLatin1String(MAEMO5_DEVICE_TARGET_ID)) {
        target = new Qt4Maemo5Target(static_cast<Qt4Project *>(parent), id);
        deployConfigIds << Qt4MaemoDeployConfiguration::fremantleWithPackagingId()
            << Qt4MaemoDeployConfiguration::fremantleWithoutPackagingId();
    } else if (id == QLatin1String(HARMATTAN_DEVICE_TARGET_ID)) {
        target = new Qt4HarmattanTarget(static_cast<Qt4Project *>(parent), id);
        deployConfigIds << Qt4MaemoDeployConfiguration::harmattanId();
    } else if (id == QLatin1String(MEEGO_DEVICE_TARGET_ID)) {
        target = new Qt4MeegoTarget(static_cast<Qt4Project *>(parent), id);
        deployConfigIds << Qt4MaemoDeployConfiguration::meegoId();
    }
    Q_ASSERT(target);

    foreach (const BuildConfigurationInfo &info, infos)
        target->addQt4BuildConfiguration(msgBuildConfigurationName(info), QString(),
                                         info.version, info.buildConfig,
                                         info.additionalArguments, info.directory, info.importing);

    foreach (const QString &deployConfigId, deployConfigIds) {
        target->addDeployConfiguration(target->createDeployConfiguration(deployConfigId));
    }
    target->createApplicationProFiles();
    if (target->runConfigurations().isEmpty())
        target->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(target));
    return target;
}

} // namespace Internal
} // namespace RemoteLinux
