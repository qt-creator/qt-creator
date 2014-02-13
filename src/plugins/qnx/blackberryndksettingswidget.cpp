/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberryndksettingswidget.h"
#include "ui_blackberryndksettingswidget.h"
#include "qnxutils.h"
#include "blackberrysigningutils.h"
#include "blackberrysetupwizard.h"

#include "blackberryconfigurationmanager.h"
#include "blackberryconfiguration.h"

#include <utils/pathchooser.h>

#include <coreplugin/icore.h>

#include <QMessageBox>
#include <QFileDialog>

#include <QStandardItemModel>
#include <QTreeWidgetItem>

namespace Qnx {
namespace Internal {

static QIcon invalidConfigIcon(QLatin1String(":/projectexplorer/images/compile_error.png"));

BlackBerryNDKSettingsWidget::BlackBerryNDKSettingsWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui_BlackBerryNDKSettingsWidget),
    m_bbConfigManager(BlackBerryConfigurationManager::instance()),
    m_autoDetectedNdks(0),
    m_manualNdks(0)
{
    m_ui->setupUi(this);

    updateInfoTable(0);

    m_activatedTargets << m_bbConfigManager.activeConfigurations();

    m_ui->ndksTreeWidget->header()->setResizeMode(QHeaderView::Stretch);
    m_ui->ndksTreeWidget->header()->setStretchLastSection(false);
    m_ui->ndksTreeWidget->setHeaderItem(new QTreeWidgetItem(QStringList() << tr("API Level") << tr("Environment File")));
    m_ui->ndksTreeWidget->setTextElideMode(Qt::ElideNone);
    m_ui->ndksTreeWidget->setColumnCount(2);
    m_autoDetectedNdks = new QTreeWidgetItem(m_ui->ndksTreeWidget);
    m_autoDetectedNdks->setText(0, tr("Auto-Detected"));
    m_autoDetectedNdks->setFirstColumnSpanned(true);
    m_autoDetectedNdks->setFlags(Qt::ItemIsEnabled);
    m_manualNdks = new QTreeWidgetItem(m_ui->ndksTreeWidget);
    m_manualNdks->setText(0, tr("Manual"));
    m_manualNdks->setFirstColumnSpanned(true);
    m_manualNdks->setFlags(Qt::ItemIsEnabled);

    m_ui->ndksTreeWidget->expandAll();

    connect(m_ui->wizardButton, SIGNAL(clicked()), this, SLOT(launchBlackBerrySetupWizard()));
    connect(m_ui->addNdkButton, SIGNAL(clicked()), this, SLOT(addNdkTarget()));
    connect(m_ui->removeNdkButton, SIGNAL(clicked()), this, SLOT(removeNdkTarget()));
    connect(m_ui->activateNdkTargetButton, SIGNAL(clicked()), this, SLOT(activateNdkTarget()));
    connect(m_ui->deactivateNdkTargetButton, SIGNAL(clicked()), this, SLOT(deactivateNdkTarget()));
    connect(m_ui->cleanUpButton, SIGNAL(clicked()), this, SLOT(cleanUp()));
    connect(m_ui->ndksTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(updateInfoTable(QTreeWidgetItem*)));
    connect(this, SIGNAL(targetsUpdated()), this, SLOT(populateDefaultConfigurationCombo()));

    // BlackBerryConfigurationManager.settingsChanged signal may be emitted multiple times
    // during the same event handling. This would result in multiple updatePage() calls even through
    // just one is needed.
    // QTimer allows to merge those multiple signal emits into a single updatePage() call.
    m_timer.setSingleShot(true);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(updatePage()));

    updatePage();
    connect(&m_bbConfigManager, SIGNAL(settingsChanged()), &m_timer, SLOT(start()));
}

void BlackBerryNDKSettingsWidget::setWizardMessageVisible(bool visible)
{
    m_ui->wizardLabel->setVisible(visible);
    m_ui->wizardButton->setVisible(visible);
}

bool BlackBerryNDKSettingsWidget::hasActiveNdk() const
{
    return !m_bbConfigManager.configurations().isEmpty();
}

