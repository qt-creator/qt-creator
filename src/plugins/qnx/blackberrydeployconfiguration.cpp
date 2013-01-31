/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberrydeployconfiguration.h"

#include "qnxconstants.h"
#include "blackberrydeployconfigurationwidget.h"
#include "blackberrydeployinformation.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorer.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <coreplugin/icore.h>
#include <ssh/sshconnection.h>

#include <QMessageBox>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char DEPLOYMENT_INFO_SETTING[] = "Qnx.BlackBerry.DeploymentInfo";
const char DEPLOYMENT_INFO_KEY[]     = "Qnx.BlackBerry.DeployInformation";
}

BlackBerryDeployConfiguration::BlackBerryDeployConfiguration(ProjectExplorer::Target *parent)
    : ProjectExplorer::DeployConfiguration(parent, Core::Id(Constants::QNX_BB_DEPLOYCONFIGURATION_ID))
{
    ctor();
}

BlackBerryDeployConfiguration::BlackBerryDeployConfiguration(ProjectExplorer::Target *parent,
                                               BlackBerryDeployConfiguration *source)
    : ProjectExplorer::DeployConfiguration(parent, source)
{
    ctor();
    cloneSteps(source);
}

void BlackBerryDeployConfiguration::ctor()
{
    BlackBerryDeployInformation *info
            = qobject_cast<BlackBerryDeployInformation *>(target()->project()->namedSettings(QLatin1String(DEPLOYMENT_INFO_SETTING)).value<QObject *>());
    if (!info) {
        info = new BlackBerryDeployInformation(static_cast<Qt4ProjectManager::Qt4Project *>(target()->project()));
        QVariant data = QVariant::fromValue(static_cast<QObject *>(info));
        target()->project()->setNamedSettings(QLatin1String(DEPLOYMENT_INFO_SETTING), data);
    }

    connect(target()->project(), SIGNAL(proFilesEvaluated()), this, SLOT(setupBarDescriptor()), Qt::UniqueConnection);

    setDefaultDisplayName(tr("Deploy to BlackBerry Device"));
}

void BlackBerryDeployConfiguration::setupBarDescriptor()
{
    Qt4ProjectManager::Qt4BuildConfiguration *bc = qobject_cast<Qt4ProjectManager::Qt4BuildConfiguration *>(target()->activeBuildConfiguration());
    if (!bc || !target()->kit())
        return;

    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target()->kit());
    QString projectName = target()->project()->displayName();

    if (deviceType == Constants::QNX_BB_OS_TYPE) {
        const QLatin1String barDescriptorFileName("bar-descriptor.xml");
        Utils::FileName barDescriptorPath = Utils::FileName::fromString(target()->project()->projectDirectory()).appendPath(barDescriptorFileName);
        const QFile barDescriptorFile(barDescriptorPath.toString());
        if (barDescriptorFile.exists())
            return;

        Utils::FileReader reader;
        QString barDescriptorTemplate;
        if (QDir(Utils::FileName::fromString(target()->project()->projectDirectory()).appendPath(QLatin1String("qml")).toString()).exists())
            barDescriptorTemplate = Core::ICore::resourcePath()
                    + QLatin1String("/templates/wizards/bb-quickapp/") + barDescriptorFileName;
        else
            barDescriptorTemplate = Core::ICore::resourcePath()
                    + QLatin1String("/templates/wizards/bb-guiapp/") + barDescriptorFileName;


        if (!reader.fetch(barDescriptorTemplate)) {
            QMessageBox::warning(Core::ICore::mainWindow(),
                                 tr("Error while setting up bar descriptor"),
                                 tr("Reading bar descriptor template failed"),
                                 QMessageBox::Ok);
            return;
        }

        QString content = QString::fromUtf8(reader.data());
        content.replace(QLatin1String("%ProjectName%"), projectName);
        Utils::FileSaver writer(barDescriptorFile.fileName(), QIODevice::WriteOnly);
        writer.write(content.toUtf8());
        if (!writer.finalize()) {
            QMessageBox::warning(Core::ICore::mainWindow(),
                                 tr("Error while setting up bar descriptor"),
                                 tr("Failure writing bar descriptor file."),
                                 QMessageBox::Ok);
            return;
        }

        // Add the Bar Descriptor to the existing project
        if (target()->project()->rootProjectNode())
            addBarDescriptorToProject(barDescriptorPath.toString());
    }
}

void BlackBerryDeployConfiguration::addBarDescriptorToProject(const QString &barDesciptorPath)
{
    if (barDesciptorPath.isEmpty())
        return;

    QMessageBox::StandardButton button =
            QMessageBox::question(Core::ICore::mainWindow(),
                                  tr("Add bar-descriptor.xml file to project"),
                                  tr("Qt Creator has set up a bar descriptor file to enable "
                                     "packaging.\nDo you want to add it to the project?"),
                                  QMessageBox::Yes | QMessageBox::No);
    if (button == QMessageBox::Yes)
        ProjectExplorer::ProjectExplorerPlugin::instance()
                ->addExistingFiles(target()->project()->rootProjectNode(), QStringList() << barDesciptorPath);
}

BlackBerryDeployConfiguration::~BlackBerryDeployConfiguration()
{
}

BlackBerryDeployInformation *BlackBerryDeployConfiguration::deploymentInfo() const
{
    BlackBerryDeployInformation *info
            = qobject_cast<BlackBerryDeployInformation *>(target()->project()->namedSettings(QLatin1String(DEPLOYMENT_INFO_SETTING)).value<QObject *>());
    return info;
}

ProjectExplorer::NamedWidget *BlackBerryDeployConfiguration::createConfigWidget()
{
    return new BlackBerryDeployConfigurationWidget(this);
}

QVariantMap BlackBerryDeployConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::DeployConfiguration::toMap());
    map.insert(QLatin1String(DEPLOYMENT_INFO_KEY), deploymentInfo()->toMap());
    return map;
}

bool BlackBerryDeployConfiguration::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::DeployConfiguration::fromMap(map))
        return false;

    QVariantMap deployInfoMap = map.value(QLatin1String(DEPLOYMENT_INFO_KEY)).toMap();
    deploymentInfo()->fromMap(deployInfoMap);
    return true;
}
