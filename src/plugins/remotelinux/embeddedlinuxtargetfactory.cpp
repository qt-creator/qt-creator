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

#include "embeddedlinuxtargetfactory.h"

#include "remotelinuxdeployconfiguration.h"
#include "remotelinuxdeployconfigurationfactory.h"
#include "genericembeddedlinuxtarget.h"
#include "remotelinux_constants.h"

#include <qt4projectmanager/buildconfigurationinfo.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <qtsupport/customexecutablerunconfiguration.h>
#include <qtsupport/qtversionmanager.h>

#include <QIcon>

namespace RemoteLinux {
namespace Internal {

EmbeddedLinuxTargetFactory::EmbeddedLinuxTargetFactory(QObject *parent) :
    Qt4ProjectManager::Qt4BaseTargetFactory(parent)
{ }

EmbeddedLinuxTargetFactory::~EmbeddedLinuxTargetFactory()
{ }

QIcon EmbeddedLinuxTargetFactory::iconForId(const Core::Id id) const
{
    if (id == Core::Id(Constants::EMBEDDED_LINUX_TARGET_ID))
        return QIcon(":/remotelinux/images/embeddedtarget.png");
    return QIcon();
}

QString EmbeddedLinuxTargetFactory::buildNameForId(const Core::Id id) const
{
    if (supportsTargetId(id))
        return tr("embedded");
    return QString();
}

QSet<QString> EmbeddedLinuxTargetFactory::targetFeatures(const Core::Id id) const
{
    Q_UNUSED(id);
    QSet<QString> features;
    features << Qt4ProjectManager::Constants::MOBILE_TARGETFEATURE_ID;
    features << Qt4ProjectManager::Constants::SHADOWBUILD_TARGETFEATURE_ID;

    return features;
}

QList<Core::Id> EmbeddedLinuxTargetFactory::supportedTargetIds() const
{
    return QList<Core::Id>() << Core::Id(RemoteLinux::Constants::EMBEDDED_LINUX_TARGET_ID);
}

bool EmbeddedLinuxTargetFactory::supportsTargetId(const Core::Id id) const
{
    return id == Core::Id(RemoteLinux::Constants::EMBEDDED_LINUX_TARGET_ID);
}

QString EmbeddedLinuxTargetFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(RemoteLinux::Constants::EMBEDDED_LINUX_TARGET_ID))
        return tr("Embedded Linux");
    return QString();
}

bool EmbeddedLinuxTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return qobject_cast<Qt4ProjectManager::Qt4Project *>(parent) && supportsTargetId(ProjectExplorer::idFromMap(map));
}

ProjectExplorer::Target *EmbeddedLinuxTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));

    GenericEmbeddedLinuxTarget *t =
            new GenericEmbeddedLinuxTarget(static_cast<Qt4ProjectManager::Qt4Project *>(parent), Core::Id());
    if (t->fromMap(map))
        return t;

    delete t;
    return 0;
}

bool EmbeddedLinuxTargetFactory::canCreate(ProjectExplorer::Project *parent, const Core::Id id) const
{
    Qt4ProjectManager::Qt4Project *project = qobject_cast<Qt4ProjectManager::Qt4Project *>(parent);
    if (!project)
        return false;

    if (!supportsTargetId(id))
        return false;

    return QtSupport::QtVersionManager::instance()->supportsTargetId(id);
}

ProjectExplorer::Target *EmbeddedLinuxTargetFactory::create(ProjectExplorer::Project *parent,
                                                            const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    QList<QtSupport::BaseQtVersion *> knownVersions =
            QtSupport::QtVersionManager::instance()->versionsForTargetId(id);
    QtSupport::BaseQtVersion *qtVersion = knownVersions.first();
    QtSupport::BaseQtVersion::QmakeBuildConfigs config = qtVersion->defaultBuildConfig();

    QList<Qt4ProjectManager::BuildConfigurationInfo> infos;
    infos.append(Qt4ProjectManager::BuildConfigurationInfo(qtVersion->uniqueId(), config, QString(), QString()));
    infos.append(Qt4ProjectManager::BuildConfigurationInfo(qtVersion->uniqueId(),
                                                           config ^ QtSupport::BaseQtVersion::DebugBuild,
                                                           QString(), QString()));

    return create(parent, id, infos);
}

ProjectExplorer::Target *EmbeddedLinuxTargetFactory::create(ProjectExplorer::Project *parent,
                                                            const Core::Id id,
                                                            const QList<Qt4ProjectManager::BuildConfigurationInfo> &infos)
{
    if (!canCreate(parent, id) || infos.isEmpty())
        return 0;

    GenericEmbeddedLinuxTarget *t = new GenericEmbeddedLinuxTarget(static_cast<Qt4ProjectManager::Qt4Project *>(parent), id);

    foreach (const Qt4ProjectManager::BuildConfigurationInfo &info, infos)
        t->addQt4BuildConfiguration(msgBuildConfigurationName(info), QString(),
                                    info.version(), info.buildConfig,
                                    info.additionalArguments, info.directory, info.importing);

    t->addDeployConfiguration(
                t->createDeployConfiguration(
                    RemoteLinuxDeployConfigurationFactory::genericDeployConfigurationId()));

    t->createApplicationProFiles(false);

    if (t->runConfigurations().isEmpty())
        t->addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(t));
    return t;
}

} // namespace Internal
} // namespace RemoteLinux
