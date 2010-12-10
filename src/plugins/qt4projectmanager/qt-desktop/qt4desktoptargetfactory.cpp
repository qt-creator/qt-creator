/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qt4desktoptargetfactory.h"
#include "qt4projectmanagerconstants.h"
#include "qt4project.h"
#include "qt4runconfiguration.h"
#include "qt4desktoptarget.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtGui/QApplication>
#include <QtGui/QStyle>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::idFromMap;

Qt4DesktopTargetFactory::Qt4DesktopTargetFactory(QObject *parent) :
    Qt4BaseTargetFactory(parent)
{
    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>)),
            this, SIGNAL(supportedTargetIdsChanged()));
}

Qt4DesktopTargetFactory::~Qt4DesktopTargetFactory()
{
}

bool Qt4DesktopTargetFactory::supportsTargetId(const QString &id) const
{
    return id == QLatin1String(Constants::DESKTOP_TARGET_ID);
}

QStringList Qt4DesktopTargetFactory::supportedTargetIds(ProjectExplorer::Project *parent) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return QStringList();
    if (!QtVersionManager::instance()->supportsTargetId(Constants::DESKTOP_TARGET_ID))
        return QStringList();
    return QStringList() << QLatin1String(Constants::DESKTOP_TARGET_ID);
}

QString Qt4DesktopTargetFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(Constants::DESKTOP_TARGET_ID))
        return Qt4DesktopTarget::defaultDisplayName();
    return QString();
}

bool Qt4DesktopTargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return false;
    return supportsTargetId(id);
}

bool Qt4DesktopTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

Qt4BaseTarget *Qt4DesktopTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Qt4Project *qt4project = static_cast<Qt4Project *>(parent);
    Qt4DesktopTarget *target = new Qt4DesktopTarget(qt4project, QLatin1String("transient ID"));

    if (target->fromMap(map))
        return target;
    delete target;
    return 0;
}

QString Qt4DesktopTargetFactory::defaultShadowBuildDirectory(const QString &projectLocation, const QString &id)
{
    if (id != QLatin1String(Constants::DESKTOP_TARGET_ID))
        return QString();

    // currently we can't have the build directory to be deeper than the source directory
    // since that is broken in qmake
    // Once qmake is fixed we can change that to have a top directory and
    // subdirectories per build. (Replacing "QChar('-')" with "QChar('/') )
    return projectLocation + QLatin1String("-desktop");
}

QList<BuildConfigurationInfo> Qt4DesktopTargetFactory::availableBuildConfigurations(const QString &proFilePath)
{
    QList<BuildConfigurationInfo> infos;
    QList<QtVersion *> knownVersions = QtVersionManager::instance()->versionsForTargetId(Constants::DESKTOP_TARGET_ID);

    foreach (QtVersion *version, knownVersions) {
        bool buildAll = version->defaultBuildConfig() & QtVersion::BuildAll;
        QtVersion::QmakeBuildConfigs config = buildAll ? QtVersion::BuildAll : QtVersion::QmakeBuildConfig(0);

        QString dir = defaultShadowBuildDirectory(Qt4Project::defaultTopLevelBuildDirectory(proFilePath), Constants::DESKTOP_TARGET_ID);
        infos.append(BuildConfigurationInfo(version, config, QString(), dir));
        infos.append(BuildConfigurationInfo(version, config | QtVersion::DebugBuild, QString(), dir));
    }
    return infos;
}

Qt4BaseTarget *Qt4DesktopTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    QList<QtVersion *> knownVersions = QtVersionManager::instance()->versionsForTargetId(id);
    if (knownVersions.isEmpty())
        return 0;

    QtVersion *qtVersion = knownVersions.first();
    bool buildAll = qtVersion->isValid() && (qtVersion->defaultBuildConfig() & QtVersion::BuildAll);
    QtVersion::QmakeBuildConfigs config = buildAll ? QtVersion::BuildAll : QtVersion::QmakeBuildConfig(0);

    QList<BuildConfigurationInfo> infos;
    infos.append(BuildConfigurationInfo(qtVersion, config | QtVersion::DebugBuild, QString(), QString()));
    infos.append(BuildConfigurationInfo(qtVersion, config, QString(), QString()));

    return create(parent, id, infos);
}

Qt4BaseTarget *Qt4DesktopTargetFactory::create(ProjectExplorer::Project *parent, const QString &id, QList<BuildConfigurationInfo> infos)
{
    if (!canCreate(parent, id))
        return 0;
    Qt4DesktopTarget *t = new Qt4DesktopTarget(static_cast<Qt4Project *>(parent), id);

    foreach (const BuildConfigurationInfo &info, infos) {
        QString displayName = info.version->displayName() + QLatin1Char(' ');
        displayName += (info.buildConfig & QtVersion::DebugBuild) ? tr("Debug") : tr("Release");
        t->addQt4BuildConfiguration(displayName,
                                    info.version,
                                    info.buildConfig,
                                    info.additionalArguments,
                                    info.directory);
    }

    t->addDeployConfiguration(t->deployConfigurationFactory()->create(t, ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));

    t->createApplicationProFiles();

    if (t->runConfigurations().isEmpty())
        t->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(t));
    return t;
}
