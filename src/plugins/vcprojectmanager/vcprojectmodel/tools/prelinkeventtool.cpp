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
#include "prelinkeventtool.h"

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

#include "../../widgets/basicconfigurationwidget.h"
#include "../../widgets/lineedit.h"

namespace VcProjectManager {
namespace Internal {

PreLinkEventTool::PreLinkEventTool()
{
}

PreLinkEventTool::PreLinkEventTool(const PreLinkEventTool &tool)
    : Tool(tool)
{
}

PreLinkEventTool::~PreLinkEventTool()
{
}

QString PreLinkEventTool::nodeWidgetName() const
{
    return QObject::tr("Pre Link Event");
}

VcNodeWidget *PreLinkEventTool::createSettingsWidget()
{
    return new PreLinkEventToolWidget(QSharedPointer<PreLinkEventTool>(this));
}

Tool::Ptr PreLinkEventTool::clone() const
{
    return Tool::Ptr(new PreLinkEventTool(*this));
}

QStringList PreLinkEventTool::commandLine() const
{
    return readStringListAttribute(QLatin1String("CommandLine"), QLatin1String("\r\n"));
}

QStringList PreLinkEventTool::commandLineDefault() const
{
    return QStringList();
}

void PreLinkEventTool::setCommandLine(const QStringList &commandList)
{
    setStringListAttribute(QLatin1String("CommandLine"), commandList, QLatin1String("\r\n"));
}

QString PreLinkEventTool::description() const
{
    return readStringAttribute(QLatin1String("Description"), QString());
}

QString PreLinkEventTool::descriptionDefault() const
{
    return QString();
}

void PreLinkEventTool::setDesription(const QString &desc)
{
    setStringAttribute(QLatin1String("Description"), desc, descriptionDefault());
}

bool PreLinkEventTool::excludedFromBuild() const
{
    return readBooleanAttribute(QLatin1String("ExcludedFromBuild"), excludedFromBuildDefault());
}

bool PreLinkEventTool::excludedFromBuildDefault() const
{
    return false;
}

void PreLinkEventTool::setExcludedFromBuild(bool excludedFromBuild)
{
    setBooleanAttribute(QLatin1String("ExcludedFromBuild"), excludedFromBuild, excludedFromBuildDefault());
}


PreLinkEventToolWidget::PreLinkEventToolWidget(PreLinkEventTool::Ptr tool)
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
                                tr("Specifies a command line for the pre-link event tool to run."));

    // Description
    m_descriptionLineEdit = new QLineEdit;
    m_descriptionLineEdit->setText(m_tool->description());
    basicWidget->insertTableRow(tr("Description"),
                                m_descriptionLineEdit,
                                tr("Provides a description for the pre-link event tool to display."));

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

PreLinkEventToolWidget::~PreLinkEventToolWidget()
{
}

void PreLinkEventToolWidget::saveData()
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