QList<BlackBerryConfiguration *> BlackBerryNDKSettingsWidget::activatedTargets()
{
    return m_activatedTargets;
}

QList<BlackBerryConfiguration *> BlackBerryNDKSettingsWidget::deactivatedTargets()
{
    return m_deactivatedTargets;
}

BlackBerryConfiguration *BlackBerryNDKSettingsWidget::defaultConfiguration() const
{
    const int currentIndex = m_ui->apiLevelCombo->currentIndex();

    return static_cast<BlackBerryConfiguration*>(
            m_ui->apiLevelCombo->itemData(currentIndex).value<void*>());
}

void BlackBerryNDKSettingsWidget::launchBlackBerrySetupWizard() const
{
    BlackBerrySigningUtils &blackBerryUtils = BlackBerrySigningUtils::instance();
    const bool alreadyConfigured = blackBerryUtils.hasRegisteredKeys() && blackBerryUtils.hasDefaultCertificate();

    if (alreadyConfigured) {
        QMessageBox::information(Core::ICore::mainWindow(), tr("Qt Creator"),
            tr("It appears that your BlackBerry environment has already been configured."));
            return;
    }

    BlackBerrySetupWizard wizard(Core::ICore::mainWindow());
    connect(&wizard, SIGNAL(ndkTargetsUpdated()), this, SLOT(updateNdkList()));
    wizard.exec();
}

void BlackBerryNDKSettingsWidget::updateInfoTable(QTreeWidgetItem* currentItem)
{
    BlackBerryConfiguration *config = m_bbConfigManager.configurationFromEnvFile(
            Utils::FileName::fromString(currentItem ? currentItem->text(1) : QString()));
    updateUi(currentItem, config);

    m_ui->informationBox->setVisible(config);
    if (config) {
        m_ui->baseNameLabel->setText(config->displayName());
        m_ui->hostLabel->setText(QDir::toNativeSeparators(config->qnxHost()));
        m_ui->targetLabel->setText(QDir::toNativeSeparators(config->sysRoot().toString()));
        m_ui->versionLabel->setText(config->version().toString());
    }
}

void BlackBerryNDKSettingsWidget::updateNdkList()
{
    qDeleteAll(m_autoDetectedNdks->takeChildren());
    qDeleteAll(m_manualNdks->takeChildren());

    bool enableCleanUp = false;
    foreach (BlackBerryConfiguration *config, m_bbConfigManager.configurations()) {
        QTreeWidgetItem *parent = config->isAutoDetected() ? m_autoDetectedNdks : m_manualNdks;
        QTreeWidgetItem *item = new QTreeWidgetItem(parent);
        item->setText(0, config->displayName());
        item->setText(1, config->ndkEnvFile().toString());
        QFont font;
        font.setBold(config->isActive() || m_activatedTargets.contains(config));
        item->setFont(0, font);
        item->setFont(1, font);
        item->setIcon(0, config->isValid() ? QIcon() : invalidConfigIcon);
        // TODO: Do the same if qmake, qcc, debugger are no longer detected...
        if (!config->isValid()) {
            QString toolTip = tr("Invalid target %1: ").arg(config->targetName());
            if (config->isAutoDetected() && !config->autoDetectionSource().toFileInfo().exists())
                toolTip += QLatin1Char('\n') + tr("- Target no longer installed.");

            if (!config->ndkEnvFile().toFileInfo().exists())
                toolTip += QLatin1Char('\n') + tr("- No NDK environment file found.");

            if (config->qmake4BinaryFile().isEmpty()
                    && config->qmake5BinaryFile().isEmpty())
                toolTip += QLatin1Char('\n') + tr("- No Qt version found.");

            if (config->gccCompiler().isEmpty())
                toolTip += QLatin1Char('\n') + tr("- No compiler found.");

            if (config->deviceDebuger().isEmpty())
                toolTip += QLatin1Char('\n') + tr("- No debugger found for device.");

            if (config->simulatorDebuger().isEmpty())
                toolTip += QLatin1Char('\n') + tr("- No debugger found for simulator.");

            item->setToolTip(0, toolTip);
            enableCleanUp = true;
        }
    }

    m_ui->ndksTreeWidget->setCurrentItem(m_autoDetectedNdks->child(0));
    m_ui->cleanUpButton->setEnabled(enableCleanUp);
}

