/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "androidbuildapkstep.h"
#include "androidbuildapkwidget.h"
#include "androidconfigurations.h"
#include "androidcreatekeystorecertificate.h"
#include "androidmanager.h"
#include "androidsdkmanager.h"
#include "createandroidmanifestwizard.h"
#include "ui_androidbuildapkwidget.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/fancylineedit.h>
#include <utils/pathchooser.h>
#include <utils/utilsicons.h>

#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidBuildApkInnerWidget::AndroidBuildApkInnerWidget(AndroidBuildApkStep *step)
    : ProjectExplorer::BuildStepConfigWidget(step),
      m_ui(new Ui::AndroidBuildApkWidget),
      m_step(step)
{
    m_ui->setupUi(this);
    setDisplayName("<b>" + tr("Build Android APK") + "</b>");
    setSummaryText(displayName());

    // Target sdk combobox
    const int minApiSupported = AndroidManager::apiLevelRange().first;
    QStringList targets = AndroidConfig::apiLevelNamesFor(AndroidConfigurations::sdkManager()->
                                                          filteredSdkPlatforms(minApiSupported));
    targets.removeDuplicates();
    m_ui->targetSDKComboBox->addItems(targets);
    m_ui->targetSDKComboBox->setCurrentIndex(targets.indexOf(AndroidManager::buildTargetSDK(step->target())));

    // Ministro
    if (m_step->useMinistro())
        m_ui->ministroOption->setChecked(true);

    // signing
    m_ui->signPackageCheckBox->setChecked(m_step->signPackage());
    m_ui->KeystoreLocationPathChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui->KeystoreLocationPathChooser->lineEdit()->setReadOnly(true);
    m_ui->KeystoreLocationPathChooser->setPath(m_step->keystorePath().toUserOutput());
    m_ui->KeystoreLocationPathChooser->setInitialBrowsePathBackup(QDir::homePath());
    m_ui->KeystoreLocationPathChooser->setPromptDialogFilter(tr("Keystore files (*.keystore *.jks)"));
    m_ui->KeystoreLocationPathChooser->setPromptDialogTitle(tr("Select Keystore File"));
    m_ui->signingDebugWarningIcon->setPixmap(Utils::Icons::WARNING.pixmap());
    m_ui->signingDebugWarningIcon->hide();
    m_ui->signingDebugWarningLabel->hide();
    signPackageCheckBoxToggled(m_step->signPackage());

    m_ui->verboseOutputCheckBox->setChecked(m_step->verboseOutput());
    m_ui->openPackageLocationCheckBox->setChecked(m_step->openPackageLocation());
    m_ui->addDebuggerCheckBox->setChecked(m_step->addDebugger());

    // target sdk
    connect(m_ui->targetSDKComboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            this, &AndroidBuildApkInnerWidget::setTargetSdk);

    // deployment options
    connect(m_ui->ministroOption, &QAbstractButton::clicked,
            m_step, &AndroidBuildApkStep::setUseMinistro);

    connect(m_ui->openPackageLocationCheckBox, &QAbstractButton::toggled,
            this, &AndroidBuildApkInnerWidget::openPackageLocationCheckBoxToggled);
    connect(m_ui->verboseOutputCheckBox, &QAbstractButton::toggled,
            this, &AndroidBuildApkInnerWidget::verboseOutputCheckBoxToggled);
    connect(m_ui->addDebuggerCheckBox, &QAbstractButton::toggled,
            m_step, &AndroidBuildApkStep::setAddDebugger);

    //signing
    connect(m_ui->signPackageCheckBox, &QAbstractButton::toggled,
            this, &AndroidBuildApkInnerWidget::signPackageCheckBoxToggled);
    connect(m_ui->KeystoreCreatePushButton, &QAbstractButton::clicked,
            this, &AndroidBuildApkInnerWidget::createKeyStore);
    connect(m_ui->KeystoreLocationPathChooser, &Utils::PathChooser::pathChanged,
            this, &AndroidBuildApkInnerWidget::updateKeyStorePath);
    connect(m_ui->certificatesAliasComboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            this, &AndroidBuildApkInnerWidget::certificatesAliasComboBoxActivated);
    connect(m_ui->certificatesAliasComboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
            this, &AndroidBuildApkInnerWidget::certificatesAliasComboBoxCurrentIndexChanged);

    connect(m_step->buildConfiguration(), &ProjectExplorer::BuildConfiguration::buildTypeChanged,
            this, &AndroidBuildApkInnerWidget::updateSigningWarning);

    updateSigningWarning();
}

