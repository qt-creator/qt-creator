/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/
#ifndef REMOTELINUXDEPLOYCONFIGURATIONWIDGET_H
#define REMOTELINUXDEPLOYCONFIGURATIONWIDGET_H

#include "remotelinux_export.h"

#include <projectexplorer/deployconfiguration.h>

namespace RemoteLinux {
class DeployableFilesPerProFile;
class RemoteLinuxDeployConfiguration;

namespace Internal {
class RemoteLinuxDeployConfigurationWidgetPrivate;
} // namespace Internal

class REMOTELINUX_EXPORT RemoteLinuxDeployConfigurationWidget
    : public ProjectExplorer::DeployConfigurationWidget
{
    Q_OBJECT

public:
    explicit RemoteLinuxDeployConfigurationWidget(QWidget *parent = 0);
    ~RemoteLinuxDeployConfigurationWidget();

    void init(ProjectExplorer::DeployConfiguration *dc);

    RemoteLinuxDeployConfiguration *deployConfiguration() const;
    DeployableFilesPerProFile *currentModel() const;

signals:
    void currentModelChanged(const RemoteLinux::DeployableFilesPerProFile *proFileInfo);

private slots:
    void handleModelListToBeReset();
    void handleModelListReset();
    void setModel(int row);
    void openProjectFile();

private:
    Internal::RemoteLinuxDeployConfigurationWidgetPrivate * const d;
};

} // namespace RemoteLinux

#endif // REMOTELINUXDEPLOYCONFIGURATIONWIDGET_H
