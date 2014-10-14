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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
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

#include "blackberryconfigurationmanager.h"
#include "blackberryapilevelconfiguration.h"
#include "blackberryruntimeconfiguration.h"

#include <utils/pathchooser.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <QMessageBox>
#include <QFileDialog>

#include <QStandardItemModel>
#include <QTreeWidgetItem>

namespace Qnx {
namespace Internal {

static QIcon invalidConfigIcon(QString::fromLatin1(Core::Constants::ICON_ERROR));

BlackBerryNDKSettingsWidget::BlackBerryNDKSettingsWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui_BlackBerryNDKSettingsWidget),
    m_bbConfigManager(BlackBerryConfigurationManager::instance()),
    m_autoDetectedNdks(0),
    m_manualApiLevel(0)
{
    m_ui->setupUi(this);

    updateInfoTable(0);

    m_activatedApiLevel << m_bbConfigManager->activeApiLevels();

    m_ui->ndksTreeWidget->header()->setSectionResizeMode(QHeaderView::Stretch);
    m_ui->ndksTreeWidget->header()->setStretchLastSection(false);
    m_ui->ndksTreeWidget->setHeaderItem(new QTreeWidgetItem(QStringList() << tr("Configuration")));
    m_ui->ndksTreeWidget->setTextElideMode(Qt::ElideNone);
    m_ui->ndksTreeWidget->setColumnCount(1);

    m_apiLevels = new QTreeWidgetItem(m_ui->ndksTreeWidget);
    m_apiLevels->setText(0, tr("API Levels"));
    m_runtimes = new QTreeWidgetItem(m_ui->ndksTreeWidget);
    m_runtimes->setText(0, tr("Runtimes"));

    m_autoDetectedNdks = new QTreeWidgetItem(m_apiLevels);
    m_autoDetectedNdks->setText(0, tr("Auto-Detected"));
    m_autoDetectedNdks->setFirstColumnSpanned(true);
    m_autoDetectedNdks->setFlags(Qt::ItemIsEnabled);
    m_manualApiLevel = new QTreeWidgetItem(m_apiLevels);
    m_manualApiLevel->setText(0, tr("Manual"));
    m_manualApiLevel->setFirstColumnSpanned(true);
    m_manualApiLevel->setFlags(Qt::ItemIsEnabled);

    m_ui->ndksTreeWidget->expandAll();

    connect(m_ui->addConfigButton, SIGNAL(clicked()), this, SLOT(addConfiguration()));
    connect(m_ui->removeConfigButton, SIGNAL(clicked()), this, SLOT(removeConfiguration()));
    connect(m_ui->activateNdkTargetButton, SIGNAL(clicked()), this, SLOT(activateApiLevel()));
    connect(m_ui->deactivateNdkTargetButton, SIGNAL(clicked()), this, SLOT(deactivateApiLevel()));

    connect(m_ui->cleanUpButton, SIGNAL(clicked()), this, SLOT(cleanUp()));
    connect(m_ui->ndksTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(updateInfoTable(QTreeWidgetItem*)));
    connect(this, SIGNAL(configurationsUpdated()), this, SLOT(populateDefaultConfigurationCombo()));

    // BlackBerryConfigurationManager.settingsChanged signal may be emitted multiple times
    // during the same event handling. This would result in multiple updatePage() calls even through
    // just one is needed.
    // QTimer allows to merge those multiple signal emits into a single updatePage() call.
    m_timer.setSingleShot(true);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(updatePage()));

    updatePage();
    connect(m_bbConfigManager, SIGNAL(settingsChanged()), &m_timer, SLOT(start()));
}

bool BlackBerryNDKSettingsWidget::hasActiveNdk() const
{
    return !m_bbConfigManager->apiLevels().isEmpty();
}

QList<BlackBerryApiLevelConfiguration *> BlackBerryNDKSettingsWidget::activatedApiLevels()
{
    return m_activatedApiLevel;
}

QList<BlackBerryApiLevelConfiguration *> BlackBerryNDKSettingsWidget::deactivatedApiLevels()
{
    return m_deactivatedApiLevel;
}

BlackBerryApiLevelConfiguration *BlackBerryNDKSettingsWidget::defaultApiLevel() const
{
    const int currentIndex = m_ui->apiLevelCombo->currentIndex();

    return static_cast<BlackBerryApiLevelConfiguration*>(
            m_ui->apiLevelCombo->itemData(currentIndex).value<void*>());
}

