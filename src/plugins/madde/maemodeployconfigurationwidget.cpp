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
#include "maemodeployconfigurationwidget.h"
#include "ui_maemodeployconfigurationwidget.h"

#include "maemoglobal.h"
#include "maemoconstants.h"
#include "qt4maemodeployconfiguration.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4nodes.h>
#include <remotelinux/deployablefile.h>
#include <remotelinux/deployablefilesperprofile.h>
#include <remotelinux/deploymentinfo.h>
#include <remotelinux/deploymentsettingsassistant.h>
#include <remotelinux/remotelinuxdeployconfigurationwidget.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

MaemoDeployConfigurationWidget::MaemoDeployConfigurationWidget(QWidget *parent)
    : DeployConfigurationWidget(parent),
      ui(new Ui::MaemoDeployConfigurationWidget),
      m_remoteLinuxWidget(new RemoteLinuxDeployConfigurationWidget)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_remoteLinuxWidget);
    QWidget * const subWidget = new QWidget;
    ui->setupUi(subWidget);
    mainLayout->addWidget(subWidget);
    mainLayout->addStretch(1);

    connect(m_remoteLinuxWidget,
        SIGNAL(currentModelChanged(const RemoteLinux::DeployableFilesPerProFile*)),
        SLOT(handleCurrentModelChanged(const RemoteLinux::DeployableFilesPerProFile*)));
    handleCurrentModelChanged(m_remoteLinuxWidget->currentModel());
}

MaemoDeployConfigurationWidget::~MaemoDeployConfigurationWidget()
{
    delete ui;
}

void MaemoDeployConfigurationWidget::init(DeployConfiguration *dc)
{
    m_remoteLinuxWidget->init(dc);
    connect(ui->addDesktopFileButton, SIGNAL(clicked()), SLOT(addDesktopFile()));
    connect(ui->addIconButton, SIGNAL(clicked()), SLOT(addIcon()));
    connect(deployConfiguration()->deploymentInfo(), SIGNAL(modelAboutToBeReset()),
        SLOT(handleDeploymentInfoToBeReset()));
}

Qt4MaemoDeployConfiguration *MaemoDeployConfigurationWidget::deployConfiguration() const
{
    return qobject_cast<Qt4MaemoDeployConfiguration *>(m_remoteLinuxWidget->deployConfiguration());
}

void MaemoDeployConfigurationWidget::handleDeploymentInfoToBeReset()
{
    ui->addDesktopFileButton->setEnabled(false);
    ui->addIconButton->setEnabled(false);
}

void MaemoDeployConfigurationWidget::handleCurrentModelChanged(const DeployableFilesPerProFile *proFileInfo)
{
    ui->addDesktopFileButton->setEnabled(canAddDesktopFile(proFileInfo));
    ui->addIconButton->setEnabled(canAddIcon(proFileInfo));
}

void MaemoDeployConfigurationWidget::addDesktopFile()
{
    DeployableFilesPerProFile * const proFileInfo = m_remoteLinuxWidget->currentModel();
    QTC_ASSERT(canAddDesktopFile(proFileInfo), return);

    const QString desktopFilePath = QFileInfo(proFileInfo->proFilePath()).path()
        + QLatin1Char('/') + proFileInfo->projectName() + QLatin1String(".desktop");
    if (!QFile::exists(desktopFilePath)) {
        const QString desktopTemplate = QLatin1String("[Desktop Entry]\nEncoding=UTF-8\n"
            "Version=1.0\nType=Application\nTerminal=false\nName=%1\nExec=%2\n"
            "Icon=%1\nX-Window-Icon=\nX-HildonDesk-ShowInToolbar=true\n"
            "X-Osso-Type=application/x-executable\n");
        Utils::FileSaver saver(desktopFilePath);
        saver.write(desktopTemplate.arg(proFileInfo->projectName(),
            proFileInfo->remoteExecutableFilePath()).toUtf8());
        if (!saver.finalize(this))
            return;
    }

    DeployableFile d;
    d.remoteDir = QLatin1String("/usr/share/applications");
    Core::Id deviceType
            = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(deployConfiguration()->target()->kit());
    if (deviceType == Maemo5OsType)
        d.remoteDir += QLatin1String("/hildon");
    d.localFilePath = desktopFilePath;
    if (!deployConfiguration()->deploymentSettingsAssistant()
            ->addDeployableToProFile(deployConfiguration()->qmakeScope(), proFileInfo,
                                     QLatin1String("desktopfile"), d)) {
        QMessageBox::critical(this, tr("Project File Update Failed"),
            tr("Could not update the project file."));
    } else {
        ui->addDesktopFileButton->setEnabled(false);
    }
}

