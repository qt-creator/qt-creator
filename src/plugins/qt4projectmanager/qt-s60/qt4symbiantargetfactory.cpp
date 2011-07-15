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
#include <projectexplorer/toolchainmanager.h>

using ProjectExplorer::idFromMap;
using ProjectExplorer::Task;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

// -------------------------------------------------------------------------
// Qt4SymbianTargetFactory
// -------------------------------------------------------------------------

Qt4SymbianTargetFactory::Qt4SymbianTargetFactory(QObject *parent) :
    Qt4BaseTargetFactory(parent)
{
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>)),
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
    // The QtVersionManager will just check whether theres
    if (QtSupport::QtVersionManager::instance()->supportsTargetId(Constants::S60_DEVICE_TARGET_ID))
        ids << QLatin1String(Constants::S60_DEVICE_TARGET_ID);
    if (QtSupport::QtVersionManager::instance()->supportsTargetId(Constants::S60_EMULATOR_TARGET_ID))
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

QString Qt4SymbianTargetFactory::shadowBuildDirectory(const QString &profilePath, const QString &id, const QString &suffix)
{
    Q_UNUSED(id);
    Q_UNUSED(suffix);
    return QFileInfo(profilePath).absolutePath();
}

QList<ProjectExplorer::Task> Qt4SymbianTargetFactory::reportIssues(const QString &proFile)
{
    QList<ProjectExplorer::Task> results;
    // Warn of strange characters in project name and path:
    const QString projectName = proFile.mid(proFile.lastIndexOf(QLatin1Char('/')) + 1);
    QString projectPath = proFile.left(proFile.lastIndexOf(QLatin1Char('/')));
#if defined (Q_OS_WIN)
    if (projectPath.at(1) == QChar(':') && projectPath.at(0).toUpper() >= QChar('A') && projectPath.at(0).toUpper() <= QChar('Z'))
        projectPath = projectPath.mid(2);
#endif
    if (projectPath.contains(QLatin1Char(' '))) {
        results.append(Task(Task::Warning,
                            QCoreApplication::translate("ProjectExplorer::Internal::S60ProjectChecker",
                                                        "The Symbian tool chain does not handle spaces "
                                                        "in the project path '%1'.").arg(projectPath),
                            QString(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    if (projectName.contains(QRegExp("[^a-zA-Z0-9.-]"))) {
        results.append(Task(Task::Warning,
                            QCoreApplication::translate("ProjectExplorer::Internal::S60ProjectChecker",
                                                        "The Symbian tool chain does not handle special "
                                                        "characters in the project name '%1' well.")
                            .arg(projectName),
                            QString(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    return results;
}

QList<BuildConfigurationInfo> Qt4SymbianTargetFactory::availableBuildConfigurations(const QString &id, const QString &proFilePath, const QtSupport::QtVersionNumber &minimumQtVersion)
{
    QList<BuildConfigurationInfo> infos = Qt4BaseTargetFactory::availableBuildConfigurations(id, proFilePath, minimumQtVersion);
    if (id != Constants::S60_EMULATOR_TARGET_ID)
        return infos;
    // For emulator filter out all non debug builds
    QList<BuildConfigurationInfo> tmp;
    foreach (const BuildConfigurationInfo &info, infos)
        if (info.buildConfig & QtSupport::BaseQtVersion::DebugBuild)
            tmp << info;
    return tmp;
}

bool Qt4SymbianTargetFactory::selectByDefault(const QString &id) const
{
    return id != QLatin1String(Constants::S60_EMULATOR_TARGET_ID);
}

QSet<QString> Qt4SymbianTargetFactory::targetFeatures(const QString & /*id*/) const
{
    QSet<QString> features;
    features << Constants::MOBILE_TARGETFEATURE_ID;
    // ideally we should check whether they're really installed
    features << Constants::QTQUICKCOMPONENTS_SYMBIAN_TARGETFEATURE_ID;
    return features;
}

ProjectExplorer::Target *Qt4SymbianTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    QList<QtSupport::BaseQtVersion *> knownVersions = QtSupport::QtVersionManager::instance()->versionsForTargetId(id);
    if (knownVersions.isEmpty())
        return 0;

    QtSupport::BaseQtVersion *qtVersion = knownVersions.first();
    QtSupport::BaseQtVersion::QmakeBuildConfigs config = qtVersion->defaultBuildConfig();

    QList<BuildConfigurationInfo> infos;
    if (id != Constants::S60_EMULATOR_TARGET_ID) {
        infos.append(BuildConfigurationInfo(qtVersion, config, QString(), QString()));
        infos.append(BuildConfigurationInfo(qtVersion, config ^ QtSupport::BaseQtVersion::DebugBuild, QString(), QString()));
    } else {
        if (config & QtSupport::BaseQtVersion::DebugBuild)
            infos.append(BuildConfigurationInfo(qtVersion, config, QString(), QString()));
        else
            infos.append(BuildConfigurationInfo(qtVersion, config ^ QtSupport::BaseQtVersion::DebugBuild, QString(), QString()));
    }

    return create(parent, id, infos);
}

ProjectExplorer::Target *Qt4SymbianTargetFactory::create(ProjectExplorer::Project *parent, const QString &id, const QList<BuildConfigurationInfo> &infos)
{
    if (!canCreate(parent, id))
        return 0;
    Qt4SymbianTarget *t = new Qt4SymbianTarget(static_cast<Qt4Project *>(parent), id);
    foreach (const BuildConfigurationInfo &info, infos)
        t->addQt4BuildConfiguration(msgBuildConfigurationName(info), QString(),
                                    info.version, info.buildConfig,
                                    info.additionalArguments, info.directory);

    t->addDeployConfiguration(t->createDeployConfiguration(QLatin1String(Qt4ProjectManager::Constants::S60_DEVICE_TARGET_ID)));

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
