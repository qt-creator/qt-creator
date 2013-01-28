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

#ifndef MAEMODEPLOYCONFIGURATIONWIDGET_H
#define MAEMODEPLOYCONFIGURATIONWIDGET_H

#include <projectexplorer/deployconfiguration.h>

namespace RemoteLinux {
class DeployableFilesPerProFile;
class RemoteLinuxDeployConfigurationWidget;
}

namespace Madde {
namespace Internal {

class Qt4MaemoDeployConfiguration;
namespace Ui { class MaemoDeployConfigurationWidget; }


class MaemoDeployConfigurationWidget : public ProjectExplorer::DeployConfigurationWidget
{
    Q_OBJECT

public:
    explicit MaemoDeployConfigurationWidget(QWidget *parent = 0);
    ~MaemoDeployConfigurationWidget();

    void init(ProjectExplorer::DeployConfiguration *dc);

    Qt4MaemoDeployConfiguration *deployConfiguration() const;

private slots:
    void addDesktopFile();
    void addIcon();
    void handleDeploymentInfoToBeReset();
    void handleCurrentModelChanged(const RemoteLinux::DeployableFilesPerProFile *proFileInfo);

private:
    bool canAddDesktopFile(const RemoteLinux::DeployableFilesPerProFile *proFileInfo) const;
    bool canAddIcon(const RemoteLinux::DeployableFilesPerProFile *proFileInfo) const;
    QString remoteIconFilePath(const RemoteLinux::DeployableFilesPerProFile *proFileInfo) const;
    QString remoteIconDir() const;

    Ui::MaemoDeployConfigurationWidget *ui;
    RemoteLinux::RemoteLinuxDeployConfigurationWidget * const m_remoteLinuxWidget;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMODEPLOYCONFIGURATIONWIDGET_H
