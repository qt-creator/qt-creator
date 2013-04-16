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
#include "manifesttool.h"

#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>

#include "../../widgets/basicconfigurationwidget.h"

namespace VcProjectManager {
namespace Internal {

ManifestTool::ManifestTool()
{
}

ManifestTool::ManifestTool(const ManifestTool &tool)
    : Tool(tool)
{
}

ManifestTool::~ManifestTool()
{
}

QString ManifestTool::nodeWidgetName() const
{
    return QObject::tr("Manifest");
}

VcNodeWidget *ManifestTool::createSettingsWidget()
{
    return new ManifestToolWidget(QSharedPointer<ManifestTool>(this));
}

Tool::Ptr ManifestTool::clone() const
{
    return Tool::Ptr(new ManifestTool(*this));
}

bool ManifestTool::suppressStartupBanner() const
{
    return readBooleanAttribute(QLatin1String("SuppressStartupBanner"), suppressStartupBannerDefault());
}

bool ManifestTool::suppressStartupBannerDefault() const
{
    return true;
}

void ManifestTool::setSuppressStartupBanner(bool suppress)
{
    setBooleanAttribute(QLatin1String("SuppressStartupBanner"), suppress, suppressStartupBannerDefault());
}

bool ManifestTool::verboseOutput() const
{
    return readBooleanAttribute(QLatin1String("VerboseOutput"), verboseOutputDefault());
}

bool ManifestTool::verboseOutputDefault() const
{
    return false;
}

void ManifestTool::setVerboseOutput(bool enable)
{
    setBooleanAttribute(QLatin1String("VerboseOutput"), enable, verboseOutputDefault());
}

QString ManifestTool::assemblyIdentity() const
{
    return readStringAttribute(QLatin1String("AssemblyIdentity"), assemblyIdentityDefault());
}

QString ManifestTool::assemblyIdentityDefault() const
{
    return QString();
}

void ManifestTool::setAssemblyIdentity(const QString &identity)
{
    setStringAttribute(QLatin1String("AssemblyIdentity"), identity, assemblyIdentityDefault());
}

bool ManifestTool::useUnicodeResponseFiles() const
{
    return readBooleanAttribute(QLatin1String("UseUnicodeResponseFiles"), useUnicodeResponseFilesDefault());
}

bool ManifestTool::useUnicodeResponseFilesDefault() const
{
    return true;
}

void ManifestTool::setUseUnicodeResponseFiles(bool enable)
{
    setBooleanAttribute(QLatin1String("UseUnicodeResponseFiles"), enable, useUnicodeResponseFilesDefault());
}

bool ManifestTool::useFAT32Workaround() const
{
    return readBooleanAttribute(QLatin1String("UseFAT32Workaround"), useFAT32WorkaroundDefault());
}

bool ManifestTool::useFAT32WorkaroundDefault() const
{
    return false;
}

void ManifestTool::setUseFAT32Workaround(bool enable)
{
    setBooleanAttribute(QLatin1String("UseFAT32Workaround"), enable, useFAT32WorkaroundDefault());
}

QString ManifestTool::additionalManifestFiles() const
{
    return readStringAttribute(QLatin1String("AdditionalManifestFiles"), additionalManifestFilesDefault());
}

QString ManifestTool::additionalManifestFilesDefault() const
{
    return QString();
}

void ManifestTool::setAdditionalManifestFiles(const QString &files)
{
    setStringAttribute(QLatin1String("AdditionalManifestFiles"), files, additionalManifestFilesDefault());
}

QString ManifestTool::inputResourceManifests() const
{
    return readStringAttribute(QLatin1String("InputResourceManifests"), inputResourceManifestsDefault());
}

QString ManifestTool::inputResourceManifestsDefault() const
{
    return QString();
}

void ManifestTool::setInputResourceManifests(const QString &manifests)
{
    setStringAttribute(QLatin1String("InputResourceManifests"), manifests, inputResourceManifestsDefault());
}

bool ManifestTool::embedManifest() const
{
    return readBooleanAttribute(QLatin1String("EmbedManifest"), embedManifestDefault());
}

bool ManifestTool::embedManifestDefault() const
{
    return true;
}

void ManifestTool::setEmbedManifest(bool enable)
{
    setBooleanAttribute(QLatin1String("EmbedManifest"), enable, embedManifestDefault());
}

QString ManifestTool::outputManifestFile() const
{
    return readStringAttribute(QLatin1String("OutputManifestFile"), outputManifestFileDefault());
}

QString ManifestTool::outputManifestFileDefault() const
{
    return QLatin1String("$(TargetPath).manifest");
}

void ManifestTool::setOutputManifestFile(const QString &file)
{
    setStringAttribute(QLatin1String("OutputManifestFile"), file, outputManifestFileDefault());
}

QString ManifestTool::manifestResourceFile() const
{
    return readStringAttribute(QLatin1String("ManifestResourceFile"), manifestResourceFileDefault());
}

QString ManifestTool::manifestResourceFileDefault() const
{
    return QLatin1String("$(IntDir)\\$(TargetFileName).embed.manifest.res");
}

void ManifestTool::setManifestResourceFile(const QString &file)
{
    setStringAttribute(QLatin1String("ManifestResourceFile"), file, manifestResourceFileDefault());
}

bool ManifestTool::generateCatalogFiles() const
{
    return readBooleanAttribute(QLatin1String("GenerateCatalogFiles"), generateCatalogFilesDefault());
}

bool ManifestTool::generateCatalogFilesDefault() const
{
    return false;
}

void ManifestTool::setGenerateCatalogFiles(bool generate)
{
    setBooleanAttribute(QLatin1String("GenerateCatalogFiles"), generate, generateCatalogFilesDefault());
}

QString ManifestTool::dependencyInformationFile() const
{
    return readStringAttribute(QLatin1String("DependencyInformationFile"), dependencyInformationFileDefault());
}

QString ManifestTool::dependencyInformationFileDefault() const
{
    return QLatin1String("$(IntDir)\\mt.dep");
}

void ManifestTool::setDependencyInformationFile(const QString &file)
{
    setStringAttribute(QLatin1String("DependencyInformationFile"), file, dependencyInformationFileDefault());
}

QString ManifestTool::typeLibraryFile() const
{
    return readStringAttribute(QLatin1String("TypeLibraryFile"), typeLibraryFileDefault());
}

QString ManifestTool::typeLibraryFileDefault() const
{
    return QString();
}

void ManifestTool::setTypeLibraryFile(const QString &file)
{
    setStringAttribute(QLatin1String("TypeLibraryFile"), file, typeLibraryFileDefault());
}

QString ManifestTool::registrarScriptFile() const
{
    return readStringAttribute(QLatin1String("RegistrarScriptFile"), registrarScriptFileDefault());
}

QString ManifestTool::registrarScriptFileDefault() const
{
    return QString();
}

void ManifestTool::setRegistrarScriptFile(const QString &file)
{
    setStringAttribute(QLatin1String("RegistrarScriptFile"), file, registrarScriptFileDefault());
}

QString ManifestTool::componentFileName() const
{
    return readStringAttribute(QLatin1String("ComponentFileName"), componentFileNameDefault());
}

QString ManifestTool::componentFileNameDefault() const
{
    return QString();
}

void ManifestTool::setComponentFileName(const QString &fileName)
{
    setStringAttribute(QLatin1String("ComponentFileName"), fileName, componentFileNameDefault());
}

QString ManifestTool::replacementsFile() const
{
    return readStringAttribute(QLatin1String("ReplacementsFile"), replacementsFileDefault());
}

QString ManifestTool::replacementsFileDefault() const
{
    return QString();
}

void ManifestTool::setReplacementsFile(const QString &file)
{
    setStringAttribute(QLatin1String("ReplacementsFile"), file, replacementsFileDefault());
}

bool ManifestTool::updateFileHashes() const
{
    return readBooleanAttribute(QLatin1String("UpdateFileHashes"), updateFileHashesDefault());
}

bool ManifestTool::updateFileHashesDefault() const
{
    return false;
}

void ManifestTool::setUpdateFileHashes(bool update)
{
    setBooleanAttribute(QLatin1String("UpdateFileHashes"), update, updateFileHashesDefault());
}

QString ManifestTool::updateFileHashesSearchPath() const
{
    return readStringAttribute(QLatin1String("UpdateFileHashesSearchPath"), updateFileHashesSearchPathDefault());
}

QString ManifestTool::updateFileHashesSearchPathDefault() const
{
    return QString();
}

void ManifestTool::setUpdateFileHashesSearchPath(const QString &searchPath)
{
    setStringAttribute(QLatin1String("UpdateFileHashesSearchPath"), searchPath, updateFileHashesSearchPathDefault());
}


ManifestToolWidget::ManifestToolWidget(ManifestTool::Ptr tool)
    : m_tool(tool)
{
    QTabWidget *mainTabWidget = new QTabWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(mainTabWidget);
    setLayout(layout);

    mainTabWidget->addTab(this->createGeneralWidget(), tr("General"));
    mainTabWidget->addTab(this->createInputAndOutputWidget(), tr("Input and Output"));
    mainTabWidget->addTab(this->createIsolatedCOMWidget(), tr("Isolated COM"));
    mainTabWidget->addTab(this->createAdvancedWidget(), tr("Advanced"));
}

ManifestToolWidget::~ManifestToolWidget()
{
}

void ManifestToolWidget::saveData()
{
    if (m_suppStartBannerComboBox->currentIndex() >= 0) {
        QVariant data = m_suppStartBannerComboBox->itemData(m_suppStartBannerComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setSuppressStartupBanner(data.toBool());
    }

    if (m_verboseOutputComboBox->currentIndex() >= 0) {
        QVariant data = m_verboseOutputComboBox->itemData(m_verboseOutputComboBox->currentIndex(),
                                                          Qt::UserRole);
        m_tool->setVerboseOutput(data.toBool());
    }

    m_tool->setAssemblyIdentity(m_asmIdentityLineEdit->text());

    if (m_unicodeRespFilesComboBox->currentIndex() >= 0) {
        QVariant data = m_unicodeRespFilesComboBox->itemData(m_unicodeRespFilesComboBox->currentIndex(),
                                                             Qt::UserRole);
        m_tool->setUseUnicodeResponseFiles(data.toBool());
    }

    if (m_useFATWorkaroundComboBox->currentIndex() >= 0) {
        QVariant data = m_useFATWorkaroundComboBox->itemData(m_useFATWorkaroundComboBox->currentIndex(),
                                                             Qt::UserRole);
        m_tool->setUseFAT32Workaround(data.toBool());
    }

    m_tool->setAdditionalManifestFiles(m_additManifestFilesLineEdit->text());
    m_tool->setInputResourceManifests(m_inputResManifestsLineEdit->text());

    if (m_embedManifestComboBox->currentIndex() >= 0) {
        QVariant data = m_embedManifestComboBox->itemData(m_embedManifestComboBox->currentIndex(),
                                                          Qt::UserRole);
        m_tool->setEmbedManifest(data.toBool());
    }

    m_tool->setOutputManifestFile(m_outputManifestFileLineEdit->text());
    m_tool->setManifestResourceFile(m_manifestResFile->text());

    if (m_genCatFilesComboBox->currentIndex() >= 0) {
        QVariant data = m_genCatFilesComboBox->itemData(m_genCatFilesComboBox->currentIndex(),
                                                        Qt::UserRole);
        m_tool->setGenerateCatalogFiles(data.toBool());
    }

    m_tool->setDependencyInformationFile(m_depInfoFileLineEdit->text());

    m_tool->setTypeLibraryFile(m_typeLibFileLineEdit->text());
    m_tool->setRegistrarScriptFile(m_regScriptFileLineEdit->text());
    m_tool->setComponentFileName(m_compFileNameLineEdit->text());
    m_tool->setReplacementsFile(m_replFileLineEdit->text());

    if (m_updateFileHashComboBox->currentIndex() >= 0) {
        QVariant data = m_updateFileHashComboBox->itemData(m_updateFileHashComboBox->currentIndex(),
                                                        Qt::UserRole);
        m_tool->setUpdateFileHashes(data.toBool());
    }

    m_tool->setUpdateFileHashesSearchPath(m_updtFileHashSearchLineEdit->text());
}

QWidget *ManifestToolWidget::createGeneralWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_suppStartBannerComboBox = new QComboBox;
    m_suppStartBannerComboBox->addItem(tr("No"), QVariant(false));
    m_suppStartBannerComboBox->addItem(tr("Yes (/nologo)"), QVariant(true));

    if (m_tool->suppressStartupBanner())
        m_suppStartBannerComboBox->setCurrentIndex(1);
    else
        m_suppStartBannerComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Suppress Startup Banner"),
                                m_suppStartBannerComboBox,
                                tr("Suppresses the display of the startup banner and information messages."));

