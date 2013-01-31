/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "qt4maemodeployconfiguration.h"

#include "debianmanager.h"
#include "maddeqemustartstep.h"
#include "maddeuploadandinstallpackagesteps.h"
#include "maemoconstants.h"
#include "maemodeploybymountsteps.h"
#include "maemoglobal.h"
#include "maemoinstalltosysrootstep.h"
#include "maemopackagecreationstep.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployablefile.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorer.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <remotelinux/remotelinuxcheckforfreediskspacestep.h>
#include <remotelinux/remotelinuxdeployconfigurationwidget.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QString>

#include <QMainWindow>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

Qt4MaemoDeployConfiguration::Qt4MaemoDeployConfiguration(ProjectExplorer::Target *target,
        const Core::Id id, const QString &displayName)
    : RemoteLinuxDeployConfiguration(target, id, displayName)
{
    init();
}

Qt4MaemoDeployConfiguration::Qt4MaemoDeployConfiguration(ProjectExplorer::Target *target,
        Qt4MaemoDeployConfiguration *source)
    : RemoteLinuxDeployConfiguration(target, source)
{
    init();
    cloneSteps(source);
}

NamedWidget *Qt4MaemoDeployConfiguration::createConfigWidget()
{
    return new RemoteLinuxDeployConfigurationWidget(this);
}

Qt4MaemoDeployConfiguration::~Qt4MaemoDeployConfiguration() {}

Core::Id Qt4MaemoDeployConfiguration::fremantleWithPackagingId()
{
    return Core::Id("DeployToFremantleWithPackaging");
}

Core::Id Qt4MaemoDeployConfiguration::fremantleWithoutPackagingId()
{
    return Core::Id("DeployToFremantleWithoutPackaging");
}

Core::Id Qt4MaemoDeployConfiguration::harmattanId()
{
    return Core::Id("DeployToHarmattan");
}

void Qt4MaemoDeployConfiguration::debianDirChanged(const Utils::FileName &dir)
{
    if (dir == DebianManager::debianDirectory(target()))
        emit packagingChanged();
}

void Qt4MaemoDeployConfiguration::setupPackaging()
{
    if (target()->project()->activeTarget() != target())
        return;

    disconnect(target()->project(), SIGNAL(fileListChanged()), this, SLOT(setupPackaging()));
    setupDebianPackaging();
}

void Qt4MaemoDeployConfiguration::setupDebianPackaging()
{
    Qt4BuildConfiguration *bc = qobject_cast<Qt4BuildConfiguration *>(target()->activeBuildConfiguration());
    if (!bc || !target()->kit())
        return;

    Utils::FileName debianDir = DebianManager::debianDirectory(target());
    DebianManager::ActionStatus status = DebianManager::createTemplate(bc, debianDir);

    if (status == DebianManager::ActionFailed)
        return;

    DebianManager * const dm = DebianManager::instance();
    dm->monitor(debianDir);
    connect(dm, SIGNAL(debianDirectoryChanged(Utils::FileName)), this,
            SLOT(debianDirChanged(Utils::FileName)));

    if (status == DebianManager::NoActionRequired)
        return;

    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target()->kit());
    QString projectName = target()->project()->displayName();

    if (!DebianManager::hasPackageManagerIcon(debianDir)) {
        // Such a file is created by the mobile wizards.
        Utils::FileName iconPath = Utils::FileName::fromString(target()->project()->projectDirectory());
        iconPath.appendPath(projectName + QLatin1String("64.png"));
        if (iconPath.toFileInfo().exists())
            dm->setPackageManagerIcon(debianDir, deviceType, iconPath);
    }

    // Set up aegis manifest on harmattan:
    if (deviceType == HarmattanOsType) {
        Utils::FileName manifest = debianDir;
        const QString manifestName = QLatin1String("manifest.aegis");
        manifest.appendPath(manifestName);
        const QFile aegisFile(manifest.toString());
        if (!aegisFile.exists()) {
            Utils::FileReader reader;
            if (!reader.fetch(Core::ICore::resourcePath()
                              + QLatin1String("/templates/shared/") + manifestName)) {
                qDebug("Reading manifest template failed.");
                return;
            }
            QString content = QString::fromUtf8(reader.data());
            content.replace(QLatin1String("%%PROJECTNAME%%"), projectName);
            Utils::FileSaver writer(aegisFile.fileName(), QIODevice::WriteOnly);
            writer.write(content.toUtf8());
            if (!writer.finalize()) {
                qDebug("Failure writing manifest file.");
                return;
            }
        }
    }

    emit packagingChanged();

    // fix path:
    QStringList files = DebianManager::debianFiles(debianDir);
    Utils::FileName path = Utils::FileName::fromString(QDir(target()->project()->projectDirectory())
                                                       .relativeFilePath(debianDir.toString()));
    QStringList relativeFiles;
    foreach (const QString &f, files) {
        Utils::FileName fn = path;
        fn.appendPath(f);
        relativeFiles << fn.toString();
    }

    addFilesToProject(relativeFiles);
}

