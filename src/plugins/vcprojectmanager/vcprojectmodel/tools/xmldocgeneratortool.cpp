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
#include "xmldocgeneratortool.h"

#include <QLineEdit>
#include <QComboBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include "../../widgets/basicconfigurationwidget.h"
#include "../../widgets/lineedit.h"

namespace VcProjectManager {
namespace Internal {

XMLDocGeneratorTool::XMLDocGeneratorTool()
{
    m_name = QLatin1String("VCXDCMakeTool");
}

XMLDocGeneratorTool::XMLDocGeneratorTool(const XMLDocGeneratorTool &tool)
    : Tool(tool)
{
}

XMLDocGeneratorTool::~XMLDocGeneratorTool()
{
}

QString XMLDocGeneratorTool::nodeWidgetName() const
{
    return QObject::tr("XML Document Generator");
}

VcNodeWidget *XMLDocGeneratorTool::createSettingsWidget()
{
    return new XMLDocGeneratorToolWidget(this);
}

Tool::Ptr XMLDocGeneratorTool::clone() const
{
    return Tool::Ptr(new XMLDocGeneratorTool(*this));
}

bool XMLDocGeneratorTool::suppressStartupBanner() const
{
    return readBooleanAttribute(QLatin1String("SuppressStartupBanner"), suppressStartupBannerDefault());
}

bool XMLDocGeneratorTool::suppressStartupBannerDefault() const
{
    return true;
}

void XMLDocGeneratorTool::setSuppressStartupBanner(bool suppress)
{
    setBooleanAttribute(QLatin1String("SuppressStartupBanner"), suppress, suppressStartupBannerDefault());
}

bool XMLDocGeneratorTool::validateIntelliSense() const
{
    return readBooleanAttribute(QLatin1String("ValidateIntelliSense"), validateIntelliSenseDefault());
}

bool XMLDocGeneratorTool::validateIntelliSenseDefault() const
{
    return false;
}

void XMLDocGeneratorTool::setValidateIntelliSense(bool validate)
{
    setBooleanAttribute(QLatin1String("ValidateIntelliSense"), validate, validateIntelliSenseDefault());
}

QStringList XMLDocGeneratorTool::additioalDocumentFiles() const
{
    return readStringListAttribute(QLatin1String("AdditionalDocumentFiles"), QLatin1String(";"));
}

QStringList XMLDocGeneratorTool::additioalDocumentFilesDefault() const
{
    return QStringList();
}

void XMLDocGeneratorTool::setAdditioalDocumentFiles(const QStringList &files)
{
    setStringListAttribute(QLatin1String("AdditionalDocumentFiles"), files, QLatin1String(";"));
}

QString XMLDocGeneratorTool::outputDocumentFile() const
{
    return readStringAttribute(QLatin1String("OutputDocumentFile"), outputDocumentFileDefault());
}

QString XMLDocGeneratorTool::outputDocumentFileDefault() const
{
    return QLatin1String("$(TargetDir)$(TargetName).xml");
}

void XMLDocGeneratorTool::setOutputDocumentFile(const QString &outDocFile)
{
    setStringAttribute(QLatin1String("OutputDocumentFile"), outDocFile, outputDocumentFileDefault());
}

bool XMLDocGeneratorTool::documentLibraryDependencies() const
{
    return readBooleanAttribute(QLatin1String("DocumentLibraryDependencies"), documentLibraryDependenciesDefault());
}

bool XMLDocGeneratorTool::documentLibraryDependenciesDefault() const
{
    return true;
}

void XMLDocGeneratorTool::setDocumentLibraryDependencies(bool docLibDepend)
{
    setBooleanAttribute(QLatin1String("DocumentLibraryDependencies"), docLibDepend, documentLibraryDependenciesDefault());
}

bool XMLDocGeneratorTool::useUnicodeResponseFiles() const
{
    return readBooleanAttribute(QLatin1String("UseUnicodeResponseFiles"), useUnicodeResponseFilesDefault());
}

bool XMLDocGeneratorTool::useUnicodeResponseFilesDefault() const
{
    return true;
}

void XMLDocGeneratorTool::setUseUnicodeResponseFiles(bool useUnicodeRespFiles)
{
    setBooleanAttribute(QLatin1String("UseUnicodeResponseFiles"), useUnicodeRespFiles, useUnicodeResponseFilesDefault());
}


XMLDocGeneratorToolWidget::XMLDocGeneratorToolWidget(XMLDocGeneratorTool *tool)
    : m_tool(tool)
{
    QTabWidget *mainTabWidget = new QTabWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(mainTabWidget);
    setLayout(layout);

    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_suppressStartBannerComboBox = new QComboBox;
    m_suppressStartBannerComboBox->addItem(tr("No"), QVariant(false));
    m_suppressStartBannerComboBox->addItem(tr("Yes (/nologo)"), QVariant(true));

    if (m_tool->suppressStartupBanner())
        m_suppressStartBannerComboBox->setCurrentIndex(1);
    else
        m_suppressStartBannerComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Suppress Startup Banner"),
                                m_suppressStartBannerComboBox,
                                tr("Suppresses the display of the startup banner and information messages."));