    m_verboseOutputComboBox = new QComboBox;
    m_verboseOutputComboBox->addItem(tr("No"), QVariant(false));
    m_verboseOutputComboBox->addItem(tr("Yes (/verbose)"), QVariant(true));

    if (m_tool->verboseOutput())
        m_verboseOutputComboBox->setCurrentIndex(1);
    else
        m_verboseOutputComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Verbose Output"),
                                m_verboseOutputComboBox,
                                tr("Displays additional information during manifest generation."));

    m_asmIdentityLineEdit = new QLineEdit;
    m_asmIdentityLineEdit->setText(m_tool->assemblyIdentity());
    basicWidget->insertTableRow(tr("Assembly Identity"),
                                m_asmIdentityLineEdit,
                                tr("Specifies the attributes of the <assemblyIdentity> element in the manifest."));

    m_unicodeRespFilesComboBox = new QComboBox;
    m_unicodeRespFilesComboBox->addItem(tr("No"), QVariant(false));
    m_unicodeRespFilesComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->useUnicodeResponseFiles())
        m_unicodeRespFilesComboBox->setCurrentIndex(1);
    else
        m_unicodeRespFilesComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Use Unicode Response Files"),
                                m_unicodeRespFilesComboBox,
                                tr("Instructs the project system to generate UNICODE response files when spawning the manifest tool. Set this property to “Yes” when files in the project have UNICODE paths."));

    m_useFATWorkaroundComboBox = new QComboBox;
    m_useFATWorkaroundComboBox->addItem(tr("No"), QVariant(false));
    m_useFATWorkaroundComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->useFAT32Workaround())
        m_useFATWorkaroundComboBox->setCurrentIndex(1);
    else
        m_useFATWorkaroundComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Use FAT32 Workaround"),
                                m_useFATWorkaroundComboBox,
                                tr("Work-around file timestamp issues on FAT32 drives, to let the manifest tool update correctly."));
    return basicWidget;
}

