/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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

namespace Qt4ProjectManager {
namespace Internal {
class Qt4MaemoDeployConfiguration;

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
    Q_SLOT void addDesktopFile();
    Q_SLOT void addIcon();

    Ui::MaemoDeployConfigurationWidget *ui;
    Qt4MaemoDeployConfiguration * m_deployConfig;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEPLOYCONFIGURATIONWIDGET_H