void BlackBerryNDKSettingsWidget::addNdkTarget()
{
    launchBlackBerryInstallerWizard(BlackBerryInstallerDataHandler::InstallMode);
    emit targetsUpdated();
}

void BlackBerryNDKSettingsWidget::removeNdkTarget()
{
    if (!m_ui->ndksTreeWidget->currentItem())
        return;

    QString ndk = m_ui->ndksTreeWidget->currentItem()->text(0);
    QString envFilePath = m_ui->ndksTreeWidget->currentItem()->text(1);

    BlackBerryConfiguration *config = m_bbConfigManager.configurationFromEnvFile(Utils::FileName::fromString(envFilePath));
    if (!config)
        return;

    if (config->isAutoDetected()) {
        uninstallNdkTarget();
        emit targetsUpdated();
        return;
    }

    QMessageBox::StandardButton button =
            QMessageBox::question(Core::ICore::mainWindow(),
                                  tr("Clean BlackBerry 10 Configuration"),
                                  tr("Are you sure you want to remove:\n %1?").arg(ndk),
                                  QMessageBox::Yes | QMessageBox::No);

    if (button == QMessageBox::Yes) {
        m_activatedTargets.removeOne(config);
        m_deactivatedTargets.removeOne(config);
        m_bbConfigManager.removeConfiguration(config);
        m_manualNdks->removeChild(m_ui->ndksTreeWidget->currentItem());
        emit targetsUpdated();
    }
}

void BlackBerryNDKSettingsWidget::activateNdkTarget()
{
    if (!m_ui->ndksTreeWidget->currentItem())
        return;

    QString envFilePath = m_ui->ndksTreeWidget->currentItem()->text(1);

    BlackBerryConfiguration *config = m_bbConfigManager.configurationFromEnvFile(Utils::FileName::fromString(envFilePath));
    if (config && !m_activatedTargets.contains(config)) {
        m_activatedTargets << config;
        if (m_deactivatedTargets.contains(config))
           m_deactivatedTargets.removeAt(m_deactivatedTargets.indexOf(config));

        updateUi(m_ui->ndksTreeWidget->currentItem(), config);
        emit targetsUpdated();
    }
}

void BlackBerryNDKSettingsWidget::deactivateNdkTarget()
{
    if (!m_ui->ndksTreeWidget->currentItem())
        return;

    QString envFilePath = m_ui->ndksTreeWidget->currentItem()->text(1);

    BlackBerryConfiguration *config = m_bbConfigManager.configurationFromEnvFile(Utils::FileName::fromString(envFilePath));
    if (config && m_activatedTargets.contains(config)) {
        m_deactivatedTargets << config;
        m_activatedTargets.removeAt(m_activatedTargets.indexOf(config));
        updateUi(m_ui->ndksTreeWidget->currentItem(), config);
        emit targetsUpdated();
    }
}

void BlackBerryNDKSettingsWidget::updateUi(QTreeWidgetItem *item, BlackBerryConfiguration *config)
{
    if (!item || !config) {
        m_ui->removeNdkButton->setEnabled(false);
        m_ui->activateNdkTargetButton->setEnabled(false);
        m_ui->deactivateNdkTargetButton->setEnabled(false);
        return;
    }

    const bool contains = m_activatedTargets.contains(config);
    QFont font;
    font.setBold(contains);
    item->setFont(0, font);
    item->setFont(1, font);

    m_ui->activateNdkTargetButton->setEnabled(!contains);
    m_ui->deactivateNdkTargetButton->setEnabled(contains && m_activatedTargets.size() > 1);
    // Disable remove button for auto detected pre-10.2 NDKs (uninstall wizard doesn't handle them)
    m_ui->removeNdkButton->setEnabled(!(config->isAutoDetected()
                                            && QnxUtils::sdkInstallerPath(config->ndkPath()).isEmpty()));
}

