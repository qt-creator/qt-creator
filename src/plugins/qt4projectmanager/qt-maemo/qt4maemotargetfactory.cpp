/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qt4maemotargetfactory.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qt-maemo/maemodeploystep.h"
#include "maemoglobal.h"
#include "qt-maemo/maemopackagecreationstep.h"
#include "qt-maemo/maemorunconfiguration.h"
#include "qt-maemo/qt4maemotarget.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customexecutablerunconfiguration.h>

#include <QtGui/QApplication>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::idFromMap;

// -------------------------------------------------------------------------
// Qt4MaemoTargetFactory
// -------------------------------------------------------------------------
Qt4MaemoTargetFactory::Qt4MaemoTargetFactory(QObject *parent) :
    Qt4BaseTargetFactory(parent)
{
    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>)),
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
    if (!qobject_cast<Qt4Project *>(parent))
        return targetIds;
    if (QtVersionManager::instance()->supportsTargetId(QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID)))
        targetIds << QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID);
    if (QtVersionManager::instance()->supportsTargetId(QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID)))
        targetIds << QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID);
    if (QtVersionManager::instance()->supportsTargetId(QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID)))
        targetIds << QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID);
    return targetIds;
}

QString Qt4MaemoTargetFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID))
        return Qt4Maemo5Target::defaultDisplayName();
    else if (id == QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID))
        return Qt4HarmattanTarget::defaultDisplayName();
    else if (id == QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID))
        return Qt4MeegoTarget::defaultDisplayName();
    return QString();
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

Qt4BaseTarget *Qt4MaemoTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    const QString id = idFromMap(map);
    AbstractQt4MaemoTarget *target = 0;
    Qt4Project *qt4project = static_cast<Qt4Project *>(parent);
    if (id == QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID))
        target = new Qt4Maemo5Target(qt4project, QLatin1String("transient ID"));
    else if (id == QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID))
        target = new Qt4HarmattanTarget(qt4project, QLatin1String("transient ID"));
    else if (id == QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID))
        target = new Qt4MeegoTarget(qt4project, QLatin1String("transient ID"));
    if (target->fromMap(map))
        return target;
    delete target;
    return 0;
}

QString Qt4MaemoTargetFactory::defaultShadowBuildDirectory(const QString &projectLocation, const QString &id)
{
    QString suffix;
    if (id == QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID))
        suffix = QLatin1String("maemo");
    else if (id == QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID))
        suffix = QLatin1String("harmattan");
    else if (id == QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID))
        suffix = QLatin1String("meego");
    else
        return QString();

    // currently we can't have the build directory to be deeper than the source directory
    // since that is broken in qmake
    // Once qmake is fixed we can change that to have a top directory and
    // subdirectories per build. (Replacing "QChar('-')" with "QChar('/') )
    return projectLocation + QLatin1Char('-') + suffix;
}

QList<BuildConfigurationInfo> Qt4MaemoTargetFactory::availableBuildConfigurations(const QString &proFilePath)
{
    return QList<BuildConfigurationInfo>()
        << availableBuildConfigurations(proFilePath, QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID))
        << availableBuildConfigurations(proFilePath, QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID))
        << availableBuildConfigurations(proFilePath, QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID));
}

QList<BuildConfigurationInfo> Qt4MaemoTargetFactory::availableBuildConfigurations(const QString &proFilePath,
    const QString &id)
{
    QList<BuildConfigurationInfo> infos;
    QList<QtVersion *> knownVersions = QtVersionManager::instance()->versionsForTargetId(id);

    foreach (QtVersion *version, knownVersions) {
        if (!version->isValid())
            continue;
        QtVersion::QmakeBuildConfigs config = version->defaultBuildConfig();
        QString dir = defaultShadowBuildDirectory(Qt4Project::defaultTopLevelBuildDirectory(proFilePath), id);
        infos.append(BuildConfigurationInfo(version, config, QString(), dir));
        infos.append(BuildConfigurationInfo(version, config ^ QtVersion::DebugBuild, QString(), dir));
    }
    return infos;
}

Qt4BaseTarget *Qt4MaemoTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    QList<QtVersion *> knownVersions = QtVersionManager::instance()->versionsForTargetId(id);
    if (knownVersions.isEmpty())
        return 0;

    QtVersion *qtVersion = knownVersions.first();
    QtVersion::QmakeBuildConfigs config = qtVersion->defaultBuildConfig();

    QList<BuildConfigurationInfo> infos;
    infos.append(BuildConfigurationInfo(qtVersion, config, QString(), QString()));
    infos.append(BuildConfigurationInfo(qtVersion, config ^ QtVersion::DebugBuild, QString(), QString()));

    return create(parent, id, infos);
}

Qt4BaseTarget *Qt4MaemoTargetFactory::create(ProjectExplorer::Project *parent,
    const QString &id, QList<BuildConfigurationInfo> infos)
{
    if (!canCreate(parent, id))
        return 0;

    AbstractQt4MaemoTarget *target = 0;
    if (id == QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID))
        target = new Qt4Maemo5Target(static_cast<Qt4Project *>(parent), id);
    else if (id == QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID))
        target = new Qt4HarmattanTarget(static_cast<Qt4Project *>(parent), id);
    else if (id == QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID))
        target = new Qt4MeegoTarget(static_cast<Qt4Project *>(parent), id);
    Q_ASSERT(target);

    foreach (const BuildConfigurationInfo &info, infos)
        target->addQt4BuildConfiguration(msgBuildConfigurationName(info),
                                         info.version, info.buildConfig,
                                         info.additionalArguments, info.directory);

    target->addDeployConfiguration(target->deployConfigurationFactory()->create(target, ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));
    target->createApplicationProFiles();
    if (target->runConfigurations().isEmpty())
        target->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(target));
    return target;
}
