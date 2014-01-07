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

#include "blackberryconfigurationmanager.h"
#include "blackberrycertificate.h"
#include "blackberryconfiguration.h"

#include "qnxtoolchain.h"
#include "qnxutils.h"

#include <coreplugin/icore.h>

#include <utils/persistentsettings.h>
#include <utils/hostosinfo.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>

#include <QMessageBox>
#include <QFileInfo>

using namespace ProjectExplorer;

namespace Qnx {
namespace Internal {

namespace {
const QLatin1String SettingsGroup("BlackBerryConfiguration");
const QLatin1String NDKLocationKey("NDKLocation"); // For 10.1 NDK support (< QTC 3.0)
const QLatin1String NDKEnvFileKey("NDKEnvFile");
const QLatin1String CertificateGroup("Certificates");
const QLatin1String ManualNDKsGroup("ManualNDKs");
const QLatin1String ActiveNDKsGroup("ActiveNDKs");
}

BlackBerryConfigurationManager::BlackBerryConfigurationManager(QObject *parent)
    :QObject(parent)
{
    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()), this, SLOT(saveSettings()));
}

void BlackBerryConfigurationManager::loadManualConfigurations()
{
    QSettings *settings = Core::ICore::settings();

    settings->beginGroup(SettingsGroup);
    settings->beginGroup(ManualNDKsGroup);

    foreach (const QString &manualNdk, settings->childGroups()) {
        settings->beginGroup(manualNdk);
        QString ndkEnvPath = settings->value(NDKEnvFileKey).toString();
        // For 10.1 NDK support (< QTC 3.0):
        // Since QTC 3.0 BBConfigurations are based on the bbndk-env file
        // to support multiple targets per NDK
        if (ndkEnvPath.isEmpty()) {
            QString ndkPath = settings->value(NDKLocationKey).toString();
            ndkEnvPath = QnxUtils::envFilePath(ndkPath);
        }

        BlackBerryConfiguration *config = new BlackBerryConfiguration(Utils::FileName::fromString(ndkEnvPath),
                                                                      false);
        if (!addConfiguration(config))
            delete config;

        settings->endGroup();
    }

    settings->endGroup();
    settings->endGroup();
}

void BlackBerryConfigurationManager::loadAutoDetectedConfigurations()
{
    QStringList activePaths = activeConfigurationNdkEnvPaths();
    foreach (const NdkInstallInformation &ndkInfo, QnxUtils::installedNdks()) {
        QString envFilePath = QnxUtils::envFilePath(ndkInfo.path, ndkInfo.version);
        BlackBerryConfiguration *config = new BlackBerryConfiguration(Utils::FileName::fromString(envFilePath),
                                                                      true, ndkInfo.name);
        if (!addConfiguration(config)) {
            delete config;
            continue;
        }

        // Activate targets
        foreach (const QString activeNdkEnvPath, activePaths) {
            if (config->ndkEnvFile().toString() == activeNdkEnvPath)
                config->activate();
        }
    }

    // If no target was/is activated, activate one since it's needed by
    // device connection and CSK code.
    if (activeConfigurations().isEmpty() && !m_configs.isEmpty())
        m_configs.first()->activate();
}

QStringList BlackBerryConfigurationManager::activeConfigurationNdkEnvPaths()
{
    QStringList actives;
    QSettings *settings = Core::ICore::settings();

    settings->beginGroup(SettingsGroup);
    settings->beginGroup(ActiveNDKsGroup);

    foreach (const QString &activeNdkEnvPath, settings->childGroups()) {
        settings->beginGroup(activeNdkEnvPath);
        actives.append(settings->value(NDKEnvFileKey).toString());
        settings->endGroup();
    }

    settings->endGroup();
    settings->endGroup();

    return actives;
}

void BlackBerryConfigurationManager::saveManualConfigurations()
{
    if (manualConfigurations().isEmpty())
        return;

    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    settings->beginGroup(ManualNDKsGroup);

    foreach (BlackBerryConfiguration *config, manualConfigurations()) {
        settings->beginGroup(config->displayName());
        settings->setValue(NDKEnvFileKey, config->ndkEnvFile().toString());
        settings->endGroup();
    }

    settings->endGroup();
    settings->endGroup();
}

void BlackBerryConfigurationManager::saveActiveConfigurationNdkEnvPath()
{
    if (activeConfigurations().isEmpty())
        return;

    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    settings->beginGroup(ActiveNDKsGroup);

    settings->remove(QString());

    foreach (BlackBerryConfiguration *config, activeConfigurations()) {
        settings->beginGroup(config->displayName());
        settings->setValue(NDKEnvFileKey, config->ndkEnvFile().toString());
        settings->endGroup();
    }

    settings->endGroup();
    settings->endGroup();
}

// Remove no longer available/valid 'auto detected' BlackBerry kits and qt versions
void BlackBerryConfigurationManager::clearInvalidConfigurations()
{
    // Deregister invalid auto deteted BlackBerry Kits
    foreach (Kit *kit, KitManager::kits()) {
        if (!kit->isAutoDetected())
            continue;

        if (DeviceTypeKitInformation::deviceTypeId(kit) == Constants::QNX_BB_OS_TYPE
                && !kit->isValid())
        KitManager::deregisterKit(kit);
    }

    // Remove invalid auto detected BlackBerry qtVerions
    foreach (QtSupport::BaseQtVersion *qtVersion, QtSupport::QtVersionManager::versions()) {
        if (!qtVersion->isAutodetected())
            continue;

        if (qtVersion->platformName() == QLatin1String(Constants::QNX_BB_PLATFORM_NAME)
                && !qtVersion->isValid())
           QtSupport::QtVersionManager::removeVersion(qtVersion);
    }
}

