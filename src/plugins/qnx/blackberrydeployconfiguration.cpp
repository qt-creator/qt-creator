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

#include "utils/checkablemessagebox.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorer.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qtsupport/qtkitinformation.h>
#include <coreplugin/icore.h>
#include <ssh/sshconnection.h>

#include <QMessageBox>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char DEPLOYMENT_INFO_SETTING[] = "Qnx.BlackBerry.DeploymentInfo";
const char DEPLOYMENT_INFO_KEY[]     = "Qnx.BlackBerry.DeployInformation";
const char BAR_DESC_SETUP[]          = "Qnx.BlackBerry.DeployInformation.BarDescriptorSetup";
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
    m_deployInformation = new BlackBerryDeployInformation(target());
    m_appBarDesciptorSetup = false;

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

    QString targetName;
    Qt4ProjectManager::Qt4Project *project =  static_cast<Qt4ProjectManager::Qt4Project *>(target()->project());
    foreach (Qt4ProjectManager::Qt4ProFileNode *node, project->applicationProFiles()) {
        QString target = node->targetInformation().target;
        if (!target.isEmpty()) {
            targetName = target;
            break;
        }
    }

    if (deviceType == Constants::QNX_BB_OS_TYPE) {
        const QLatin1String barDescriptorFileName("bar-descriptor.xml");
        Utils::FileName barDescriptorPath = Utils::FileName::fromString(target()->project()->projectDirectory()).appendPath(barDescriptorFileName);
        const QFile barDescriptorFile(barDescriptorPath.toString());
        if (barDescriptorFile.exists())
            return;

        if (m_appBarDesciptorSetup)
            return;

        QDialogButtonBox::StandardButton button = Utils::CheckableMessageBox::question(Core::ICore::mainWindow(),
                                             tr("Setup Application Descriptor File"),
                                             tr("You need to set up a bar descriptor file to enable "
                                                "packaging.\nDo you want Qt Creator to generate it for your project?"),
                                             tr("Don't ask again for this project"), &m_appBarDesciptorSetup);

        if (button == QDialogButtonBox::No)
            return;

        QString barDescriptorTemplate;
        QtSupport::QtVersionNumber qtVersion = QtSupport::QtKitInformation::qtVersion(target()->kit())->qtVersion();
        if (qtVersion >= QtSupport::QtVersionNumber(5, 0, 0))
            barDescriptorTemplate = Core::ICore::resourcePath()
                    + QLatin1String("/templates/wizards/bb-qt5-bardescriptor/bar-descriptor.xml");
        else
            barDescriptorTemplate = Core::ICore::resourcePath()
                    + QLatin1String("/templates/wizards/bb-bardescriptor/bar-descriptor.xml");

        Utils::FileReader reader;
        if (!reader.fetch(barDescriptorTemplate)) {
            QMessageBox::warning(Core::ICore::mainWindow(),
                                 tr("Cannot Set up Application Descriptor File"),
                                 tr("Reading the bar descriptor template failed."),
                                 QMessageBox::Ok);
            return;
        }

        QString content = QString::fromUtf8(reader.data());
        content.replace(QLatin1String("PROJECTNAME"), projectName);
        content.replace(QLatin1String("PROJECTPATH"), targetName);
        content.replace(QLatin1String("ID"), QLatin1String("com.example.") + projectName);

        if (Utils::FileName::fromString(target()->project()->projectDirectory())
                 .appendPath(QLatin1String("qml")).toFileInfo().exists())
            content.replace(QLatin1String("</qnx>"),
                            QLatin1String("    <asset path=\"%SRC_DIR%/qml\">qml</asset>\n</qnx>"));

        Utils::FileSaver writer(barDescriptorFile.fileName(), QIODevice::WriteOnly);
        writer.write(content.toUtf8());
        if (!writer.finalize()) {
            QMessageBox::warning(Core::ICore::mainWindow(),
                                 tr("Cannot Set up Application Descriptor File"),
                                 tr("Writing the bar descriptor file failed."),
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

    ProjectExplorer::ProjectExplorerPlugin::instance()
            ->addExistingFiles(target()->project()->rootProjectNode(), QStringList() << barDesciptorPath);
}

BlackBerryDeployConfiguration::~BlackBerryDeployConfiguration()
{
}

BlackBerryDeployInformation *BlackBerryDeployConfiguration::deploymentInfo() const
{
    return m_deployInformation;
}

ProjectExplorer::NamedWidget *BlackBerryDeployConfiguration::createConfigWidget()
{
    return new BlackBerryDeployConfigurationWidget(this);
}

QVariantMap BlackBerryDeployConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::DeployConfiguration::toMap());
    map.insert(QLatin1String(DEPLOYMENT_INFO_KEY), deploymentInfo()->toMap());
    map.insert(QLatin1String(BAR_DESC_SETUP), m_appBarDesciptorSetup);
    return map;
}

bool BlackBerryDeployConfiguration::fromMap(const QVariantMap &map)
{
    if (!ProjectExplorer::DeployConfiguration::fromMap(map))
        return false;

    m_appBarDesciptorSetup = map.value(QLatin1String(BAR_DESC_SETUP)).toBool();
    QVariantMap deployInfoMap = map.value(QLatin1String(DEPLOYMENT_INFO_KEY)).toMap();
    deploymentInfo()->fromMap(deployInfoMap);
    return true;
}