QWidget *ManifestToolWidget::createInputAndOutputWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_additManifestFilesLineEdit = new QLineEdit;
    m_additManifestFilesLineEdit->setText(m_tool->additionalManifestFiles());
    basicWidget->insertTableRow(tr("Additional Manifest Files"),
                                m_additManifestFilesLineEdit,
                                tr("Specifies the additional user manifest fragment files to merge into the manifest."));

    m_inputResManifestsLineEdit = new QLineEdit;
    m_inputResManifestsLineEdit->setText(m_tool->inputResourceManifests());
    basicWidget->insertTableRow(tr("Input Resource Manifests"),
                                m_inputResManifestsLineEdit,
                                tr("Specifies the input manifest stored as resources."));

    m_embedManifestComboBox = new QComboBox;
    m_embedManifestComboBox->addItem(tr("No"), QVariant(false));
    m_embedManifestComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->embedManifest())
        m_embedManifestComboBox->setCurrentIndex(1);
    else
        m_embedManifestComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Embed Manifest"),
                                m_embedManifestComboBox,
                                tr("Specifies if the manifest file should be embedded in the assembly or created as a stand-alone file."));

    m_outputManifestFileLineEdit = new QLineEdit;
    m_outputManifestFileLineEdit->setText(m_tool->outputManifestFile());
    basicWidget->insertTableRow(tr("Output Manifest File"),
                                m_outputManifestFileLineEdit,
                                tr("Specifies the output manifest file if the manifest is not embedded in the assembly."));

    m_manifestResFile = new QLineEdit;
    m_manifestResFile->setText(m_tool->manifestResourceFile());
    basicWidget->insertTableRow(tr("Manifest Resource File"),
                                m_manifestResFile,
                                tr("Specifies the output resources file used to embed the manifest into the project output."));

    m_genCatFilesComboBox = new QComboBox;
    m_genCatFilesComboBox->addItem(tr("No"), QVariant(false));
    m_genCatFilesComboBox->addItem(tr("Yes (/makecdfs)"), QVariant(true));

    if (m_tool->generateCatalogFiles())
        m_genCatFilesComboBox->setCurrentIndex(1);
    else
        m_genCatFilesComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Generate Catalog Files"),
                                m_genCatFilesComboBox,
                                tr("Generates .cdf files to make catalogs."));

    m_depInfoFileLineEdit = new QLineEdit;
    m_depInfoFileLineEdit->setText(m_tool->dependencyInformationFile());
    basicWidget->insertTableRow(tr("Dependency Information File"),
                                m_depInfoFileLineEdit,
                                tr("Specifies the dependency information file used by Visual Studio to track build dependency information for the manifest tool."));
    return basicWidget;
}