// Switch to QnxToolchain for exisintg configuration using GccToolChain
void BlackBerryConfigurationManager::checkToolChainConfiguration()
{
    foreach (BlackBerryConfiguration *config, m_configs) {
        foreach (ToolChain *tc, ToolChainManager::toolChains()) {
            if (tc->compilerCommand() == config->gccCompiler()
                    && !tc->id().startsWith(QLatin1String(Constants::QNX_TOOLCHAIN_ID))) {
                if (config->isActive()) {
                    // reset
                    config->deactivate();
                    config->activate();
                    break;
                }
            }
        }
    }
}

bool BlackBerryConfigurationManager::addConfiguration(BlackBerryConfiguration *config)
{
    foreach (BlackBerryConfiguration *c, m_configs) {
        if (c->ndkPath() == config->ndkPath()
                && c->targetName() == config->targetName()) {
            if (!config->isAutoDetected())
                QMessageBox::warning(0, tr("NDK Already Known"),
                                 tr("The NDK already has a configuration."), QMessageBox::Ok);
            return false;
        }
    }

    if (config->isValid()) {
        m_configs.append(config);
        return true;
    }

    return false;
}

void BlackBerryConfigurationManager::removeConfiguration(BlackBerryConfiguration *config)
{
    if (!config)
        return;

    if (config->isActive())
        config->deactivate();

    clearConfigurationSettings(config);

    m_configs.removeAt(m_configs.indexOf(config));
    delete config;
}

QList<BlackBerryConfiguration *> BlackBerryConfigurationManager::configurations() const
{
    return m_configs;
}

QList<BlackBerryConfiguration *> BlackBerryConfigurationManager::manualConfigurations() const
{
    QList<BlackBerryConfiguration*> manuals;
    foreach (BlackBerryConfiguration *config, m_configs) {
        if (!config->isAutoDetected())
            manuals << config;
    }

    return manuals;
}

QList<BlackBerryConfiguration *> BlackBerryConfigurationManager::activeConfigurations() const
{
    QList<BlackBerryConfiguration*> actives;
    foreach (BlackBerryConfiguration *config, m_configs) {
        if (config->isActive())
            actives << config;
    }

    return actives;
}

BlackBerryConfiguration *BlackBerryConfigurationManager::configurationFromEnvFile(const Utils::FileName &envFile) const
{
    foreach (BlackBerryConfiguration *config, m_configs) {
        if (config->ndkEnvFile() == envFile)
            return config;
    }

    return 0;
}

// Returns a valid qnxEnv from a valid configuration;
// Needed by other classes to get blackberry process path (keys registration, debug token...)
QList<Utils::EnvironmentItem> BlackBerryConfigurationManager::defaultQnxEnv()
{
    foreach (BlackBerryConfiguration *config, m_configs) {
        if (config->isActive() && config->qnxEnv().size())
            return config->qnxEnv();
    }

    return QList<Utils::EnvironmentItem>();
}

void BlackBerryConfigurationManager::loadSettings()
{
    clearInvalidConfigurations();
    loadAutoDetectedConfigurations();
    loadManualConfigurations();
    checkToolChainConfiguration();

    emit settingsLoaded();
}

void BlackBerryConfigurationManager::clearConfigurationSettings(BlackBerryConfiguration *config)
{
    if (!config)
        return;

    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    settings->beginGroup(ManualNDKsGroup);

    foreach (const QString &manualNdk, settings->childGroups()) {
        if (manualNdk == config->displayName()) {
            settings->remove(manualNdk);
            break;
        }
    }

    settings->endGroup();
    settings->endGroup();
}

void BlackBerryConfigurationManager::saveSettings()
{
    saveActiveConfigurationNdkEnvPath();
    saveManualConfigurations();
}

BlackBerryConfigurationManager &BlackBerryConfigurationManager::instance()
{
    if (m_instance == 0)
        m_instance = new BlackBerryConfigurationManager();

    return *m_instance;
}

BlackBerryConfigurationManager::~BlackBerryConfigurationManager()
{
    qDeleteAll(m_configs);
}

QString BlackBerryConfigurationManager::barsignerCskPath() const
{
    return QnxUtils::dataDirPath() + QLatin1String("/barsigner.csk");
}

QString BlackBerryConfigurationManager::idTokenPath() const
{
    return QnxUtils::dataDirPath() + QLatin1String("/bbidtoken.csk");
}

QString BlackBerryConfigurationManager::barsignerDbPath() const
{
    return QnxUtils::dataDirPath() + QLatin1String("/barsigner.db");
}

QString BlackBerryConfigurationManager::defaultKeystorePath() const
{
    return QnxUtils::dataDirPath() + QLatin1String("/author.p12");
}

QString BlackBerryConfigurationManager::defaultDebugTokenPath() const
{
    return QnxUtils::dataDirPath() + QLatin1String("/debugtoken.bar");
}

BlackBerryConfigurationManager* BlackBerryConfigurationManager::m_instance = 0;

} // namespace Internal
} // namespace Qnx
