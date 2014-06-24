/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "androiddeployqtwidget.h"
#include "ui_androiddeployqtwidget.h"

#include "androidcreatekeystorecertificate.h"
#include "androiddeployqtstep.h"
#include "androidmanager.h"
#include "createandroidmanifestwizard.h"
#include "androidextralibrarylistmodel.h"

#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <utils/fancylineedit.h>
#include <utils/pathchooser.h>

#include <QFileDialog>

#include <algorithm>


using namespace Android;
using namespace Internal;

AndroidDeployQtWidget::AndroidDeployQtWidget(AndroidDeployQtStep *step)
    : ProjectExplorer::BuildStepConfigWidget(),
      m_ui(new Ui::AndroidDeployQtWidget),
      m_step(step),
      m_currentBuildConfiguration(0),
      m_ignoreChange(false)
{
    m_ui->setupUi(this);

    // Target sdk combobox
    int minApiLevel = 9;
    QStringList targets = AndroidConfig::apiLevelNamesFor(AndroidConfigurations::currentConfig().sdkTargets(minApiLevel));
    m_ui->targetSDKComboBox->addItems(targets);
    m_ui->targetSDKComboBox->setCurrentIndex(targets.indexOf(step->buildTargetSdk()));

    // deployment option
    switch (m_step->deployAction()) {
    case AndroidDeployQtStep::MinistroDeployment:
        m_ui->ministroOption->setChecked(true);
        break;
    case AndroidDeployQtStep::DebugDeployment:
        m_ui->temporaryQtOption->setChecked(true);
        break;
    case AndroidDeployQtStep::BundleLibrariesDeployment:
        m_ui->bundleQtOption->setChecked(true);
        break;
    default:
        // can't happen
        break;
    }

    // signing
    m_ui->signPackageCheckBox->setChecked(m_step->signPackage());
    m_ui->KeystoreLocationPathChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui->KeystoreLocationPathChooser->lineEdit()->setReadOnly(true);
    m_ui->KeystoreLocationPathChooser->setPath(m_step->keystorePath().toUserOutput());
    m_ui->KeystoreLocationPathChooser->setInitialBrowsePathBackup(QDir::homePath());
    m_ui->KeystoreLocationPathChooser->setPromptDialogFilter(tr("Keystore files (*.keystore *.jks)"));
    m_ui->KeystoreLocationPathChooser->setPromptDialogTitle(tr("Select keystore file"));
    m_ui->signingDebugWarningIcon->hide();
    m_ui->signingDebugWarningLabel->hide();
    signPackageCheckBoxToggled(m_step->signPackage());

    m_ui->verboseOutputCheckBox->setChecked(m_step->verboseOutput());
    m_ui->openPackageLocationCheckBox->setChecked(m_step->openPackageLocation());

    bool oldFiles = AndroidManager::checkForQt51Files(m_step->project()->projectDirectory());
    m_ui->oldFilesWarningIcon->setVisible(oldFiles);
    m_ui->oldFilesWarningLabel->setVisible(oldFiles);

    // target sdk
    connect(m_ui->targetSDKComboBox, SIGNAL(activated(QString)), SLOT(setTargetSdk(QString)));

    // deployment options
    connect(m_ui->ministroOption, SIGNAL(clicked()), SLOT(setMinistro()));
    connect(m_ui->temporaryQtOption, SIGNAL(clicked()), SLOT(setDeployLocalQtLibs()));
    connect(m_ui->bundleQtOption, SIGNAL(clicked()), SLOT(setBundleQtLibs()));

    connect(m_ui->installMinistroButton, SIGNAL(clicked()), SLOT(installMinistro()));
    connect(m_ui->cleanLibsPushButton, SIGNAL(clicked()), SLOT(cleanLibsOnDevice()));
    connect(m_ui->resetDefaultDevices, SIGNAL(clicked()), SLOT(resetDefaultDevices()));
    connect(m_ui->openPackageLocationCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(openPackageLocationCheckBoxToggled(bool)));
    connect(m_ui->verboseOutputCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(verboseOutputCheckBoxToggled(bool)));

    //signing
    connect(m_ui->signPackageCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(signPackageCheckBoxToggled(bool)));
    connect(m_ui->KeystoreCreatePushButton, SIGNAL(clicked()),
            this, SLOT(createKeyStore()));
    connect(m_ui->KeystoreLocationPathChooser, SIGNAL(pathChanged(QString)),
            SLOT(updateKeyStorePath(QString)));
    connect(m_ui->certificatesAliasComboBox, SIGNAL(activated(QString)),
            this, SLOT(certificatesAliasComboBoxActivated(QString)));
    connect(m_ui->certificatesAliasComboBox, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(certificatesAliasComboBoxCurrentIndexChanged(QString)));

    activeBuildConfigurationChanged();
    connect(m_step->target(), SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(activeBuildConfigurationChanged()));

    connect(m_ui->inputFileComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(inputFileComboBoxIndexChanged()));

    updateInputFileUi();
    connect(m_step, SIGNAL(inputFileChanged()),
            this, SLOT(updateInputFileUi()));

    connect(m_ui->createAndroidManifestButton, SIGNAL(clicked()),
            this, SLOT(createManifestButton()));

    m_extraLibraryListModel = new AndroidExtraLibraryListModel(static_cast<QmakeProjectManager::QmakeProject *>(m_step->project()),
                                                               this);
    m_ui->androidExtraLibsListView->setModel(m_extraLibraryListModel);

    connect(m_ui->androidExtraLibsListView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(checkEnableRemoveButton()));

    connect(m_ui->addAndroidExtraLibButton, SIGNAL(clicked()), this, SLOT(addAndroidExtraLib()));
    connect(m_ui->removeAndroidExtraLibButton, SIGNAL(clicked()), this, SLOT(removeAndroidExtraLib()));

    connect(m_extraLibraryListModel, SIGNAL(enabledChanged(bool)),
            m_ui->additionalLibrariesGroupBox, SLOT(setEnabled(bool)));

    m_ui->additionalLibrariesGroupBox->setEnabled(m_extraLibraryListModel->isEnabled());
}

