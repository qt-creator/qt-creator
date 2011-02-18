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

#include "qt4simulatortargetfactory.h"
#include "qt4projectmanagerconstants.h"
#include "qt4project.h"
#include "qt4runconfiguration.h"
#include "qt4simulatortarget.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customexecutablerunconfiguration.h>

#include <QtGui/QApplication>
#include <QtGui/QStyle>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::idFromMap;

// -------------------------------------------------------------------------
// Qt4SimulatorTargetFactory
// -------------------------------------------------------------------------

Qt4SimulatorTargetFactory::Qt4SimulatorTargetFactory(QObject *parent) :
    Qt4BaseTargetFactory(parent)
{
    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>)),
            this, SIGNAL(supportedTargetIdsChanged()));
}

Qt4SimulatorTargetFactory::~Qt4SimulatorTargetFactory()
{
}

bool Qt4SimulatorTargetFactory::supportsTargetId(const QString &id) const
{
    return id == QLatin1String(Constants::QT_SIMULATOR_TARGET_ID);
}

QStringList Qt4SimulatorTargetFactory::supportedTargetIds(ProjectExplorer::Project *parent) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return QStringList();
    if (!QtVersionManager::instance()->supportsTargetId(Constants::QT_SIMULATOR_TARGET_ID))
        return QStringList();
    return QStringList() << QLatin1String(Constants::QT_SIMULATOR_TARGET_ID);
}

QString Qt4SimulatorTargetFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(Constants::QT_SIMULATOR_TARGET_ID))
        return Qt4SimulatorTarget::defaultDisplayName();
    return QString();
}

bool Qt4SimulatorTargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return false;
    return supportsTargetId(id);
}

bool Qt4SimulatorTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

Qt4BaseTarget *Qt4SimulatorTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Qt4Project *qt4project = static_cast<Qt4Project *>(parent);
    Qt4SimulatorTarget *target = new Qt4SimulatorTarget(qt4project, QLatin1String("transient ID"));
    if (target->fromMap(map))
        return target;
    delete target;
    return 0;
}

QString Qt4SimulatorTargetFactory::defaultShadowBuildDirectory(const QString &projectLocation, const QString &id)
{
    if (id != QLatin1String(Constants::QT_SIMULATOR_TARGET_ID))
        return QString();

    // currently we can't have the build directory to be deeper than the source directory
    // since that is broken in qmake
    // Once qmake is fixed we can change that to have a top directory and
    // subdirectories per build. (Replacing "QChar('-')" with "QChar('/') )
    return projectLocation + QLatin1String("-simulator");
}

QList<BuildConfigurationInfo> Qt4SimulatorTargetFactory::availableBuildConfigurations(const QString &proFilePath)
{
    QList<BuildConfigurationInfo> infos;
    QList<QtVersion *> knownVersions = QtVersionManager::instance()->versionsForTargetId(Constants::QT_SIMULATOR_TARGET_ID);

    foreach (QtVersion *version, knownVersions) {
        if (!version->isValid())
            continue;
        QtVersion::QmakeBuildConfigs config = version->defaultBuildConfig();
        QString dir = defaultShadowBuildDirectory(Qt4Project::defaultTopLevelBuildDirectory(proFilePath), Constants::QT_SIMULATOR_TARGET_ID);
        infos.append(BuildConfigurationInfo(version, config, QString(), dir));
        infos.append(BuildConfigurationInfo(version, config ^ QtVersion::DebugBuild, QString(), dir));
    }
    return infos;
}

Qt4BaseTarget *Qt4SimulatorTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
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

Qt4BaseTarget *Qt4SimulatorTargetFactory::create(ProjectExplorer::Project *parent, const QString &id, QList<BuildConfigurationInfo> infos)
{
    if (!canCreate(parent, id))
        return 0;
    Qt4SimulatorTarget *t = new Qt4SimulatorTarget(static_cast<Qt4Project *>(parent), id);

    foreach (const BuildConfigurationInfo &info, infos)
        t->addQt4BuildConfiguration(msgBuildConfigurationName(info), info.version, info.buildConfig,
                                    info.additionalArguments, info.directory);

    t->addDeployConfiguration(t->deployConfigurationFactory()->create(t, ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));

    t->createApplicationProFiles();

    if (t->runConfigurations().isEmpty())
        t->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(t));
    return t;
}
