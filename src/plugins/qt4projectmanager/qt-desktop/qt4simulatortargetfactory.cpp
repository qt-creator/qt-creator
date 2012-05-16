/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qt4simulatortargetfactory.h"
#include "buildconfigurationinfo.h"
#include "qt4projectmanagerconstants.h"
#include "qt4project.h"
#include "qt4runconfiguration.h"
#include "qt4simulatortarget.h"
#include <qtsupport/qtversionmanager.h>

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/customexecutablerunconfiguration.h>

#include <QApplication>
#include <QStyle>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::idFromMap;

// -------------------------------------------------------------------------
// Qt4SimulatorTargetFactory
// -------------------------------------------------------------------------

Qt4SimulatorTargetFactory::Qt4SimulatorTargetFactory(QObject *parent) :
    Qt4BaseTargetFactory(parent)
{
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SIGNAL(canCreateTargetIdsChanged()));
}

Qt4SimulatorTargetFactory::~Qt4SimulatorTargetFactory()
{
}

bool Qt4SimulatorTargetFactory::supportsTargetId(const Core::Id id) const
{
    return id == Core::Id(Constants::QT_SIMULATOR_TARGET_ID);
}

QList<Core::Id> Qt4SimulatorTargetFactory::supportedTargetIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::QT_SIMULATOR_TARGET_ID);
}

QString Qt4SimulatorTargetFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(Constants::QT_SIMULATOR_TARGET_ID))
        return Qt4SimulatorTarget::defaultDisplayName();
    return QString();
}

QIcon Qt4SimulatorTargetFactory::iconForId(const Core::Id id) const
{
    if (id == Core::Id(Constants::QT_SIMULATOR_TARGET_ID))
        return QIcon(QLatin1String(":/projectexplorer/images/SymbianEmulator.png"));
    return QIcon();
}

QString Qt4SimulatorTargetFactory::buildNameForId(const Core::Id id) const
{
    if (id == Core::Id(Constants::QT_SIMULATOR_TARGET_ID))
        return QLatin1String("simulator");
    return QString();
}

bool Qt4SimulatorTargetFactory::canCreate(ProjectExplorer::Project *parent, const Core::Id id) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return false;
    if (!supportsTargetId(id))
        return false;
    return QtSupport::QtVersionManager::instance()->supportsTargetId(id);
}

bool Qt4SimulatorTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return qobject_cast<Qt4Project *>(parent) && supportsTargetId(idFromMap(map));
}

ProjectExplorer::Target *Qt4SimulatorTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Qt4Project *qt4project = static_cast<Qt4Project *>(parent);
    Qt4SimulatorTarget *target = new Qt4SimulatorTarget(qt4project, idFromMap(map));
    if (target->fromMap(map))
        return target;
    delete target;
    return 0;
}

QSet<QString> Qt4SimulatorTargetFactory::targetFeatures(const Core::Id /*id*/) const
{
    QSet<QString> features;

    features << QLatin1String(Constants::MOBILE_TARGETFEATURE_ID)
             << QLatin1String(Constants::SHADOWBUILD_TARGETFEATURE_ID);

    return features;
}

ProjectExplorer::Target *Qt4SimulatorTargetFactory::create(ProjectExplorer::Project *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    QList<QtSupport::BaseQtVersion *> knownVersions = QtSupport::QtVersionManager::instance()->versionsForTargetId(id);
    if (knownVersions.isEmpty())
        return 0;

    QtSupport::BaseQtVersion *qtVersion = knownVersions.first();
    QtSupport::BaseQtVersion::QmakeBuildConfigs config = qtVersion->defaultBuildConfig();
    QList<BuildConfigurationInfo> infos;
    infos.append(BuildConfigurationInfo(qtVersion->uniqueId(), config, QString(), QString()));
    infos.append(BuildConfigurationInfo(qtVersion->uniqueId(), config ^ QtSupport::BaseQtVersion::DebugBuild, QString(), QString()));

    return create(parent, id, infos);
}

ProjectExplorer::Target *Qt4SimulatorTargetFactory::create(ProjectExplorer::Project *parent, const Core::Id id,
                                                           const QList<BuildConfigurationInfo> &infos)
{
    if (!canCreate(parent, id))
        return 0;
    if (infos.isEmpty())
        return 0;
    Qt4SimulatorTarget *t = new Qt4SimulatorTarget(static_cast<Qt4Project *>(parent), id);

    foreach (const BuildConfigurationInfo &info, infos)
        t->addQt4BuildConfiguration(msgBuildConfigurationName(info), QString(), info.version(), info.buildConfig,
                                    info.additionalArguments, info.directory, info.importing);

    t->addDeployConfiguration(t->createDeployConfiguration(Core::Id(ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID)));

    t->createApplicationProFiles(false);

    if (t->runConfigurations().isEmpty())
        t->addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(t));
    return t;
}