AndroidDeployQtWidget::~AndroidDeployQtWidget()
{
    delete m_ui;
}

void AndroidDeployQtWidget::createManifestButton()
{
    CreateAndroidManifestWizard wizard(m_step->target());
    wizard.exec();
}

void AndroidDeployQtWidget::updateInputFileUi()
{
    QmakeProjectManager::QmakeProject *project
            = static_cast<QmakeProjectManager::QmakeProject *>(m_step->project());
    QList<QmakeProjectManager::QmakeProFileNode *> nodes = project->applicationProFiles();
    int size = nodes.size();
    if (size == 0 || size == 1) {
        // there's nothing to select, e.g. before parsing
        m_ui->inputFileLabel->setVisible(false);
        m_ui->inputFileComboBox->setVisible(false);
    } else {
        m_ignoreChange = true;
        m_ui->inputFileLabel->setVisible(true);
        m_ui->inputFileComboBox->setVisible(true);

        m_ui->inputFileComboBox->clear();
        foreach (QmakeProjectManager::QmakeProFileNode *node, nodes)
            m_ui->inputFileComboBox->addItem(node->displayName(), node->path());

        int index = m_ui->inputFileComboBox->findData(m_step->proFilePathForInputFile());
        m_ui->inputFileComboBox->setCurrentIndex(index);
        m_ignoreChange = false;
    }
}

void AndroidDeployQtWidget::inputFileComboBoxIndexChanged()
{
    if (m_ignoreChange)
        return;
    QString proFilePath = m_ui->inputFileComboBox->itemData(m_ui->inputFileComboBox->currentIndex()).toString();
    m_step->setProFilePathForInputFile(proFilePath);
}

QString AndroidDeployQtWidget::displayName() const
{
    return tr("<b>Deploy configurations</b>");
}

QString AndroidDeployQtWidget::summaryText() const
{
    return displayName();
}

void AndroidDeployQtWidget::setTargetSdk(const QString &sdk)
{
    m_step->setBuildTargetSdk(sdk);
}

void AndroidDeployQtWidget::setMinistro()
{
    m_step->setDeployAction(AndroidDeployQtStep::MinistroDeployment);
}

void AndroidDeployQtWidget::setDeployLocalQtLibs()
{
    m_step->setDeployAction(AndroidDeployQtStep::DebugDeployment);
}

void AndroidDeployQtWidget::setBundleQtLibs()
{
    m_step->setDeployAction(AndroidDeployQtStep::BundleLibrariesDeployment);
}

void AndroidDeployQtWidget::installMinistro()
{
    QString packagePath =
        QFileDialog::getOpenFileName(this, tr("Qt Android Smart Installer"),
                                     QDir::homePath(), tr("Android package (*.apk)"));
    if (!packagePath.isEmpty())
        AndroidManager::installQASIPackage(m_step->target(), packagePath);
}

void AndroidDeployQtWidget::cleanLibsOnDevice()
{
    AndroidManager::cleanLibsOnDevice(m_step->target());
}

