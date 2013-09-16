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
#include "toolsettingswidget.h"

#include "../../vcprojectmodel/tools/configurationtool.h"
#include "../../vcprojectmodel/tools/toolsection.h"
#include "../../vcprojectmodel/tools/toolsectiondescription.h"
#include "toolsectionsettingswidget.h"

#include <QTableWidget>
#include <QVBoxLayout>

namespace VcProjectManager {
namespace Internal {

ToolSettingsWidget::ToolSettingsWidget(ConfigurationTool *tool, QWidget *parent)
    : VcNodeWidget(parent),
      m_tool(tool)
{
    QTabWidget *mainTabWidget = new QTabWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(mainTabWidget);
    setLayout(layout);

    if (m_tool) {
        for (int i = 0; i < m_tool->sectionCount(); ++i) {
            ToolSection *toolSection = m_tool->section(i);

            if (toolSection) {
                ToolSectionSettingsWidget *toolSectionWidget = toolSection->createSettingsWidget();
                mainTabWidget->addTab(toolSection->createSettingsWidget(), toolSection->sectionDescription()->name());
                m_sections.append(toolSectionWidget);
            }
        }
    }
}

void ToolSettingsWidget::saveData()
{
    foreach (ToolSectionSettingsWidget *toolSectionWidget, m_sections)
        toolSectionWidget->saveData();
}

} // namespace Internal
} // namespace VcProjectManager
