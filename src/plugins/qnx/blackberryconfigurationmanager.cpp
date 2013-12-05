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

#include <debugger/debuggerkitinformation.h>

#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>

using namespace ProjectExplorer;

namespace Qnx {
namespace Internal {

namespace {
const QLatin1String SettingsGroup("BlackBerryConfiguration");
const QLatin1String NDKLocationKey("NDKLocation"); // For 10.1 NDK support (< QTC 3.0)
const QLatin1String NDKEnvFileKey("NDKEnvFile");
const QLatin1String ManualNDKsGroup("ManualNDKs");
const QLatin1String ActiveNDKsGroup("ActiveNDKs");
const QLatin1String DefaultApiLevelKey("DefaultApiLevel");
const QLatin1String BBConfigsFileVersionKey("Version");
const QLatin1String BBConfigDataKey("BBConfiguration.");
const QLatin1String BBConfigCountKey("BBConfiguration.Count");
}

static Utils::FileName bbConfigSettingsFileName()
{
    return Utils::FileName::fromString(Core::ICore::userResourcePath() + QLatin1String("/qnx/")
                                + QLatin1String(Constants::QNX_BLACKBERRY_CONFIGS_FILENAME));
}

static bool sortConfigurationsByVersion(const BlackBerryConfiguration *a, const BlackBerryConfiguration *b)
{
    return a->version() > b->version();
}

BlackBerryConfigurationManager::BlackBerryConfigurationManager(QObject *parent)
    : QObject(parent),
    m_defaultApiLevel(0)
{
    m_writer = new Utils::PersistentSettingsWriter(bbConfigSettingsFileName(),
                                                   QLatin1String("BlackBerryConfigurations"));
    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()), this, SLOT(saveSettings()));
}

void BlackBerryConfigurationManager::saveConfigurations()
{
    QTC_ASSERT(m_writer, return);
    QVariantMap data;
    data.insert(QLatin1String(BBConfigsFileVersionKey), 1);
    int count = 0;
    foreach (BlackBerryConfiguration *config, m_configs) {
        QVariantMap tmp = config->toMap();
        if (tmp.isEmpty())
            continue;

        data.insert(BBConfigDataKey + QString::number(count), tmp);
        ++count;
    }

    data.insert(QLatin1String(BBConfigCountKey), count);
    m_writer->save(data, Core::ICore::mainWindow());
}

void BlackBerryConfigurationManager::restoreConfigurations()
{
    Utils::PersistentSettingsReader reader;
    if (!reader.load(bbConfigSettingsFileName()))
        return;

    QVariantMap data = reader.restoreValues();
    int count = data.value(BBConfigCountKey, 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = BBConfigDataKey + QString::number(i);
        if (!data.contains(key))
            continue;

        const QVariantMap dMap = data.value(key).toMap();
        insertByVersion(new BlackBerryConfiguration(dMap));
    }
}

// Backward compatibility: Read existing entries in the ManualNDKsGroup
// and then remove the group since not used.
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

        BlackBerryConfiguration *config = new BlackBerryConfiguration(Utils::FileName::fromString(ndkEnvPath));
        if (!addConfiguration(config))
            delete config;

        settings->endGroup();
    }

    settings->endGroup();
    settings->remove(ManualNDKsGroup);
    settings->endGroup();
}

void BlackBerryConfigurationManager::loadDefaultApiLevel()
{
    if (m_configs.isEmpty())
        return;

    QSettings *settings = Core::ICore::settings();

    settings->beginGroup(SettingsGroup);

    const QString ndkEnvFile = settings->value(DefaultApiLevelKey).toString();

    BlackBerryConfiguration *defaultApiLevel = 0;

    // now check whether there is a cached value available to override it
    foreach (BlackBerryConfiguration *config, m_configs) {
        if (config->ndkEnvFile().toString() == ndkEnvFile) {
            defaultApiLevel = config;
            break;
        }
    }

    if (defaultApiLevel)
        setDefaultApiLevel(defaultApiLevel);
    else
        setDefaultApiLevel(m_configs.first());


    settings->endGroup();
}

