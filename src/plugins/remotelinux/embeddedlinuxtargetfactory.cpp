/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "embeddedlinuxtargetfactory.h"

#include "remotelinuxdeployconfiguration.h"
#include "remotelinuxdeployconfigurationfactory.h"
#include "embeddedlinuxtarget.h"
#include "remotelinux_constants.h"

#include <projectexplorer/customexecutablerunconfiguration.h>

#include <qt4projectmanager/buildconfigurationinfo.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <qtsupport/qtversionmanager.h>

#include <QtGui/QIcon>

namespace RemoteLinux {
namespace Internal {

EmbeddedLinuxTargetFactory::EmbeddedLinuxTargetFactory(QObject *parent) :
    Qt4ProjectManager::Qt4BaseTargetFactory(parent)
{ }

EmbeddedLinuxTargetFactory::~EmbeddedLinuxTargetFactory()
{ }

QIcon EmbeddedLinuxTargetFactory::iconForId(const QString &id) const
{
    if (id == QLatin1String(Constants::EMBEDDED_LINUX_TARGET_ID))
        return QIcon(":/remotelinux/images/embeddedtarget.png");
    return QIcon();
}

QString EmbeddedLinuxTargetFactory::buildNameForId(const QString &id) const
{
    if (supportsTargetId(id))
        return tr("embedded");
    return QString();
}

QSet<QString> EmbeddedLinuxTargetFactory::targetFeatures(const QString & /*id*/) const
{
    QSet<QString> features;
    features << Qt4ProjectManager::Constants::MOBILE_TARGETFEATURE_ID;
    features << Qt4ProjectManager::Constants::SHADOWBUILD_TARGETFEATURE_ID;

    return features;
}

QStringList EmbeddedLinuxTargetFactory::supportedTargetIds(ProjectExplorer::Project *project) const
{
    Q_UNUSED(project);
    if (QtSupport::QtVersionManager::instance()->supportsTargetId(RemoteLinux::Constants::EMBEDDED_LINUX_TARGET_ID))
        return QStringList() << QLatin1String(RemoteLinux::Constants::EMBEDDED_LINUX_TARGET_ID);
    return QStringList();
}

bool EmbeddedLinuxTargetFactory::supportsTargetId(const QString &id) const
{
    return id == RemoteLinux::Constants::EMBEDDED_LINUX_TARGET_ID;
}

QString EmbeddedLinuxTargetFactory::displayNameForId(const QString &id) const
{
    if (id == RemoteLinux::Constants::EMBEDDED_LINUX_TARGET_ID)
        return tr("Embedded Linux");
    return QString();
}

bool EmbeddedLinuxTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::Target *EmbeddedLinuxTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));

    EmbeddedLinuxTarget *t = new EmbeddedLinuxTarget(static_cast<Qt4ProjectManager::Qt4Project *>(parent),
                                                     Constants::EMBEDDED_LINUX_TARGET_ID);
    if (t->fromMap(map))
        return t;

    delete t;
    return 0;
}

bool EmbeddedLinuxTargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    Qt4ProjectManager::Qt4Project *project = qobject_cast<Qt4ProjectManager::Qt4Project *>(parent);
    if (!project)
        return false;

    return supportsTargetId(id);
}

ProjectExplorer::Target *EmbeddedLinuxTargetFactory::create(ProjectExplorer::Project *parent,
                                                            const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    QList<QtSupport::BaseQtVersion *> knownVersions =
            QtSupport::QtVersionManager::instance()->versionsForTargetId(id);
    QtSupport::BaseQtVersion *qtVersion = knownVersions.first();
    QtSupport::BaseQtVersion::QmakeBuildConfigs config = qtVersion->defaultBuildConfig();

    QList<Qt4ProjectManager::BuildConfigurationInfo> infos;
    infos.append(Qt4ProjectManager::BuildConfigurationInfo(qtVersion, config, QString(), QString()));
    infos.append(Qt4ProjectManager::BuildConfigurationInfo(qtVersion,
                                                           config ^ QtSupport::BaseQtVersion::DebugBuild,
                                                           QString(), QString()));

    return create(parent, id, infos);
}

ProjectExplorer::Target *EmbeddedLinuxTargetFactory::create(ProjectExplorer::Project *parent,
                                                            const QString &id,
                                                            const QList<Qt4ProjectManager::BuildConfigurationInfo> &infos)
{
    if (!canCreate(parent, id) || infos.isEmpty())
        return 0;

    EmbeddedLinuxTarget *t = new EmbeddedLinuxTarget(static_cast<Qt4ProjectManager::Qt4Project *>(parent), id);

    foreach (const Qt4ProjectManager::BuildConfigurationInfo &info, infos)
        t->addQt4BuildConfiguration(msgBuildConfigurationName(info), QString(),
                                    info.version, info.buildConfig,
                                    info.additionalArguments, info.directory, info.importing);

    t->addDeployConfiguration(
                t->createDeployConfiguration(
                    RemoteLinuxDeployConfigurationFactory::genericDeployConfigurationId()));

    t->createApplicationProFiles();

    if (t->runConfigurations().isEmpty())
        t->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(t));
    return t;
}

} // namespace Internal
} // namespace RemoteLinux
