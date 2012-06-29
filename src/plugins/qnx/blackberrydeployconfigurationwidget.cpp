/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberrydeployconfigurationwidget.h"
#include "ui_blackberrydeployconfigurationwidget.h"
#include "blackberrydeployconfiguration.h"
#include "blackberrydeployinformation.h"
#include "pathchooserdelegate.h"

#include <coreplugin/icore.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/pathchooser.h>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryDeployConfigurationWidget::BlackBerryDeployConfigurationWidget(QWidget *parent)
    : ProjectExplorer::DeployConfigurationWidget(parent)
    , m_ui(new Ui::BlackBerryDeployConfigurationWidget)
    , m_deployConfiguration(0)
{
    m_ui->setupUi(this);
}

BlackBerryDeployConfigurationWidget::~BlackBerryDeployConfigurationWidget()
{
    delete m_ui;
}

void BlackBerryDeployConfigurationWidget::init(ProjectExplorer::DeployConfiguration *dc)
{
    m_deployConfiguration = qobject_cast<BlackBerryDeployConfiguration *>(dc);

    m_ui->deployPackagesView->setModel(m_deployConfiguration->deploymentInfo());

    PathChooserDelegate *appDescriptorPathDelegate = new PathChooserDelegate(this);
    appDescriptorPathDelegate->setExpectedKind(Utils::PathChooser::File);
    appDescriptorPathDelegate->setPromptDialogFilter(QLatin1String("*.xml"));

    PathChooserDelegate *barPathDelegate = new PathChooserDelegate(this);
    barPathDelegate->setExpectedKind(Utils::PathChooser::File);
    barPathDelegate->setPromptDialogFilter(QLatin1String("*.bar"));

    m_ui->deployPackagesView->setItemDelegateForColumn(1, appDescriptorPathDelegate);
    m_ui->deployPackagesView->setItemDelegateForColumn(2, barPathDelegate);

    m_ui->deployPackagesView->header()->resizeSections(QHeaderView::ResizeToContents);
}
