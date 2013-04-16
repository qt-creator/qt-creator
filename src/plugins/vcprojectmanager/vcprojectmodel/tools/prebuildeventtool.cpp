/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#include "prebuildeventtool.h"

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

#include "../../widgets/basicconfigurationwidget.h"
#include "../../widgets/lineedit.h"

namespace VcProjectManager {
namespace Internal {

PreBuildEventTool::PreBuildEventTool()
{
}

PreBuildEventTool::PreBuildEventTool(const PreBuildEventTool &tool)
    : Tool(tool)
{
}

PreBuildEventTool::~PreBuildEventTool()
{
}

QString PreBuildEventTool::nodeWidgetName() const
{
    return QObject::tr("Pre Build Event");
}

VcNodeWidget *PreBuildEventTool::createSettingsWidget()
{
    return new PreBuildEventToolWidget(QSharedPointer<PreBuildEventTool>(this));
}

Tool::Ptr PreBuildEventTool::clone() const
{
    return Tool::Ptr(new PreBuildEventTool(*this));
}

QStringList PreBuildEventTool::commandLine() const
{
    return readStringListAttribute(QLatin1String("CommandLine"), QLatin1String("\r\n"));
}

QStringList PreBuildEventTool::commandLineDefault() const
{
    return QStringList();
}

void PreBuildEventTool::setCommandLine(const QStringList &commandList)
{
    setStringListAttribute(QLatin1String("CommandLine"), commandList, QLatin1String("\r\n"));
}

QString PreBuildEventTool::description() const
{
    return readStringAttribute(QLatin1String("Description"), descriptionDefault());
}

QString PreBuildEventTool::descriptionDefault() const
{
    return QString();
}

void PreBuildEventTool::setDesription(const QString &desc)
{
    setStringAttribute(QLatin1String("Description"), desc, descriptionDefault());
}

bool PreBuildEventTool::excludedFromBuild() const
{
    return readBooleanAttribute(QLatin1String("ExcludedFromBuild"), excludedFromBuildDefault());
}

bool PreBuildEventTool::excludedFromBuildDefault() const
{
    return false;
}

void PreBuildEventTool::setExcludedFromBuild(bool excludedFromBuild)
{
    setBooleanAttribute(QLatin1String("ExcludedFromBuild"), excludedFromBuild, excludedFromBuildDefault());
}


PreBuildEventToolWidget::PreBuildEventToolWidget(PreBuildEventTool::Ptr tool)
    : m_tool(tool)
{
    QTabWidget *mainTabWidget = new QTabWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(mainTabWidget);
    setLayout(layout);

    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget(this);
    mainTabWidget->addTab(basicWidget, tr("General"));

    // Command Line
    m_cmdLineLineEdit = new LineEdit;
    m_cmdLineLineEdit->setTextList(m_tool->commandLine(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Command Line"),
                                m_cmdLineLineEdit,
                                tr("Specifies a command line for the pre-build event tool to run."));

    // Description
    m_descriptionLineEdit = new QLineEdit;
    m_descriptionLineEdit->setText(m_tool->description());
    basicWidget->insertTableRow(tr("Description"),
                                m_descriptionLineEdit,
                                tr("Provides a description for the pre-build event tool to display."));

    // Excluded From Build
    m_excludedFromBuildComboBox = new QComboBox;
    m_excludedFromBuildComboBox->addItem(tr("No"));
    m_excludedFromBuildComboBox->addItem(tr("Yes"));

    if (m_tool->excludedFromBuild())
        m_excludedFromBuildComboBox->setCurrentIndex(1);
    else
        m_excludedFromBuildComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Excluded From Build"),
                                m_excludedFromBuildComboBox,
                                tr("Specifies whether this build event is excluded from the build for the current configuration."));
}

PreBuildEventToolWidget::~PreBuildEventToolWidget()
{
}

void PreBuildEventToolWidget::saveData()
{
    m_tool->setCommandLine(m_cmdLineLineEdit->textToList(QLatin1String(";")));
    m_tool->setDesription(m_descriptionLineEdit->text());

    if (m_excludedFromBuildComboBox->currentIndex() >= 0) {
        QVariant data = m_excludedFromBuildComboBox->itemData(m_excludedFromBuildComboBox->currentIndex(),
                                                      Qt::UserRole);
        m_tool->setExcludedFromBuild(data.toBool());
    }
}

} // namespace Internal
} // namespace VcProjectManager