void BlackBerryNDKSettingsWidget::uninstallNdkTarget()
{
    const QMessageBox::StandardButton answer = QMessageBox::question(this, tr("Confirmation"),
                                                                     tr("Are you sure you want to uninstall %1?").
                                                                     arg(m_ui->baseNameLabel->text()),
                                                                     QMessageBox::Yes | QMessageBox::No);

    if (answer == QMessageBox::Yes)
        launchBlackBerryInstallerWizard(BlackBerryInstallerDataHandler::UninstallMode, m_ui->versionLabel->text());
}

void BlackBerryNDKSettingsWidget::cleanUp()
{
    foreach (BlackBerryConfiguration *config, m_bbConfigManager.configurations()) {
        if (!config->isValid()) {
            m_activatedTargets.removeOne(config);
            m_deactivatedTargets.removeOne(config);
            m_bbConfigManager.removeConfiguration(config);
        }
    }

    updateNdkList();
}

void BlackBerryNDKSettingsWidget::handleInstallationFinished()
{
    m_bbConfigManager.loadAutoDetectedConfigurations();
    updateNdkList();
}

void BlackBerryNDKSettingsWidget::handleUninstallationFinished()
{
    if (!m_ui->ndksTreeWidget->currentItem())
        return;

    QString targetName = m_ui->ndksTreeWidget->currentItem()->text(0);
    QString envFilePath = m_ui->ndksTreeWidget->currentItem()->text(1);
    // Check if the target is corrrecly uninstalled
    foreach (const NdkInstallInformation &ndk, QnxUtils::installedNdks()) {
        if (ndk.name == targetName)
            return;
    }

    BlackBerryConfiguration *config = m_bbConfigManager.configurationFromEnvFile(Utils::FileName::fromString(envFilePath));
    if (m_activatedTargets.contains(config))
        m_activatedTargets.removeAt(m_activatedTargets.indexOf(config));
    else if (m_deactivatedTargets.contains(config))
        m_deactivatedTargets.removeAt(m_deactivatedTargets.indexOf(config));

    m_bbConfigManager.removeConfiguration(config);

    updateNdkList();
}

void BlackBerryNDKSettingsWidget::populateDefaultConfigurationCombo()
{
    // prevent QComboBox::currentIndexChanged() from being emitted
    m_ui->apiLevelCombo->clear();

    QList<BlackBerryConfiguration*> configurations = m_bbConfigManager.configurations();

    m_ui->apiLevelCombo->addItem(tr("Newest version"),
            QVariant::fromValue(static_cast<void*>(0)));

    if (configurations.isEmpty())
        return;

    int configIndex = 0;

    BlackBerryConfiguration *defaultConfig = m_bbConfigManager.defaultConfiguration();

    foreach (BlackBerryConfiguration *config, configurations) {
        m_ui->apiLevelCombo->addItem(config->displayName(),
                QVariant::fromValue(static_cast<void*>(config)));

        if (config == defaultConfig)
            configIndex = m_ui->apiLevelCombo->count() - 1;
    }

    const int currentIndex = (m_bbConfigManager.newestConfigurationEnabled()) ? 0 : configIndex;

    m_ui->apiLevelCombo->setCurrentIndex(currentIndex);
}

void BlackBerryNDKSettingsWidget::launchBlackBerryInstallerWizard(BlackBerryInstallerDataHandler::Mode mode,
                                                                  const QString& targetVersion)
{
    BlackBerryInstallWizard wizard(mode, BlackBerryInstallerDataHandler::ApiLevel, targetVersion, this);
    if (mode == BlackBerryInstallerDataHandler::InstallMode)
        connect(&wizard, SIGNAL(processFinished()), this, SLOT(handleInstallationFinished()));
    else
        connect(&wizard, SIGNAL(processFinished()), this, SLOT(handleUninstallationFinished()));

    wizard.exec();
}

void BlackBerryNDKSettingsWidget::updatePage()
{
    updateNdkList();
    populateDefaultConfigurationCombo();
}

} // namespace Internal
} // namespace Qnx

