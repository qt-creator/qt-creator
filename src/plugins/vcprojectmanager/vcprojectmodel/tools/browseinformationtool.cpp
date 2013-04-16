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
#include "browseinformationtool.h"

#include "../../widgets/basicconfigurationwidget.h"
#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>

namespace VcProjectManager {
namespace Internal {


BrowseInformationTool::BrowseInformationTool()
{
}

BrowseInformationTool::BrowseInformationTool(const BrowseInformationTool &tool)
    : Tool(tool)
{
}

BrowseInformationTool::~BrowseInformationTool()
{
}

QString BrowseInformationTool::nodeWidgetName() const
{
    return QObject::tr("Browse Information");
}

VcNodeWidget *BrowseInformationTool::createSettingsWidget()
{
    return new BrowseInformationToolWidget(QSharedPointer<BrowseInformationTool>(this));
}

Tool::Ptr BrowseInformationTool::clone() const
{
    return Tool::Ptr(new BrowseInformationTool(*this));
}

bool BrowseInformationTool::suppressStartupBanner() const
{
    return readBooleanAttribute(QLatin1String("SuppressStartupBanner"), suppressStartupBannerDefault());
}

bool BrowseInformationTool::suppressStartupBannerDefault() const
{
    return true;
}

void BrowseInformationTool::setSuppressStartupBanner(bool suppress)
{
    setBooleanAttribute(QLatin1String("SuppressStartupBanner"), suppress, suppressStartupBannerDefault());
}

QString BrowseInformationTool::outputFile() const
{
    return readStringAttribute(QLatin1String("OutputFile"), outputFileDefault());
}

QString BrowseInformationTool::outputFileDefault() const
{
    return QLatin1String("$(OutDir)/$(ProjectName).bsc");
}

void BrowseInformationTool::setOutputFile(const QString &outFile)
{
    setStringAttribute(QLatin1String("OutputFile"), outFile, outputFileDefault());
}


BrowseInformationToolWidget::BrowseInformationToolWidget(BrowseInformationTool::Ptr browseTool)
    : m_tool(browseTool)
{
    QTabWidget *mainTabWidget = new QTabWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(mainTabWidget);
    setLayout(layout);

    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_suppresStartUpBannerComboBox = new QComboBox;
    m_suppresStartUpBannerComboBox->addItem(tr("No"), QVariant(false));
    m_suppresStartUpBannerComboBox->addItem(tr("Yes (/nologo)"), QVariant(true));

    if (m_tool->suppressStartupBanner())
        m_suppresStartUpBannerComboBox->setCurrentIndex(1);
    else
        m_suppresStartUpBannerComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Suppress Startup Banner"),
                                m_suppresStartUpBannerComboBox,
                                tr("Suppress the display of the startup banner and information messages."));

    m_outputFileLineEdit = new QLineEdit;
    m_outputFileLineEdit->setText(m_tool->outputFile());
    basicWidget->insertTableRow(tr("Output File"),
                                m_outputFileLineEdit,
                                tr("Overrides the default output file name."));

    mainTabWidget->addTab(basicWidget, tr("General"));
}

BrowseInformationToolWidget::~BrowseInformationToolWidget()
{
}

void BrowseInformationToolWidget::saveData()
{
    if (m_suppresStartUpBannerComboBox->currentIndex() >= 0) {
        QVariant data = m_suppresStartUpBannerComboBox->itemData(m_suppresStartUpBannerComboBox->currentIndex(),
                                                                 Qt::UserRole);

        m_tool->setSuppressStartupBanner(data.toBool());
    }

    m_tool->setOutputFile(m_outputFileLineEdit->text());
}

} // namespace Internal
} // namespace VcProjectManager
