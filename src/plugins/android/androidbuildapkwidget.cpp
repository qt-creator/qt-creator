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

const int minApiSupported = 9;

AndroidBuildApkWidget::AndroidBuildApkWidget(AndroidBuildApkStep *step)
    : ProjectExplorer::BuildStepConfigWidget(),
      m_ui(new Ui::AndroidBuildApkWidget),
      m_step(step)
{
    m_ui->setupUi(this);

    // Target sdk combobox
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
            this, &AndroidBuildApkWidget::setTargetSdk);

    // deployment options
    connect(m_ui->ministroOption, &QAbstractButton::clicked,
            m_step, &AndroidBuildApkStep::setUseMinistro);

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