void AndroidDeployQtWidget::resetDefaultDevices()
{
    AndroidConfigurations::clearDefaultDevices(m_step->project());
}

void AndroidDeployQtWidget::signPackageCheckBoxToggled(bool checked)
{
    m_ui->certificatesAliasComboBox->setEnabled(checked);
    m_step->setSignPackage(checked);
    updateSigningWarning();
    if (!checked)
        return;
    if (!m_step->keystorePath().isEmpty())
        setCertificates();
}

void AndroidDeployQtWidget::createKeyStore()
{
    AndroidCreateKeystoreCertificate d;
    if (d.exec() != QDialog::Accepted)
        return;
    m_ui->KeystoreLocationPathChooser->setPath(d.keystoreFilePath().toUserOutput());
    m_step->setKeystorePath(d.keystoreFilePath());
    m_step->setKeystorePassword(d.keystorePassword());
    m_step->setCertificateAlias(d.certificateAlias());
    m_step->setCertificatePassword(d.certificatePassword());
    setCertificates();
}

void AndroidDeployQtWidget::setCertificates()
{
    QAbstractItemModel *certificates = m_step->keystoreCertificates();
    m_ui->signPackageCheckBox->setChecked(certificates);
    m_ui->certificatesAliasComboBox->setModel(certificates);
}

void AndroidDeployQtWidget::updateKeyStorePath(const QString &path)
{
    Utils::FileName file = Utils::FileName::fromString(path);
    m_step->setKeystorePath(file);
    m_ui->signPackageCheckBox->setChecked(!file.isEmpty());
    if (!file.isEmpty())
        setCertificates();
}

void AndroidDeployQtWidget::certificatesAliasComboBoxActivated(const QString &alias)
{
    if (alias.length())
        m_step->setCertificateAlias(alias);
}

void AndroidDeployQtWidget::certificatesAliasComboBoxCurrentIndexChanged(const QString &alias)
{
    if (alias.length())
        m_step->setCertificateAlias(alias);
}

void AndroidDeployQtWidget::openPackageLocationCheckBoxToggled(bool checked)
{
    m_step->setOpenPackageLocation(checked);
}

void AndroidDeployQtWidget::verboseOutputCheckBoxToggled(bool checked)
{
    m_step->setVerboseOutput(checked);
}

void AndroidDeployQtWidget::activeBuildConfigurationChanged()
{
    if (m_currentBuildConfiguration)
        disconnect(m_currentBuildConfiguration, SIGNAL(qmakeBuildConfigurationChanged()),
                   this, SLOT(updateSigningWarning()));
    updateSigningWarning();
    QmakeProjectManager::QmakeBuildConfiguration *bc
            = qobject_cast<QmakeProjectManager::QmakeBuildConfiguration *>(m_step->target()->activeBuildConfiguration());
    m_currentBuildConfiguration = bc;
    if (bc)
        connect(bc, SIGNAL(qmakeBuildConfigurationChanged()), this, SLOT(updateSigningWarning()));
                m_currentBuildConfiguration = bc;
}

void AndroidDeployQtWidget::updateSigningWarning()
{
    QmakeProjectManager::QmakeBuildConfiguration *bc = qobject_cast<QmakeProjectManager::QmakeBuildConfiguration *>(m_step->target()->activeBuildConfiguration());
    bool debug = bc && (bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild);
    if (m_step->signPackage() && debug) {
        m_ui->signingDebugWarningIcon->setVisible(true);
        m_ui->signingDebugWarningLabel->setVisible(true);
    } else {
        m_ui->signingDebugWarningIcon->setVisible(false);
        m_ui->signingDebugWarningLabel->setVisible(false);
    }
}

void AndroidDeployQtWidget::addAndroidExtraLib()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this,
                                                          tr("Select additional libraries"),
                                                          m_currentBuildConfiguration->target()->project()->projectDirectory().toString(),
                                                          tr("Libraries (*.so)"));

    if (!fileNames.isEmpty())
        m_extraLibraryListModel->addEntries(fileNames);
}

void AndroidDeployQtWidget::removeAndroidExtraLib()
{
    QModelIndexList removeList = m_ui->androidExtraLibsListView->selectionModel()->selectedIndexes();
    m_extraLibraryListModel->removeEntries(removeList);
}

void AndroidDeployQtWidget::checkEnableRemoveButton()
{
    m_ui->removeAndroidExtraLibButton->setEnabled(m_ui->androidExtraLibsListView->selectionModel()->hasSelection());
}