void BlackBerryConfigurationManager::loadAutoDetectedConfigurations()
{
    foreach (const NdkInstallInformation &ndkInfo, QnxUtils::installedNdks()) {
        BlackBerryConfiguration *config = new BlackBerryConfiguration(ndkInfo);
        if (!addConfiguration(config)) {
            delete config;
            continue;
        }
    }
}

void BlackBerryConfigurationManager::setDefaultApiLevel(BlackBerryConfiguration *config)
{
    if (config && !m_configs.contains(config)) {
        qWarning() << "BlackBerryConfigurationManager::setDefaultApiLevel -"
                      " configuration does not belong to this instance: "
                   << config->ndkEnvFile().toString();
        return;
    }

    m_defaultApiLevel = config;
}

void BlackBerryConfigurationManager::saveDefaultApiLevel()
{
    if (!m_defaultApiLevel)
        return;

    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    settings->setValue(DefaultApiLevelKey, m_defaultApiLevel->ndkEnvFile().toString());
    settings->endGroup();
}

void BlackBerryConfigurationManager::setKitsAutoDetectionSource()
{
    foreach (Kit *kit, KitManager::kits()) {
        if (kit->isAutoDetected() &&
                (DeviceTypeKitInformation::deviceTypeId(kit) ==  Constants::QNX_BB_CATEGORY_ICON) &&
                kit->autoDetectionSource().isEmpty()) {
            QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
            foreach (BlackBerryConfiguration *config, m_configs) {
                if ((version &&
                     (version->qmakeCommand() == config->qmake4BinaryFile() || version->qmakeCommand() == config->qmake5BinaryFile()))
                        && (SysRootKitInformation::sysRoot(kit) == config->sysRoot()))
                    kit->setAutoDetectionSource(config->ndkEnvFile().toString());
            }
        }
    }
}

void BlackBerryConfigurationManager::insertByVersion(BlackBerryConfiguration *config)
{
    QList<BlackBerryConfiguration *>::iterator it = qLowerBound(m_configs.begin(), m_configs.end(),
                                                                config, &sortConfigurationsByVersion);
    m_configs.insert(it, config);
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
        if (config->ndkEnvFile() == c->ndkEnvFile()) {
            if (!config->isAutoDetected())
                QMessageBox::warning(Core::ICore::mainWindow(), tr("NDK Already Known"),
                                 tr("The NDK already has a configuration."), QMessageBox::Ok);
            return false;
        }
    }

    if (config->isValid()) {
        insertByVersion(config);
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

    m_configs.removeAt(m_configs.indexOf(config));

    if (m_defaultApiLevel == config) {
        if (m_configs.isEmpty())
            setDefaultApiLevel(0);
        else
            setDefaultApiLevel(m_configs.first());

        saveDefaultApiLevel();
    }

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
    if (envFile.isEmpty())
        return 0;

    foreach (BlackBerryConfiguration *config, m_configs) {
        if (config->ndkEnvFile() == envFile)
            return config;
    }

    return 0;
}

BlackBerryConfiguration *BlackBerryConfigurationManager::defaultApiLevel() const
{
    return m_defaultApiLevel;
}

QList<Utils::EnvironmentItem> BlackBerryConfigurationManager::defaultApiLevelEnv() const
{
    if (!m_defaultApiLevel)
        return QList<Utils::EnvironmentItem>();

    return m_defaultApiLevel->qnxEnv();
}

void BlackBerryConfigurationManager::loadSettings()
{
    // Backward compatibility: Set kit's auto detection source
    // for existing BlackBerry kits that do not have it set yet.
    setKitsAutoDetectionSource();

    restoreConfigurations();
    // For backward compatibility
    loadManualConfigurations();
    loadAutoDetectedConfigurations();
    loadDefaultApiLevel();
    checkToolChainConfiguration();

    // If no target was/is activated, activate one since it's needed by
    // device connection and CSK code.
    if (activeConfigurations().isEmpty() && !m_configs.isEmpty())
        m_configs.first()->activate();

    emit settingsLoaded();
}

void BlackBerryConfigurationManager::saveSettings()
{
    saveConfigurations();
    saveDefaultApiLevel();
}

BlackBerryConfigurationManager &BlackBerryConfigurationManager::instance()
{
    static BlackBerryConfigurationManager instance;

    return instance;
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

} // namespace Internal
} // namespace Qnx
