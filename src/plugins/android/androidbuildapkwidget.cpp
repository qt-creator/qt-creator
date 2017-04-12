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
#include "ui_androidbuildapkwidget.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/fancylineedit.h>
#include <utils/pathchooser.h>
#include <utils/utilsicons.h>

#include <QFileDialog>

#include <algorithm>

using namespace Android;
using namespace Internal;

AndroidBuildApkWidget::AndroidBuildApkWidget(AndroidBuildApkStep *step)
    : ProjectExplorer::BuildStepConfigWidget(),
      m_ui(new Ui::AndroidBuildApkWidget),
      m_step(step)
{
    m_ui->setupUi(this);

    m_ui->deprecatedInfoIconLabel->setPixmap(Utils::Icons::INFO.pixmap());

    // Target sdk combobox
    int minApiLevel = 9;
    const AndroidConfig &config = AndroidConfigurations::currentConfig();
    QStringList targets = AndroidConfig::apiLevelNamesFor(config.sdkTargets(minApiLevel));
    targets.removeDuplicates();
    m_ui->targetSDKComboBox->addItems(targets);
    m_ui->targetSDKComboBox->setCurrentIndex(targets.indexOf(AndroidManager::buildTargetSDK(step->target())));

    // deployment option
    switch (m_step->deployAction()) {
    case AndroidBuildApkStep::MinistroDeployment:
        m_ui->ministroOption->setChecked(true);
        break;
    case AndroidBuildApkStep::DebugDeployment:
        m_ui->temporaryQtOption->setChecked(true);
        break;
    case AndroidBuildApkStep::BundleLibrariesDeployment:
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
    m_ui->KeystoreLocationPathChooser->setPromptDialogTitle(tr("Select Keystore File"));
    m_ui->signingDebugWarningIcon->setPixmap(Utils::Icons::WARNING.pixmap());
    m_ui->signingDebugWarningIcon->hide();
    m_ui->signingDebugWarningLabel->hide();
    m_ui->signingDebugDeployErrorIcon->setPixmap(Utils::Icons::CRITICAL.pixmap());
    signPackageCheckBoxToggled(m_step->signPackage());

    m_ui->useGradleCheckBox->setEnabled(config.antScriptsAvailable());
    m_ui->useGradleCheckBox->setChecked(!config.antScriptsAvailable() ||
                                        m_step->useGradle());
    m_ui->verboseOutputCheckBox->setChecked(m_step->verboseOutput());
    m_ui->openPackageLocationCheckBox->setChecked(m_step->openPackageLocation());
    m_ui->addDebuggerCheckBox->setChecked(m_step->addDebugger());

    // target sdk
    connect(m_ui->targetSDKComboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            this, &AndroidBuildApkWidget::setTargetSdk);

    // deployment options
    connect(m_ui->ministroOption, &QAbstractButton::clicked,
            this, &AndroidBuildApkWidget::setMinistro);
    connect(m_ui->temporaryQtOption, &QAbstractButton::clicked,
            this, &AndroidBuildApkWidget::setDeployLocalQtLibs);
    connect(m_ui->bundleQtOption, &QAbstractButton::clicked,
            this, &AndroidBuildApkWidget::setBundleQtLibs);
    connect(m_ui->ministroOption, &QAbstractButton::clicked,
            this, &AndroidBuildApkWidget::updateDebugDeploySigningWarning);
    connect(m_ui->temporaryQtOption, &QAbstractButton::clicked,
            this, &AndroidBuildApkWidget::updateDebugDeploySigningWarning);
    connect(m_ui->bundleQtOption, &QAbstractButton::clicked,
            this, &AndroidBuildApkWidget::updateDebugDeploySigningWarning);

    connect(m_ui->useGradleCheckBox, &QAbstractButton::toggled,
            this, &AndroidBuildApkWidget::useGradleCheckBoxToggled);
    connect(m_ui->openPackageLocationCheckBox, &QAbstractButton::toggled,
            this, &AndroidBuildApkWidget::openPackageLocationCheckBoxToggled);
    connect(m_ui->verboseOutputCheckBox, &QAbstractButton::toggled,
            this, &AndroidBuildApkWidget::verboseOutputCheckBoxToggled);
    connect(m_ui->addDebuggerCheckBox, &QAbstractButton::toggled,
            m_step, &AndroidBuildApkStep::setAddDebugger);

    //signing
    connect(m_ui->signPackageCheckBox, &QAbstractButton::toggled,
            this, &AndroidBuildApkWidget::signPackageCheckBoxToggled);
    connect(m_ui->KeystoreCreatePushButton, &QAbstractButton::clicked,
            this, &AndroidBuildApkWidget::createKeyStore);
    connect(m_ui->KeystoreLocationPathChooser, &Utils::PathChooser::pathChanged,
            this, &AndroidBuildApkWidget::updateKeyStorePath);
    connect(m_ui->certificatesAliasComboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            this, &AndroidBuildApkWidget::certificatesAliasComboBoxActivated);
    connect(m_ui->certificatesAliasComboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
            this, &AndroidBuildApkWidget::certificatesAliasComboBoxCurrentIndexChanged);

    connect(m_step->buildConfiguration(), &ProjectExplorer::BuildConfiguration::buildTypeChanged,
            this, &AndroidBuildApkWidget::updateSigningWarning);

    updateSigningWarning();
    updateDebugDeploySigningWarning();
    QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(step->target()->kit());
    bool qt54 = qt->qtVersion() >= QtSupport::QtVersionNumber(5, 4, 0);
    m_ui->temporaryQtOption->setVisible(qt54);
    m_ui->useGradleCheckBox->setVisible(qt54);
}

AndroidBuildApkWidget::~AndroidBuildApkWidget()
{
    delete m_ui;
}

QString AndroidBuildApkWidget::displayName() const
{
    return tr("<b>Build Android APK</b>");
}

QString AndroidBuildApkWidget::summaryText() const
{
    return displayName();
}

void AndroidBuildApkWidget::setTargetSdk(const QString &sdk)
{
    m_step->setBuildTargetSdk(sdk);
}

void AndroidBuildApkWidget::setMinistro()
{
    m_step->setDeployAction(AndroidBuildApkStep::MinistroDeployment);
}

void AndroidBuildApkWidget::setDeployLocalQtLibs()
{
    m_step->setDeployAction(AndroidBuildApkStep::DebugDeployment);
}

void AndroidBuildApkWidget::setBundleQtLibs()
{
    m_step->setDeployAction(AndroidBuildApkStep::BundleLibrariesDeployment);
}

void AndroidBuildApkWidget::signPackageCheckBoxToggled(bool checked)
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

void AndroidBuildApkWidget::createKeyStore()
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

void AndroidBuildApkWidget::setCertificates()
{
    QAbstractItemModel *certificates = m_step->keystoreCertificates();
    if (certificates) {
        m_ui->signPackageCheckBox->setChecked(certificates);
        m_ui->certificatesAliasComboBox->setModel(certificates);
    }
}

void AndroidBuildApkWidget::updateKeyStorePath(const QString &path)
{
    Utils::FileName file = Utils::FileName::fromString(path);
    m_step->setKeystorePath(file);
    m_ui->signPackageCheckBox->setChecked(!file.isEmpty());
    if (!file.isEmpty())
        setCertificates();
}

void AndroidBuildApkWidget::certificatesAliasComboBoxActivated(const QString &alias)
{
    if (alias.length())
        m_step->setCertificateAlias(alias);
}

void AndroidBuildApkWidget::certificatesAliasComboBoxCurrentIndexChanged(const QString &alias)
{
    if (alias.length())
        m_step->setCertificateAlias(alias);
}

void AndroidBuildApkWidget::openPackageLocationCheckBoxToggled(bool checked)
{
    m_step->setOpenPackageLocation(checked);
}

void AndroidBuildApkWidget::verboseOutputCheckBoxToggled(bool checked)
{
    m_step->setVerboseOutput(checked);
}

void AndroidBuildApkWidget::updateSigningWarning()
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

void AndroidBuildApkWidget::updateDebugDeploySigningWarning()
{
    if (m_step->deployAction() == AndroidBuildApkStep::DebugDeployment) {
        m_ui->signingDebugDeployError->setVisible(true);
        m_ui->signingDebugDeployErrorIcon->setVisible(true);
        m_ui->signPackageCheckBox->setChecked(false);
        m_ui->signPackageCheckBox->setEnabled(false);
    } else {
        m_ui->signingDebugDeployError->setVisible(false);
        m_ui->signingDebugDeployErrorIcon->setVisible(false);
        m_ui->signPackageCheckBox->setEnabled(true);
    }
}

void AndroidBuildApkWidget::useGradleCheckBoxToggled(bool checked)
{
    m_step->setUseGradle(checked);
}
