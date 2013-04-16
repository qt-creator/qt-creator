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
#include "custombuildsteptool.h"

#include <QTabWidget>
#include <QLineEdit>
#include <QVBoxLayout>

#include "../../widgets/basicconfigurationwidget.h"
#include "../../widgets/lineedit.h"

namespace VcProjectManager {
namespace Internal {

CustomBuildStepTool::CustomBuildStepTool()
{
}

CustomBuildStepTool::CustomBuildStepTool(const CustomBuildStepTool &tool)
    : Tool(tool)
{
}

CustomBuildStepTool::~CustomBuildStepTool()
{
}

QString CustomBuildStepTool::nodeWidgetName() const
{
    return QObject::tr("Custom Build Step");
}

VcNodeWidget *CustomBuildStepTool::createSettingsWidget()
{
    return new CustomBuildStepToolWidget(QSharedPointer<CustomBuildStepTool>(this));
}

Tool::Ptr CustomBuildStepTool::clone() const
{
    return Tool::Ptr(new CustomBuildStepTool(*this));
}

QStringList CustomBuildStepTool::commandLine() const
{
    return readStringListAttribute(QLatin1String("CommandLine"), QLatin1String("\r\n"));
}

QStringList CustomBuildStepTool::commandLineDefault() const
{
    return QStringList();
}

void CustomBuildStepTool::setCommandLine(const QStringList &commandList)
{
    setStringListAttribute(QLatin1String("CommandLine"), commandList, QLatin1String("\r\n"));
}

QString CustomBuildStepTool::description() const
{
    return readStringAttribute(QLatin1String("Description"), descriptionDefault());
}

QString CustomBuildStepTool::descriptionDefault() const
{
    return QString(QLatin1String("Performing Custom Build Step"));
}

void CustomBuildStepTool::setDesription(const QString &desc)
{
    setStringAttribute(QLatin1String("Description"), desc, descriptionDefault());
}

QStringList CustomBuildStepTool::outputs() const
{
    return readStringListAttribute(QLatin1String("Outputs"), QLatin1String(";"));
}

QStringList CustomBuildStepTool::outputsDefault() const
{
    return QStringList();
}

void CustomBuildStepTool::setOutputs(const QStringList &outputList)
{
    setStringListAttribute(QLatin1String("Outputs"), outputList, QLatin1String(";"));
}

QStringList CustomBuildStepTool::additionalDependencies() const
{
    return readStringListAttribute(QLatin1String("AdditionalDependencies"), QLatin1String(";"));
}

QStringList CustomBuildStepTool::additionalDependenciesDefault() const
{
    return QStringList();
}

void CustomBuildStepTool::setAdditionalDependencies(const QStringList &dependencies)
{
    setStringListAttribute(QLatin1String("AdditionalDependencies"), dependencies, QLatin1String(";"));
}


CustomBuildStepToolWidget::CustomBuildStepToolWidget(CustomBuildStepTool::Ptr tool)
    : m_tool(tool)
{
    QTabWidget *mainTabWidget = new QTabWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(mainTabWidget);
    setLayout(layout);

    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_cmdLineEditLine = new LineEdit;
    m_cmdLineEditLine->setTextList(m_tool->commandLine(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Command Line"),
                                m_cmdLineEditLine,
                                tr("Specifies a command line for the custom build step."));

    m_descriptionLineEdit = new QLineEdit;
    m_descriptionLineEdit->setText(m_tool->description());
    basicWidget->insertTableRow(tr("Description"),
                                m_descriptionLineEdit,
                                tr("Provides a description for the custom build step."));

    m_outputsLineEdit = new LineEdit;
    m_outputsLineEdit->setTextList(m_tool->outputs(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Outputs"),
                                m_outputsLineEdit,
                                tr("Specifies the output files the custom build step generates."));

    m_additDependLineEdit = new LineEdit;
    m_additDependLineEdit->setTextList(m_tool->additionalDependencies(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Additional Dependencies"),
                                m_additDependLineEdit,
                                tr("Specifies the additional input files tom use for the  custom build step."));

    mainTabWidget->addTab(basicWidget, tr("General"));
}

CustomBuildStepToolWidget::~CustomBuildStepToolWidget()
{
}

void CustomBuildStepToolWidget::saveData()
{
    m_tool->setCommandLine(m_cmdLineEditLine->textToList(QLatin1String(";")));
    m_tool->setDesription(m_descriptionLineEdit->text());
    m_tool->setOutputs(m_outputsLineEdit->textToList(QLatin1String(";")));
    m_tool->setAdditionalDependencies(m_additDependLineEdit->textToList(QLatin1String(";")));
}

} // namespace Internal
} // namespace VcProjectManager