void BlackBerryNDKSettingsWidget::updateInfoTable(QTreeWidgetItem* currentItem)
{
    updateUi(currentItem);
    if (!currentItem)
        return;

    if (currentItem->parent() == m_runtimes) {
        BlackBerryRuntimeConfiguration *runtime = static_cast<BlackBerryRuntimeConfiguration*>(
                    currentItem->data(0, Qt::UserRole).value<void*>());
        if (runtime) {
            m_ui->baseName->setText(runtime->displayName());
            m_ui->version->setText(runtime->version().toString());
            m_ui->path->setText(runtime->path());

            m_ui->removeConfigButton->setEnabled(runtime);
            m_ui->activateNdkTargetButton->setEnabled(false);
            m_ui->deactivateNdkTargetButton->setEnabled(false);
        }

        return;
    } else if (currentItem->parent() == m_autoDetectedNdks || currentItem->parent() == m_manualApiLevel) {
        BlackBerryApiLevelConfiguration *config = static_cast<BlackBerryApiLevelConfiguration*>(
                    currentItem->data(0, Qt::UserRole).value<void*>());

        m_ui->path->setText(config->envFile().toString());
        m_ui->baseName->setText(config->displayName());
        m_ui->host->setText(QDir::toNativeSeparators(config->qnxHost().toString()));
        m_ui->target->setText(QDir::toNativeSeparators(config->sysRoot().toString()));
        m_ui->version->setText(config->version().toString());
    }
}

void BlackBerryNDKSettingsWidget::updateConfigurationList()
{
    m_activatedApiLevel.clear();
    m_activatedApiLevel << m_bbConfigManager->activeApiLevels();
    m_deactivatedApiLevel.clear();

    qDeleteAll(m_autoDetectedNdks->takeChildren());
    qDeleteAll(m_manualApiLevel->takeChildren());
    qDeleteAll(m_runtimes->takeChildren());

    bool enableCleanUp = false;
    foreach (BlackBerryApiLevelConfiguration *config, m_bbConfigManager->apiLevels()) {
        QTreeWidgetItem *parent = config->isAutoDetected() ? m_autoDetectedNdks : m_manualApiLevel;
        QTreeWidgetItem *item = new QTreeWidgetItem(parent);
        item->setText(0, config->displayName());
        item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(config)));
        QFont font;
        font.setBold(config->isActive() || m_activatedApiLevel.contains(config));
        item->setFont(0, font);
        item->setIcon(0, config->isValid() ? QIcon() : invalidConfigIcon);
        // TODO: Do the same if qmake, qcc, debugger are no longer detected...
        if (!config->isValid()) {
            QString toolTip = tr("Invalid target %1:").arg(config->targetName());
            if (config->isAutoDetected() && !config->autoDetectionSource().toFileInfo().exists())
                toolTip += QLatin1Char('\n') + tr("- Target no longer installed.");

            if (!config->envFile().toFileInfo().exists())
                toolTip += QLatin1Char('\n') + tr("- No NDK environment file found.");

            if (config->qmake4BinaryFile().isEmpty()
                    && config->qmake5BinaryFile().isEmpty())
                toolTip += QLatin1Char('\n') + tr("- No Qt version found.");

            if (config->qccCompilerPath().isEmpty())
                toolTip += QLatin1Char('\n') + tr("- No compiler found.");

            if (config->armDebuggerPath().isEmpty())
                toolTip += QLatin1Char('\n') + tr("- No debugger found for device.");

            if (config->x86DebuggerPath().isEmpty())
                toolTip += QLatin1Char('\n') + tr("- No debugger found for simulator.");

            item->setToolTip(0, toolTip);
            enableCleanUp = true;
        }
    }

    foreach (BlackBerryRuntimeConfiguration *runtime, m_bbConfigManager->runtimes()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_runtimes);
        item->setText(0, runtime->displayName());
        item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(runtime)));
    }

    m_ui->ndksTreeWidget->setCurrentItem(m_autoDetectedNdks->child(0));
    m_ui->cleanUpButton->setEnabled(enableCleanUp);
}

void BlackBerryNDKSettingsWidget::addConfiguration()
{
    launchBlackBerryInstallerWizard(BlackBerryInstallerDataHandler::InstallMode, BlackBerryInstallerDataHandler::ApiLevel);
    emit configurationsUpdated();
}