void MaemoDeployConfigurationWidget::addIcon()
{
    DeployableFilesPerProFile * const proFileInfo = m_remoteLinuxWidget->currentModel();
    const int iconDim
        = MaemoGlobal::applicationIconSize(deployConfiguration()->target());
    const QString origFilePath = QFileDialog::getOpenFileName(this,
        tr("Choose Icon (will be scaled to %1x%1 pixels, if necessary)").arg(iconDim),
        proFileInfo->projectDir(), QLatin1String("(*.png)"));
    if (origFilePath.isEmpty())
        return;
    QPixmap pixmap(origFilePath);
    if (pixmap.isNull()) {
        QMessageBox::critical(this, tr("Invalid Icon"),
            tr("Unable to read image"));
        return;
    }
    const QSize iconSize(iconDim, iconDim);
    if (pixmap.size() != iconSize)
        pixmap = pixmap.scaled(iconSize);
    const QString newFileName = proFileInfo->projectName() + QLatin1Char('.')
        + QFileInfo(origFilePath).suffix();
    const QString newFilePath = proFileInfo->projectDir() + QLatin1Char('/') + newFileName;
    if (!pixmap.save(newFilePath)) {
        QMessageBox::critical(this, tr("Failed to Save Icon"),
            tr("Could not save icon to '%1'.").arg(newFilePath));
        return;
    }

    if (!deployConfiguration()->deploymentSettingsAssistant()
            ->addDeployableToProFile(deployConfiguration()->qmakeScope(), proFileInfo,
                                     QLatin1String("icon"), DeployableFile(newFilePath, remoteIconDir()))) {
        QMessageBox::critical(this, tr("Project File Update Failed"),
            tr("Could not update the project file."));
    } else {
        ui->addIconButton->setEnabled(false);
    }
}

bool MaemoDeployConfigurationWidget::canAddDesktopFile(const DeployableFilesPerProFile *proFileInfo) const
{
    return proFileInfo && proFileInfo->isApplicationProject()
        && deployConfiguration()->localDesktopFilePath(proFileInfo).isEmpty();
}

bool MaemoDeployConfigurationWidget::canAddIcon(const DeployableFilesPerProFile *proFileInfo) const
{
    return proFileInfo && proFileInfo->isApplicationProject()
        && remoteIconFilePath(proFileInfo).isEmpty();
}

QString MaemoDeployConfigurationWidget::remoteIconFilePath(const DeployableFilesPerProFile *proFileInfo) const
{
    QTC_ASSERT(proFileInfo->projectType() == ApplicationTemplate, return  QString());

    const QStringList imageTypes = QStringList() << QLatin1String("jpg") << QLatin1String("png")
        << QLatin1String("svg");
    for (int i = 0; i < proFileInfo->rowCount(); ++i) {
        const DeployableFile &d = proFileInfo->deployableAt(i);
        const QString extension = QFileInfo(d.localFilePath).suffix();
        if (d.remoteDir.startsWith(remoteIconDir()) && imageTypes.contains(extension))
            return d.remoteDir + QLatin1Char('/') + QFileInfo(d.localFilePath).fileName();
    }
    return QString();
}

QString MaemoDeployConfigurationWidget::remoteIconDir() const
{
    return QString::fromLatin1("/usr/share/icons/hicolor/%1x%1/apps")
        .arg(MaemoGlobal::applicationIconSize(deployConfiguration()->target()));
}

} // namespace Internal
} // namespace Madde
