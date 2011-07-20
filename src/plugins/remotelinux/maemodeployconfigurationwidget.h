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

#ifndef MAEMODEPLOYCONFIGURATIONWIDGET_H
#define MAEMODEPLOYCONFIGURATIONWIDGET_H

#include <projectexplorer/deployconfiguration.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MaemoDeployConfigurationWidget;
}
QT_END_NAMESPACE

namespace RemoteLinux {
class Qt4MaemoDeployConfiguration;
namespace Internal {

class MaemoDeployConfigurationWidget : public ProjectExplorer::DeployConfigurationWidget
{
    Q_OBJECT

public:
    explicit MaemoDeployConfigurationWidget(QWidget *parent = 0);
    ~MaemoDeployConfigurationWidget();

    void init(ProjectExplorer::DeployConfiguration *dc);

private:
    Q_SLOT void handleModelListToBeReset();
    Q_SLOT void handleModelListReset();
    Q_SLOT void setModel(int row);
    Q_SLOT void handleSelectedDeviceConfigurationChanged(int index);
    Q_SLOT void handleDeviceConfigurationListChanged();
    Q_SLOT void addDesktopFile();
    Q_SLOT void addIcon();
    Q_SLOT void showDeviceConfigurations();

    Ui::MaemoDeployConfigurationWidget *ui;
    Qt4MaemoDeployConfiguration * m_deployConfig;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMODEPLOYCONFIGURATIONWIDGET_H