void BlackBerryNDKSettingsWidget::removeConfiguration()
{
    QTreeWidgetItem * current = m_ui->ndksTreeWidget->currentItem();
    if (!current)
        return;

    if (current->parent() == m_runtimes) {
        uninstallConfiguration(BlackBerryInstallerDataHandler::Runtime);
        emit configurationsUpdated();
    } else {
        const QString ndk = m_ui->ndksTreeWidget->currentItem()->text(0);
        BlackBerryApiLevelConfiguration *config = static_cast<BlackBerryApiLevelConfiguration*>(
                    current->data(0, Qt::UserRole).value<void*>());
        if (config->isAutoDetected()) {
            uninstallConfiguration(BlackBerryInstallerDataHandler::ApiLevel);
            emit configurationsUpdated();
            return;
        }

        QMessageBox::StandardButton button =
                QMessageBox::question(Core::ICore::mainWindow(),
                                      tr("Clean BlackBerry 10 Configuration"),
                                      tr("Are you sure you want to remove:\n %1?").arg(ndk),
                                      QMessageBox::Yes | QMessageBox::No);

        if (button == QMessageBox::Yes) {
            m_activatedApiLevel.removeOne(config);
            m_deactivatedApiLevel.removeOne(config);
            m_bbConfigManager->removeApiLevel(config);
            m_manualApiLevel->removeChild(m_ui->ndksTreeWidget->currentItem());
            emit configurationsUpdated();
        }
    }
}

void BlackBerryNDKSettingsWidget::activateApiLevel()
{
    if (!m_ui->ndksTreeWidget->currentItem())
        return;

    BlackBerryApiLevelConfiguration *config = static_cast<BlackBerryApiLevelConfiguration*>(
                m_ui->ndksTreeWidget->currentItem()->data(0, Qt::UserRole).value<void*>());

    if (!m_activatedApiLevel.contains(config)) {
        m_activatedApiLevel << config;
        if (m_deactivatedApiLevel.contains(config))
           m_deactivatedApiLevel.removeAt(m_deactivatedApiLevel.indexOf(config));

        updateUi(m_ui->ndksTreeWidget->currentItem());
        emit configurationsUpdated();
    }
}

void BlackBerryNDKSettingsWidget::deactivateApiLevel()
{
    if (!m_ui->ndksTreeWidget->currentItem())
        return;

    BlackBerryApiLevelConfiguration *config = static_cast<BlackBerryApiLevelConfiguration*>(
                m_ui->ndksTreeWidget->currentItem()->data(0, Qt::UserRole).value<void*>());
    if (m_activatedApiLevel.contains(config)) {
        m_deactivatedApiLevel << config;
        m_activatedApiLevel.removeAt(m_activatedApiLevel.indexOf(config));
        updateUi(m_ui->ndksTreeWidget->currentItem());
        emit configurationsUpdated();
    }
}

void BlackBerryNDKSettingsWidget::updateUi(QTreeWidgetItem *item)
{
    if (!item || (item->parent() != m_runtimes &&
                  item->parent() != m_autoDetectedNdks &&
                  item->parent() != m_manualApiLevel )) {
        m_ui->removeConfigButton->setEnabled(false);
        m_ui->activateNdkTargetButton->setEnabled(false);
        m_ui->deactivateNdkTargetButton->setEnabled(false);
       m_ui->informationBox->setVisible(false);
        return;
    }

    const bool isRuntimeItem = item->parent() == m_runtimes;
    // Update the infornation to show in the information panel
    m_ui->informationBox->setVisible(true);
    m_ui->informationBox->setTitle(isRuntimeItem ?
                                       tr("Runtime Information") : tr("API Level Information"));
    m_ui->pathLabel->setText(isRuntimeItem ? tr("Path:") : tr("Environment file:"));
    m_ui->hostLabel->setVisible(!isRuntimeItem);
    m_ui->host->setVisible(!isRuntimeItem);
    m_ui->targetLabel->setVisible(!isRuntimeItem);
    m_ui->target->setVisible(!isRuntimeItem);

    if (!isRuntimeItem) {
        BlackBerryApiLevelConfiguration *config = static_cast<BlackBerryApiLevelConfiguration*>(
                    item->data(0, Qt::UserRole).value<void*>());
        const bool contains = m_activatedApiLevel.contains(config);
        QFont font;
        font.setBold(contains);
        item->setFont(0, font);

        m_ui->activateNdkTargetButton->setEnabled(!contains);
        m_ui->deactivateNdkTargetButton->setEnabled(contains);
        // Disable remove button for auto detected pre-10.2 NDKs (uninstall wizard doesn't handle them)
        m_ui->removeConfigButton->setEnabled(!(config->isAutoDetected()
                                               && QnxUtils::sdkInstallerPath(config->ndkPath()).isEmpty()));
    }
}

