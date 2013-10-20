/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#include "configurationbasewidget.h"

#include <QSplitter>
#include <QListWidget>
#include <QStackedWidget>
#include <QHBoxLayout>

#include "../vcprojectmodel/tools/tool_constants.h"
#include "../vcprojectmodel/tools/configurationtool.h"
#include "../vcprojectmodel/tools/toolattributes/tooldescription.h"
#include "../interfaces/iconfigurationbuildtools.h"
#include "../interfaces/itools.h"

namespace VcProjectManager {
namespace Internal {

ConfigurationBaseWidget::ConfigurationBaseWidget(IConfiguration *config)
    : m_config(config)
{
    QSplitter *mainWidgetSplitter = new QSplitter(Qt::Horizontal, this);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(mainWidgetSplitter);
    setLayout(layout);

    m_listWidget = new QListWidget;
    m_listWidget->setMinimumWidth(200);
    m_listWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_listWidget->setMaximumWidth(300);
    m_stackWidget = new QStackedWidget;

    mainWidgetSplitter->addWidget(m_listWidget);
    mainWidgetSplitter->addWidget(m_stackWidget);
    mainWidgetSplitter->setCollapsible(0, false);
    mainWidgetSplitter->setCollapsible(1, false);
    mainWidgetSplitter->setStretchFactor(0, 1);
    mainWidgetSplitter->setStretchFactor(1, 5);
    // add tool items

    for (int i = 0; i < m_config->tools()->configurationBuildTools()->toolCount(); ++i) {
        IConfigurationBuildTool *configTool = m_config->tools()->configurationBuildTools()->tool(i);

        if (configTool) {
            VcNodeWidget *toolWidget = configTool->createSettingsWidget();

            if (toolWidget) {
                m_listWidget->addItem(configTool->toolDescription()->toolDisplayName());
                m_stackWidget->addWidget(toolWidget);
                m_toolWidgets.append(toolWidget);
            }
        }
    }

    connect(m_listWidget, SIGNAL(currentRowChanged(int)), m_stackWidget, SLOT(setCurrentIndex(int)));
}

ConfigurationBaseWidget::~ConfigurationBaseWidget()
{
}

void ConfigurationBaseWidget::saveData()
{
    foreach (VcNodeWidget *toolWidget, m_toolWidgets)
        toolWidget->saveData();
}

} // namespace Internal
} // namespace VcProjectManager