AndroidBuildApkInnerWidget::~AndroidBuildApkInnerWidget()
{
    delete m_ui;
}

void AndroidBuildApkInnerWidget::setTargetSdk(const QString &sdk)
{
    m_step->setBuildTargetSdk(sdk);
}

void AndroidBuildApkInnerWidget::signPackageCheckBoxToggled(bool checked)
{
    m_ui->certificatesAliasComboBox->setEnabled(checked);
    m_step->setSignPackage(checked);
    m_ui->addDebuggerCheckBox->setChecked(!checked);
    updateSigningWarning();
    if (!checked)
        return;
    if (!m_step->keystorePath().isEmpty())
        setCertificates();
}

void AndroidBuildApkInnerWidget::createKeyStore()
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

void AndroidBuildApkInnerWidget::setCertificates()
{
    QAbstractItemModel *certificates = m_step->keystoreCertificates();
    if (certificates) {
        m_ui->signPackageCheckBox->setChecked(certificates);
        m_ui->certificatesAliasComboBox->setModel(certificates);
    }
}

void AndroidBuildApkInnerWidget::updateKeyStorePath(const QString &path)
{
    Utils::FileName file = Utils::FileName::fromString(path);
    m_step->setKeystorePath(file);
    m_ui->signPackageCheckBox->setChecked(!file.isEmpty());
    if (!file.isEmpty())
        setCertificates();
}

void AndroidBuildApkInnerWidget::certificatesAliasComboBoxActivated(const QString &alias)
{
    if (alias.length())
        m_step->setCertificateAlias(alias);
}

void AndroidBuildApkInnerWidget::certificatesAliasComboBoxCurrentIndexChanged(const QString &alias)
{
    if (alias.length())
        m_step->setCertificateAlias(alias);
}

void AndroidBuildApkInnerWidget::openPackageLocationCheckBoxToggled(bool checked)
{
    m_step->setOpenPackageLocation(checked);
}

void AndroidBuildApkInnerWidget::verboseOutputCheckBoxToggled(bool checked)
{
    m_step->setVerboseOutput(checked);
}

void AndroidBuildApkInnerWidget::updateSigningWarning()
{
    bool nonRelease = m_step->buildConfiguration()->buildType()
            != ProjectExplorer::BuildConfiguration::Release;
    if (m_step->signPackage() && nonRelease) {
        m_ui->signingDebugWarningIcon->setVisible(true);
        m_ui->signingDebugWarningLabel->setVisible(true);
    } else {
        m_ui->signingDebugWarningIcon->setVisible(false);
        m_ui->signingDebugWarningLabel->setVisible(false);
    }
}


// AndroidBuildApkWidget