void BlackBerryNDKSettingsWidget::uninstallConfiguration(BlackBerryInstallerDataHandler::Target target)
{
    const QMessageBox::StandardButton answer = QMessageBox::question(this, tr("Confirmation"),
                                                                     tr("Are you sure you want to uninstall %1?").
                                                                     arg(m_ui->baseName->text()),
                                                                     QMessageBox::Yes | QMessageBox::No);

    if (answer == QMessageBox::Yes) {
        if (target == BlackBerryInstallerDataHandler::ApiLevel) {
            launchBlackBerryInstallerWizard(BlackBerryInstallerDataHandler::UninstallMode,
                                        BlackBerryInstallerDataHandler::ApiLevel, m_ui->version->text());
        } else if (target == BlackBerryInstallerDataHandler::Runtime) {
            if (m_ui->ndksTreeWidget->currentItem()) {
                launchBlackBerryInstallerWizard(BlackBerryInstallerDataHandler::UninstallMode,
                                                BlackBerryInstallerDataHandler::Runtime,
                                                m_ui->ndksTreeWidget->currentItem()->text(0));
            }
        }

    }
}

void BlackBerryNDKSettingsWidget::cleanUp()
{
    foreach (BlackBerryApiLevelConfiguration *config, m_bbConfigManager->apiLevels()) {
        if (!config->isValid()) {
            m_activatedApiLevel.removeOne(config);
            m_deactivatedApiLevel.removeOne(config);
            m_bbConfigManager->removeApiLevel(config);
        }
    }

    updateConfigurationList();
}

void BlackBerryNDKSettingsWidget::handleInstallationFinished()
{
    m_bbConfigManager->loadAutoDetectedConfigurations(
            BlackBerryConfigurationManager::ApiLevel | BlackBerryConfigurationManager::Runtime);
}

void BlackBerryNDKSettingsWidget::handleUninstallationFinished()
{
    QTreeWidgetItem *current = m_ui->ndksTreeWidget->currentItem();
    if (!current)
        return;

    if (current->parent() == m_runtimes) {
        BlackBerryRuntimeConfiguration *runtime = static_cast<BlackBerryRuntimeConfiguration*>(
                    current->data(0, Qt::UserRole).value<void*>());
        m_bbConfigManager->removeRuntime(runtime);
        updateConfigurationList();
        return;
    }

    const QString targetName = current->text(0);
    // Check if the target is corrrecly uninstalled
    foreach (const ConfigInstallInformation &ndk, QnxUtils::installedConfigs()) {
        if (ndk.name == targetName)
            return;
    }

    BlackBerryApiLevelConfiguration *config = static_cast<BlackBerryApiLevelConfiguration*>(
                current->data(0, Qt::UserRole).value<void*>());
    if (m_activatedApiLevel.contains(config))
        m_activatedApiLevel.removeAt(m_activatedApiLevel.indexOf(config));
    else if (m_deactivatedApiLevel.contains(config))
        m_deactivatedApiLevel.removeAt(m_deactivatedApiLevel.indexOf(config));

    m_bbConfigManager->removeApiLevel(config);

    updateConfigurationList();
}

void BlackBerryNDKSettingsWidget::populateDefaultConfigurationCombo()
{
    // prevent QComboBox::currentIndexChanged() from being emitted
    m_ui->apiLevelCombo->clear();

    QList<BlackBerryApiLevelConfiguration*> configurations = m_bbConfigManager->apiLevels();

    m_ui->apiLevelCombo->addItem(tr("Newest Version"),
            QVariant::fromValue(static_cast<void*>(0)));

    if (configurations.isEmpty())
        return;

    int configIndex = 0;

    BlackBerryApiLevelConfiguration *defaultConfig = m_bbConfigManager->defaultApiLevel();

    foreach (BlackBerryApiLevelConfiguration *config, configurations) {
        m_ui->apiLevelCombo->addItem(config->displayName(),
                QVariant::fromValue(static_cast<void*>(config)));

        if (config == defaultConfig)
            configIndex = m_ui->apiLevelCombo->count() - 1;
    }

    const int currentIndex = (m_bbConfigManager->newestApiLevelEnabled()) ? 0 : configIndex;

    m_ui->apiLevelCombo->setCurrentIndex(currentIndex);
}

void BlackBerryNDKSettingsWidget::launchBlackBerryInstallerWizard(
        BlackBerryInstallerDataHandler::Mode mode,
        BlackBerryInstallerDataHandler::Target target,
        const QString& targetVersion)
{
    BlackBerryInstallWizard wizard(mode, target, targetVersion, this);
    if (mode == BlackBerryInstallerDataHandler::InstallMode)
        connect(&wizard, SIGNAL(processFinished()), this, SLOT(handleInstallationFinished()));
    else
        connect(&wizard, SIGNAL(processFinished()), this, SLOT(handleUninstallationFinished()));

    wizard.exec();
}

void BlackBerryNDKSettingsWidget::updatePage()
{
    updateConfigurationList();
    populateDefaultConfigurationCombo();
}

} // namespace Internal
} // namespace Qnx