void Qt4MaemoDeployConfiguration::addFilesToProject(const QStringList &files)
{
    if (files.isEmpty())
        return;

    const QString list = QLatin1String("<ul><li>") + files.join(QLatin1String("</li><li>"))
            + QLatin1String("</li></ul>");
    QMessageBox::StandardButton button =
            QMessageBox::question(Core::ICore::mainWindow(),
                                  tr("Add Packaging Files to Project"),
                                  tr("<html>Qt Creator has set up the following files to enable "
                                     "packaging:\n   %1\nDo you want to add them to the project?</html>")
                                  .arg(list), QMessageBox::Yes | QMessageBox::No);
    if (button == QMessageBox::Yes)
        ProjectExplorer::ProjectExplorerPlugin::instance()
                ->addExistingFiles(target()->project()->rootProjectNode(), files);
}

void Qt4MaemoDeployConfiguration::init()
{
    connect(target()->project(), SIGNAL(fileListChanged()), this, SLOT(setupPackaging()));
}

Qt4MaemoDeployConfigurationFactory::Qt4MaemoDeployConfigurationFactory(QObject *parent)
    : DeployConfigurationFactory(parent)
{ setObjectName(QLatin1String("Qt4MaemoDeployConfigurationFactory")); }

QList<Core::Id> Qt4MaemoDeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> ids;
    if (!canHandle(parent))
        return ids;

    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(parent->kit());
    if (deviceType == Maemo5OsType)
        ids << Qt4MaemoDeployConfiguration::fremantleWithPackagingId()
            << Qt4MaemoDeployConfiguration::fremantleWithoutPackagingId();
    else if (deviceType == HarmattanOsType)
        ids << Qt4MaemoDeployConfiguration::harmattanId();
    return ids;
}

QString Qt4MaemoDeployConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == Qt4MaemoDeployConfiguration::fremantleWithoutPackagingId())
        return tr("Copy Files to Maemo5 Device");
    else if (id == Qt4MaemoDeployConfiguration::fremantleWithPackagingId())
        return tr("Build Debian Package and Install to Maemo5 Device");
    else if (id == Qt4MaemoDeployConfiguration::harmattanId())
        return tr("Build Debian Package and Install to Harmattan Device");
    return QString();
}

bool Qt4MaemoDeployConfigurationFactory::canCreate(Target *parent,
    const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

DeployConfiguration *Qt4MaemoDeployConfigurationFactory::create(Target *parent,
    const Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));

    const QString displayName = displayNameForId(id);
    Qt4MaemoDeployConfiguration * const dc = new Qt4MaemoDeployConfiguration(parent, id, displayName);
    dc->setupDebianPackaging();

    if (id == Qt4MaemoDeployConfiguration::fremantleWithoutPackagingId()) {
        dc->stepList()->insertStep(0, new MaemoMakeInstallToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaddeQemuStartStep(dc->stepList()));
        dc->stepList()->insertStep(2, new RemoteLinuxCheckForFreeDiskSpaceStep(dc->stepList()));
        dc->stepList()->insertStep(3, new MaemoCopyFilesViaMountStep(dc->stepList()));
    } else if (id == Qt4MaemoDeployConfiguration::fremantleWithPackagingId()) {
        dc->stepList()->insertStep(0, new MaemoDebianPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoInstallDebianPackageToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(2, new MaddeQemuStartStep(dc->stepList()));
        dc->stepList()->insertStep(3, new RemoteLinuxCheckForFreeDiskSpaceStep(dc->stepList()));
        dc->stepList()->insertStep(4, new MaemoInstallPackageViaMountStep(dc->stepList()));
    } else if (id == Qt4MaemoDeployConfiguration::harmattanId()) {
        dc->stepList()->insertStep(0, new MaemoDebianPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoInstallDebianPackageToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(2, new MaddeQemuStartStep(dc->stepList()));
        dc->stepList()->insertStep(3, new RemoteLinuxCheckForFreeDiskSpaceStep(dc->stepList()));
        dc->stepList()->insertStep(4, new MaemoUploadAndInstallPackageStep(dc->stepList()));
    }
    return dc;
}

bool Qt4MaemoDeployConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    Core::Id id = idFromMap(map);
    return canHandle(parent) && availableCreationIds(parent).contains(id)
            && MaemoGlobal::supportsMaemoDevice(parent->kit());
}

DeployConfiguration *Qt4MaemoDeployConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    QTC_ASSERT(canRestore(parent, map), return 0);

    Qt4MaemoDeployConfiguration * const dc
        = qobject_cast<Qt4MaemoDeployConfiguration *>(create(parent, idFromMap(map)));
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

DeployConfiguration *Qt4MaemoDeployConfigurationFactory::clone(Target *parent,
    DeployConfiguration *product)
{
    if (!canClone(parent, product))
        return 0;
    return new Qt4MaemoDeployConfiguration(parent,
                                           qobject_cast<Qt4MaemoDeployConfiguration *>(product));
}

bool Qt4MaemoDeployConfigurationFactory::canHandle(Target *parent) const
{
    if (!qobject_cast<Qt4ProjectManager::Qt4Project *>(parent->project()))
        return false;
    if (!parent->project()->supportsKit(parent->kit()))
        return false;
    return MaemoGlobal::supportsMaemoDevice(parent->kit());
}

} // namespace Internal
} // namespace Madde