AndroidBuildApkWidget::AndroidBuildApkWidget(AndroidBuildApkStep *step) :
    BuildStepConfigWidget(step),
    m_step(step)
{
    setDisplayName("<b>" + tr("Build Android APK") + "</b>");
    setSummaryText("<b>" + tr("Build Android APK") + "</b>");

    m_extraLibraryListModel = new AndroidExtraLibraryListModel(m_step->target(), this);

    auto base = new AndroidBuildApkInnerWidget(step);
    base->layout()->setContentsMargins(0, 0, 0, 0);

    auto createTemplatesGroupBox = new QGroupBox(tr("Android"));
    createTemplatesGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto createAndroidTemplatesButton = new QPushButton(tr("Create Templates"));

    auto horizontalLayout = new QHBoxLayout(createTemplatesGroupBox);
    horizontalLayout->addWidget(createAndroidTemplatesButton);
    horizontalLayout->addStretch(1);

    auto additionalLibrariesGroupBox = new QGroupBox(tr("Additional Libraries"));
    additionalLibrariesGroupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    m_androidExtraLibsListView = new QListView;
    m_androidExtraLibsListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_androidExtraLibsListView->setToolTip(tr("List of extra libraries to include in Android package and load on startup."));
    m_androidExtraLibsListView->setModel(m_extraLibraryListModel);

    auto addAndroidExtraLibButton = new QToolButton;
    addAndroidExtraLibButton->setText(tr("Add..."));
    addAndroidExtraLibButton->setToolTip(tr("Select library to include in package."));
    addAndroidExtraLibButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    addAndroidExtraLibButton->setToolButtonStyle(Qt::ToolButtonTextOnly);

    m_removeAndroidExtraLibButton = new QToolButton;
    m_removeAndroidExtraLibButton->setText(tr("Remove"));
    m_removeAndroidExtraLibButton->setToolTip(tr("Remove currently selected library from list."));

    auto androidExtraLibsButtonLayout = new QVBoxLayout();
    androidExtraLibsButtonLayout->addWidget(addAndroidExtraLibButton);
    androidExtraLibsButtonLayout->addWidget(m_removeAndroidExtraLibButton);
    androidExtraLibsButtonLayout->addStretch(1);

    auto androidExtraLibsLayout = new QHBoxLayout(additionalLibrariesGroupBox);
    androidExtraLibsLayout->addWidget(m_androidExtraLibsListView);
    androidExtraLibsLayout->addLayout(androidExtraLibsButtonLayout);

    auto topLayout = new QVBoxLayout(this);
    topLayout->addWidget(base);
    topLayout->addWidget(createTemplatesGroupBox);
    topLayout->addWidget(additionalLibrariesGroupBox);

    connect(createAndroidTemplatesButton, &QAbstractButton::clicked, this, [this] {
        CreateAndroidManifestWizard wizard(m_step->target());
        wizard.exec();
    });

    connect(addAndroidExtraLibButton, &QAbstractButton::clicked,
            this, &AndroidBuildApkWidget::addAndroidExtraLib);

    connect(m_removeAndroidExtraLibButton, &QAbstractButton::clicked,
            this, &AndroidBuildApkWidget::removeAndroidExtraLib);

    connect(m_androidExtraLibsListView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AndroidBuildApkWidget::checkEnableRemoveButton);

    connect(m_extraLibraryListModel, &AndroidExtraLibraryListModel::enabledChanged,
            additionalLibrariesGroupBox, &QWidget::setEnabled);

    Target *target = m_step->target();
    RunConfiguration *rc = target->activeRunConfiguration();
    const ProjectNode *node = rc ? target->project()->findNodeForBuildKey(rc->buildKey()) : nullptr;
    additionalLibrariesGroupBox->setEnabled(node && !node->parseInProgress());
}

void AndroidBuildApkWidget::addAndroidExtraLib()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this,
                                                          tr("Select additional libraries"),
                                                          QDir::homePath(),
                                                          tr("Libraries (*.so)"));

    if (!fileNames.isEmpty())
        m_extraLibraryListModel->addEntries(fileNames);
}

void AndroidBuildApkWidget::removeAndroidExtraLib()
{
    QModelIndexList removeList = m_androidExtraLibsListView->selectionModel()->selectedIndexes();
    m_extraLibraryListModel->removeEntries(removeList);
}

void AndroidBuildApkWidget::checkEnableRemoveButton()
{
    m_removeAndroidExtraLibButton->setEnabled(m_androidExtraLibsListView->selectionModel()->hasSelection());
}

} // Internal
} // Android
