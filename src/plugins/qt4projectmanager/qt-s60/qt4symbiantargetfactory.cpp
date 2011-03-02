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

#include "qt4symbiantargetfactory.h"
#include "qt4projectmanagerconstants.h"
#include "qt4project.h"
#include "buildconfigurationinfo.h"

#include "qt-s60/s60deployconfiguration.h"
#include "qt-s60/s60devicerunconfiguration.h"
#include "qt-s60/s60emulatorrunconfiguration.h"
#include "qt-s60/s60createpackagestep.h"
#include "qt-s60/s60deploystep.h"
#include "qt-s60/qt4symbiantarget.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customexecutablerunconfiguration.h>

using ProjectExplorer::idFromMap;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

// -------------------------------------------------------------------------
// Qt4SymbianTargetFactory
// -------------------------------------------------------------------------

Qt4SymbianTargetFactory::Qt4SymbianTargetFactory(QObject *parent) :
    Qt4BaseTargetFactory(parent)
{
    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>)),
            this, SIGNAL(supportedTargetIdsChanged()));
}

Qt4SymbianTargetFactory::~Qt4SymbianTargetFactory()
{
}

bool Qt4SymbianTargetFactory::supportsTargetId(const QString &id) const
{
    return id == QLatin1String(Constants::S60_DEVICE_TARGET_ID)
            || id == QLatin1String(Constants::S60_EMULATOR_TARGET_ID);
}

QStringList Qt4SymbianTargetFactory::supportedTargetIds(ProjectExplorer::Project *parent) const
{
    if (parent && !qobject_cast<Qt4Project *>(parent))
        return QStringList();

    QStringList ids;
    if (QtVersionManager::instance()->supportsTargetId(Constants::S60_DEVICE_TARGET_ID))
        ids << QLatin1String(Constants::S60_DEVICE_TARGET_ID);
    if (QtVersionManager::instance()->supportsTargetId(Constants::S60_EMULATOR_TARGET_ID))
        ids << QLatin1String(Constants::S60_EMULATOR_TARGET_ID);

    return ids;
}

QString Qt4SymbianTargetFactory::displayNameForId(const QString &id) const
{
    return Qt4SymbianTarget::defaultDisplayName(id);
}

QIcon Qt4SymbianTargetFactory::iconForId(const QString &id) const
{
    if (id == QLatin1String(Constants::S60_EMULATOR_TARGET_ID))
        return QIcon(":/projectexplorer/images/SymbianEmulator.png");
    if (id == QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return QIcon(":/projectexplorer/images/SymbianDevice.png");
    return QIcon();
}

bool Qt4SymbianTargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return false;
    return supportsTargetId(id);
}

bool Qt4SymbianTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

ProjectExplorer::Target *Qt4SymbianTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Qt4Project *qt4project = static_cast<Qt4Project *>(parent);
    Qt4SymbianTarget *target = new Qt4SymbianTarget(qt4project, idFromMap(map));

    if (target->fromMap(map))
        return target;
    delete target;
    return 0;
}

QString Qt4SymbianTargetFactory::defaultShadowBuildDirectory(const QString &projectLocation, const QString &id)
{
    Q_UNUSED(projectLocation);
    Q_UNUSED(id);
    // should not be called from anywhere, since we override Qt4BaseTarget::defaultBuldDirectory()
    return QString();
}

QList<BuildConfigurationInfo> Qt4SymbianTargetFactory::availableBuildConfigurations(const QString &id, const QString &proFilePath, const QtVersionNumber &minimumQtVersion)
{
    QList<BuildConfigurationInfo> infos;
    QList<QtVersion *> knownVersions = QtVersionManager::instance()->versionsForTargetId(id, minimumQtVersion);

    foreach (QtVersion *version, knownVersions) {
        if (!version->isValid())
            continue;
        bool buildAll = version->defaultBuildConfig() & QtVersion::BuildAll;
        QtVersion::QmakeBuildConfigs config = buildAll ? QtVersion::BuildAll : QtVersion::QmakeBuildConfig(0);
        QString dir = QFileInfo(proFilePath).absolutePath(), id;
        if (id == Constants::S60_EMULATOR_TARGET_ID) {
            infos.append(BuildConfigurationInfo(version, config | QtVersion::DebugBuild, QString(), dir));
        } else {
            infos.append(BuildConfigurationInfo(version, config, QString(), dir));
            infos.append(BuildConfigurationInfo(version, config ^ QtVersion::DebugBuild, QString(), dir));
        }
    }

    return infos;
}

bool Qt4SymbianTargetFactory::isMobileTarget(const QString &id)
{
    Q_UNUSED(id)
    return true;
}

ProjectExplorer::Target *Qt4SymbianTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    QList<QtVersion *> knownVersions = QtVersionManager::instance()->versionsForTargetId(id);
    if (knownVersions.isEmpty())
        return 0;

    QtVersion *qtVersion = knownVersions.first();
    QtVersion::QmakeBuildConfigs config = qtVersion->defaultBuildConfig();

    QList<BuildConfigurationInfo> infos;
    if (id != Constants::S60_EMULATOR_TARGET_ID) {
        infos.append(BuildConfigurationInfo(qtVersion, config, QString(), QString()));
        infos.append(BuildConfigurationInfo(qtVersion, config ^ QtVersion::DebugBuild, QString(), QString()));
    } else {
        if(config & QtVersion::DebugBuild)
            infos.append(BuildConfigurationInfo(qtVersion, config, QString(), QString()));
        else
            infos.append(BuildConfigurationInfo(qtVersion, config ^ QtVersion::DebugBuild, QString(), QString()));
    }

    return create(parent, id, infos);
}

ProjectExplorer::Target *Qt4SymbianTargetFactory::create(ProjectExplorer::Project *parent, const QString &id, const QList<BuildConfigurationInfo> &infos)
{
    if (!canCreate(parent, id))
        return 0;
    Qt4SymbianTarget *t = new Qt4SymbianTarget(static_cast<Qt4Project *>(parent), id);
    foreach (const BuildConfigurationInfo &info, infos)
        t->addQt4BuildConfiguration(msgBuildConfigurationName(info),
                                    info.version, info.buildConfig,
                                    info.additionalArguments, info.directory);

    t->addDeployConfiguration(t->deployConfigurationFactory()->create(t, ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));

    t->createApplicationProFiles();

    if (t->runConfigurations().isEmpty())
        t->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(t));
    return t;
}

ProjectExplorer::Target *Qt4SymbianTargetFactory::create(ProjectExplorer::Project *parent, const QString &id, Qt4TargetSetupWidget *widget)
{
    if (!widget->isTargetSelected())
        return 0;
    Qt4DefaultTargetSetupWidget *w = static_cast<Qt4DefaultTargetSetupWidget *>(widget);
    return create(parent, id, w->buildConfigurationInfos());
}