QWidget *ManifestToolWidget::createIsolatedCOMWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_typeLibFileLineEdit = new QLineEdit;
    m_typeLibFileLineEdit->setText(m_tool->typeLibraryFile());
    basicWidget->insertTableRow(tr("Type Library File"),
                                m_typeLibFileLineEdit,
                                tr("Specifies the type library to use for regfree COM manifest support."));

    m_regScriptFileLineEdit = new QLineEdit;
    m_regScriptFileLineEdit->setText(m_tool->registrarScriptFile());
    basicWidget->insertTableRow(tr("Type Library File"),
                                m_regScriptFileLineEdit,
                                tr("Specifies the register script file to use for regfree COM manifest support."));

    m_compFileNameLineEdit = new QLineEdit;
    m_compFileNameLineEdit->setText(m_tool->componentFileName());
    basicWidget->insertTableRow(tr("Type Library File"),
                                m_compFileNameLineEdit,
                                tr("Specifies the file name of the component that is built from the .rgs or .tlb specifies."));

    m_replFileLineEdit = new QLineEdit;
    m_replFileLineEdit->setText(m_tool->replacementsFile());
    basicWidget->insertTableRow(tr("Type Library File"),
                                m_replFileLineEdit,
                                tr("Specifies the XML replacement file used to replace strings in the .rgs file."));
    return basicWidget;
}

QWidget *ManifestToolWidget::createAdvancedWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_updateFileHashComboBox = new QComboBox;
    m_updateFileHashComboBox->addItem(tr("No"), QVariant(false));
    m_updateFileHashComboBox->addItem(tr("Yes (/hashupdate)"), QVariant(true));

    if (m_tool->updateFileHashes())
        m_updateFileHashComboBox->setCurrentIndex(1);
    else
        m_updateFileHashComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Update File Hashes"),
                                m_updateFileHashComboBox,
                                tr("Specifies if the file hashes should be updated."));

    m_updtFileHashSearchLineEdit = new QLineEdit;
    m_updtFileHashSearchLineEdit->setText(m_tool->updateFileHashesSearchPath());
    basicWidget->insertTableRow(tr("Type Library File"),
                                m_updtFileHashSearchLineEdit,
                                tr("Specifies the search path to use when updating file hashes."));
    return basicWidget;
}

} // namespace Internal
} // namespace VcProjectManager