    m_validIntellisenseComboBox = new QComboBox;
    m_validIntellisenseComboBox->addItem(tr("No"), QVariant(false));
    m_validIntellisenseComboBox->addItem(tr("Yes (/validate)"), QVariant(true));

    if (m_tool->validateIntelliSense())
        m_validIntellisenseComboBox->setCurrentIndex(1);
    else
        m_validIntellisenseComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Validate IntelliSense"),
                                m_validIntellisenseComboBox,
                                tr("Validates the IntelliSense information after the documentation merger completes."));

    m_additDocFilesLineEdit = new LineEdit;
    m_additDocFilesLineEdit->setTextList(m_tool->additioalDocumentFiles(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Additional Document Files"),
                                m_additDocFilesLineEdit,
                                tr("Additional documentation files (.XDC) to merge."));

    m_outputDocFileLineEdit = new QLineEdit;
    m_outputDocFileLineEdit->setText(m_tool->outputDocumentFile());
    basicWidget->insertTableRow(tr("Output Document File"),
                                m_outputDocFileLineEdit,
                                tr("Specifies the output document file (.XML) that is merged from all the input document files."));

    m_docLibDepComboBox = new QComboBox;
    m_docLibDepComboBox->addItem(tr("No"), QVariant(false));
    m_docLibDepComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->documentLibraryDependencies())
        m_docLibDepComboBox->setCurrentIndex(1);
    else
        m_docLibDepComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Document Library Dependencies"),
                                m_docLibDepComboBox,
                                tr("If true, project dependencies that output static libraries will have their XML document comment files (.XDC) documented in this project's XML document file."));

    m_useUnicodeRespFilesComboBox = new QComboBox;
    m_useUnicodeRespFilesComboBox->addItem(tr("No"), QVariant(false));
    m_useUnicodeRespFilesComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->useUnicodeResponseFiles())
        m_useUnicodeRespFilesComboBox->setCurrentIndex(1);
    else
        m_useUnicodeRespFilesComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Use Unicode Response Files"),
                                m_useUnicodeRespFilesComboBox,
                                tr("Instructs the project system to generate UNICODE response files when spawning the xdcmake tool. Set this property to “Yes” when files in the project have UNICODE paths."));

    mainTabWidget->addTab(basicWidget, tr("General"));
}

XMLDocGeneratorToolWidget::~XMLDocGeneratorToolWidget()
{
}

void XMLDocGeneratorToolWidget::saveData()
{
    if (m_suppressStartBannerComboBox->currentIndex() >= 0) {
        QVariant data = m_suppressStartBannerComboBox->itemData(m_suppressStartBannerComboBox->currentIndex(),
                                                                Qt::UserRole);
        m_tool->setSuppressStartupBanner(data.toBool());
    }

    if (m_validIntellisenseComboBox->currentIndex() >= 0) {
        QVariant data = m_validIntellisenseComboBox->itemData(m_validIntellisenseComboBox->currentIndex(),
                                                              Qt::UserRole);
        m_tool->setValidateIntelliSense(data.toBool());
    }

    m_tool->setAdditioalDocumentFiles(m_additDocFilesLineEdit->textToList(QLatin1String(";")));
    m_tool->setOutputDocumentFile(m_outputDocFileLineEdit->text());

    if (m_docLibDepComboBox->currentIndex() >= 0) {
        QVariant data = m_docLibDepComboBox->itemData(m_docLibDepComboBox->currentIndex(),
                                                      Qt::UserRole);
        m_tool->setDocumentLibraryDependencies(data.toBool());
    }

    if (m_useUnicodeRespFilesComboBox->currentIndex() >= 0) {
        QVariant data = m_useUnicodeRespFilesComboBox->itemData(m_useUnicodeRespFilesComboBox->currentIndex(),
                                                                Qt::UserRole);
        m_tool->setUseUnicodeResponseFiles(data.toBool());
    }
}

} // namespace Internal
} // namespace VcProjectManager
