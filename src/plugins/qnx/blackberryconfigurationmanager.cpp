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
#include "blackberryapilevelconfiguration.h"
#include "blackberryruntimeconfiguration.h"

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
#include <qmakeprojectmanager/qmakekitinformation.h>

#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>
#include <QDir>

using namespace ProjectExplorer;

namespace Qnx {
namespace Internal {

BlackBerryConfigurationManager *BlackBerryConfigurationManager::m_instance = 0;

namespace {
const QLatin1String SettingsGroup("BlackBerryConfiguration");
const QLatin1String NDKLocationKey("NDKLocation"); // For 10.1 NDK support (< QTC 3.0)
const QLatin1String NDKEnvFileKey("NDKEnvFile");
const QLatin1String ManualNDKsGroup("ManualNDKs");
const QLatin1String ActiveNDKsGroup("ActiveNDKs");
const QLatin1String DefaultConfigurationKey("DefaultConfiguration");
const QLatin1String NewestConfigurationValue("Newest");
const QLatin1String BBConfigsFileVersionKey("Version");
const QLatin1String BBConfigDataKey("BBConfiguration.");
const QLatin1String BBConfigCountKey("BBConfiguration.Count");
}

static Utils::FileName bbConfigSettingsFileName()
{
    return Utils::FileName::fromString(Core::ICore::userResourcePath() + QLatin1String("/qnx/")
                                + QLatin1String(Constants::QNX_BLACKBERRY_CONFIGS_FILENAME));
}

template <class T> static bool sortConfigurationsByVersion(const T *a, const T *b)
{
    return a->version() > b->version();
}

BlackBerryConfigurationManager::BlackBerryConfigurationManager(QObject *parent)
    : QObject(parent),
    m_defaultConfiguration(0)
{
    m_writer = new Utils::PersistentSettingsWriter(bbConfigSettingsFileName(),
                                                   QLatin1String("BlackBerryConfigurations"));
    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()), this, SLOT(saveSettings()));
    m_instance = this;
}

void BlackBerryConfigurationManager::saveConfigurations()
{
    QTC_ASSERT(m_writer, return);
    QVariantMap data;
    data.insert(QLatin1String(BBConfigsFileVersionKey), 1);
    int count = 0;
    foreach (BlackBerryApiLevelConfiguration *apiLevel, m_apiLevels) {
        QVariantMap tmp = apiLevel->toMap();
        if (tmp.isEmpty())
            continue;

        data.insert(BBConfigDataKey + QString::number(count), tmp);
        ++count;
    }

    foreach (BlackBerryRuntimeConfiguration *runtime, m_runtimes) {
        QVariantMap tmp = runtime->toMap();
        if (tmp.isEmpty())
            continue;

        data.insert(BBConfigDataKey + QString::number(count), tmp);
        ++count;
    }

    data.insert(QLatin1String(BBConfigCountKey), count);

    const QString newestConfig = (newestApiLevelEnabled())
        ? NewestConfigurationValue : defaultApiLevel()->envFile().toString();

    //save default configuration
    data.insert(QLatin1String(DefaultConfigurationKey), newestConfig);

    m_writer->save(data, Core::ICore::mainWindow());
}

void BlackBerryConfigurationManager::restoreConfigurations()
{
    Utils::PersistentSettingsReader reader;
    if (!reader.load(bbConfigSettingsFileName()))
        return;

    QVariantMap data = reader.restoreValues();

    // load default configuration
    const QString ndkEnvFile = data.value(DefaultConfigurationKey).toString();

    // use newest API level
    const bool useNewestConfiguration = (ndkEnvFile == NewestConfigurationValue);

    int count = data.value(BBConfigCountKey, 0).toInt();

    for (int i = 0; i < count; ++i) {
        const QString key = BBConfigDataKey + QString::number(i);

        if (!data.contains(key))
            continue;

        const QVariantMap dMap = data.value(key).toMap();
        const QString configurationType =
                dMap.value(QLatin1String(Constants::QNX_BB_KEY_CONFIGURATION_TYPE)).toString();
        if (configurationType == QLatin1String(Constants::QNX_BB_RUNTIME_TYPE)) {
            BlackBerryRuntimeConfiguration *runtime = new BlackBerryRuntimeConfiguration(dMap);
            insertRuntimeByVersion(runtime);
        } else if (configurationType == QLatin1String(Constants::QNX_BB_APILEVEL_TYPE)
                   || configurationType.isEmpty()) { // Backward compatibility
            BlackBerryApiLevelConfiguration *apiLevel = new BlackBerryApiLevelConfiguration(dMap);
            insertApiLevelByVersion(apiLevel);

            if (!useNewestConfiguration && (apiLevel->envFile().toString() == ndkEnvFile))
                setDefaultConfiguration(apiLevel);
        }
    }

    emit settingsChanged();
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

        BlackBerryApiLevelConfiguration *config =
                new BlackBerryApiLevelConfiguration(Utils::FileName::fromString(ndkEnvPath));
        if (!addApiLevel(config))
            delete config;

        settings->endGroup();
    }

    settings->endGroup();
    settings->remove(ManualNDKsGroup);
    settings->endGroup();
}

void BlackBerryConfigurationManager::loadAutoDetectedApiLevels()
{
    foreach (const ConfigInstallInformation &ndkInfo, QnxUtils::installedConfigs()) {
        BlackBerryApiLevelConfiguration *config = new BlackBerryApiLevelConfiguration(ndkInfo);
        if (!addApiLevel(config)) {
            delete config;
        }
    }
}

void BlackBerryConfigurationManager::loadAutoDetectedRuntimes()
{
    QRegExp regExp(QLatin1String("runtime_(\\d+)_(\\d+)_(\\d+)_(\\d+)"));
    foreach (BlackBerryApiLevelConfiguration *apiLevel, m_apiLevels) {
        QDir ndkDir(apiLevel->ndkPath());
        foreach (const QFileInfo& fi, ndkDir.entryInfoList(QDir::Dirs)) {
            if (regExp.exactMatch(fi.baseName())) {
                BlackBerryRuntimeConfiguration *runtime =
                        new BlackBerryRuntimeConfiguration(fi.absoluteFilePath());
                if (!addRuntime(runtime))
                    delete runtime;
            }
        }
    }
}

void BlackBerryConfigurationManager::setDefaultConfiguration(
        BlackBerryApiLevelConfiguration *config)
{
    if (config && !m_apiLevels.contains(config)) {
        qWarning() << "BlackBerryConfigurationManager::setDefaultConfiguration -"
                      " configuration does not belong to this instance: "
                   << config->envFile().toString();
        return;
    }

    m_defaultConfiguration = config;
    emit settingsChanged();
}

bool BlackBerryConfigurationManager::newestApiLevelEnabled() const
{
    return !m_defaultConfiguration;
}

void BlackBerryConfigurationManager::emitSettingsChanged()
{
    emit settingsChanged();
}

#ifdef WITH_TESTS
void BlackBerryConfigurationManager::initUnitTest()
{
    foreach (BlackBerryApiLevelConfiguration *apiLevel, m_apiLevels)
        removeApiLevel(apiLevel);

    foreach (BlackBerryRuntimeConfiguration *runtime, m_runtimes)
        removeRuntime(runtime);

    m_defaultConfiguration = 0;
}
#endif

void BlackBerryConfigurationManager::setKitsAutoDetectionSource()
{
    foreach (Kit *kit, KitManager::kits()) {
        if (kit->isAutoDetected() &&
                (DeviceTypeKitInformation::deviceTypeId(kit) ==  Constants::QNX_BB_OS_TYPE) &&
                kit->autoDetectionSource().isEmpty()) {
            QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
            foreach (BlackBerryApiLevelConfiguration *config, m_apiLevels) {
                if ((version &&
                     (version->qmakeCommand() == config->qmake4BinaryFile() || version->qmakeCommand() == config->qmake5BinaryFile()))
                        && (SysRootKitInformation::sysRoot(kit) == config->sysRoot())) {
                    kit->setAutoDetectionSource(config->envFile().toString());
                    // Set stickyness since not necessary saved for those kits
                    kit->setSticky(QtSupport::QtKitInformation::id(), true);
                    kit->setSticky(ToolChainKitInformation::id(), true);
                    kit->setSticky(DeviceTypeKitInformation::id(), true);
                    kit->setSticky(SysRootKitInformation::id(), true);
                    kit->setSticky(Debugger::DebuggerKitInformation::id(), true);
                    kit->setSticky(QmakeProjectManager::QmakeKitInformation::id(), true);
                }
            }
        }
    }
}

void BlackBerryConfigurationManager::insertApiLevelByVersion(
        BlackBerryApiLevelConfiguration *apiLevel)
{
    QList<BlackBerryApiLevelConfiguration *>::iterator it =
            qLowerBound(m_apiLevels.begin(), m_apiLevels.end(),
                        apiLevel, sortConfigurationsByVersion<BlackBerryApiLevelConfiguration>);
    m_apiLevels.insert(it, apiLevel);
}

void BlackBerryConfigurationManager::insertRuntimeByVersion(
        BlackBerryRuntimeConfiguration *runtime)
{
    QList<BlackBerryRuntimeConfiguration *>::iterator it =
            qLowerBound(m_runtimes.begin(), m_runtimes.end(),
                        runtime, sortConfigurationsByVersion<BlackBerryRuntimeConfiguration>);
    m_runtimes.insert(it, runtime);
}

// Switch to QnxToolchain for exisintg configuration using GccToolChain
void BlackBerryConfigurationManager::checkToolChainConfiguration()
{
    foreach (BlackBerryApiLevelConfiguration *config, m_apiLevels) {
        foreach (ToolChain *tc, ToolChainManager::toolChains()) {
            if (tc->compilerCommand() == config->qccCompilerPath()
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

bool BlackBerryConfigurationManager::addApiLevel(BlackBerryApiLevelConfiguration *config)
{
    foreach (BlackBerryApiLevelConfiguration *c, m_apiLevels) {
        if (config->envFile() == c->envFile()) {
            if (!config->isAutoDetected())
                QMessageBox::warning(Core::ICore::mainWindow(), tr("NDK Already Known"),
                                 tr("The NDK already has a configuration."), QMessageBox::Ok);
            return false;
        }
    }

    if (config->isValid()) {
        insertApiLevelByVersion(config);
        emit settingsChanged();
        return true;
    }

    return false;
}

void BlackBerryConfigurationManager::removeApiLevel(BlackBerryApiLevelConfiguration *config)
{
    if (!config)
        return;

    if (config->isActive())
        config->deactivate();

    m_apiLevels.removeAll(config);

    if (defaultApiLevel() == config)
        setDefaultConfiguration(0);

    delete config;

    emit settingsChanged();
}

bool BlackBerryConfigurationManager::addRuntime(BlackBerryRuntimeConfiguration *runtime)
{
    foreach (BlackBerryRuntimeConfiguration *rt, m_runtimes) {
        if (runtime->path() == rt->path())
            return false;
    }

    insertRuntimeByVersion(runtime);
    return true;
}

void BlackBerryConfigurationManager::removeRuntime(BlackBerryRuntimeConfiguration *runtime)
{
    if (!runtime)
        return;

    m_runtimes.removeAll(runtime);
    delete runtime;
}

QList<BlackBerryApiLevelConfiguration *> BlackBerryConfigurationManager::apiLevels() const
{
    return m_apiLevels;
}

QList<BlackBerryRuntimeConfiguration *> BlackBerryConfigurationManager::runtimes() const
{
    return m_runtimes;
}

QList<BlackBerryApiLevelConfiguration *> BlackBerryConfigurationManager::manualApiLevels() const
{
    QList<BlackBerryApiLevelConfiguration*> manuals;
    foreach (BlackBerryApiLevelConfiguration *config, m_apiLevels) {
        if (!config->isAutoDetected())
            manuals << config;
    }

    return manuals;
}

QList<BlackBerryApiLevelConfiguration *> BlackBerryConfigurationManager::activeApiLevels() const
{
    QList<BlackBerryApiLevelConfiguration*> actives;
    foreach (BlackBerryApiLevelConfiguration *config, m_apiLevels) {
        if (config->isActive())
            actives << config;
    }

    return actives;
}

BlackBerryApiLevelConfiguration *BlackBerryConfigurationManager::apiLevelFromEnvFile(
        const Utils::FileName &envFile) const
{
    if (envFile.isEmpty())
        return 0;

    foreach (BlackBerryApiLevelConfiguration *config, m_apiLevels) {
        if (config->envFile() == envFile)
            return config;
    }

    return 0;
}

BlackBerryRuntimeConfiguration *BlackBerryConfigurationManager::runtimeFromFilePath(
        const QString &path)
{
    foreach (BlackBerryRuntimeConfiguration *runtime, m_runtimes) {
        if (runtime->path() == path)
            return runtime;
    }

    return 0;
}

BlackBerryApiLevelConfiguration *BlackBerryConfigurationManager::defaultApiLevel() const
{
    if (m_apiLevels.isEmpty())
        return 0;

    // !m_defaultConfiguration means use newest configuration
    if (!m_defaultConfiguration)
        return m_apiLevels.first();

    return m_defaultConfiguration;
}

QList<Utils::EnvironmentItem> BlackBerryConfigurationManager::defaultConfigurationEnv() const
{
    const BlackBerryApiLevelConfiguration *config = defaultApiLevel();

    if (config)
        return config->qnxEnv();

    return QList<Utils::EnvironmentItem>();
}

void BlackBerryConfigurationManager::loadAutoDetectedConfigurations(QFlags<ConfigurationType> types)
{
    if (types.testFlag(ApiLevel))
        loadAutoDetectedApiLevels();
    if (types.testFlag(Runtime))
        loadAutoDetectedRuntimes();
    emit settingsChanged();
}

void BlackBerryConfigurationManager::loadSettings()
{
    restoreConfigurations();
    // For backward compatibility
    loadManualConfigurations();
    loadAutoDetectedApiLevels();
    loadAutoDetectedRuntimes();
    checkToolChainConfiguration();

    // Backward compatibility: Set kit's auto detection source
    // for existing BlackBerry kits that do not have it set yet.
    setKitsAutoDetectionSource();

    emit settingsLoaded();
    emit settingsChanged();
}

void BlackBerryConfigurationManager::saveSettings()
{
    saveConfigurations();
}

BlackBerryConfigurationManager *BlackBerryConfigurationManager::instance()
{
    return m_instance;
}

BlackBerryConfigurationManager::~BlackBerryConfigurationManager()
{
    m_instance = 0;
    qDeleteAll(m_apiLevels);
    qDeleteAll(m_runtimes);
    delete m_writer;
}

QString BlackBerryConfigurationManager::barsignerCskPath() const
{
    return QnxUtils::bbDataDirPath() + QLatin1String("/barsigner.csk");
}

QString BlackBerryConfigurationManager::idTokenPath() const
{
    return QnxUtils::bbDataDirPath() + QLatin1String("/bbidtoken.csk");
}

QString BlackBerryConfigurationManager::barsignerDbPath() const
{
    return QnxUtils::bbDataDirPath() + QLatin1String("/barsigner.db");
}

QString BlackBerryConfigurationManager::defaultKeystorePath() const
{
    return QnxUtils::bbDataDirPath() + QLatin1String("/author.p12");
}

QString BlackBerryConfigurationManager::defaultDebugTokenPath() const
{
    return QnxUtils::bbDataDirPath() + QLatin1String("/debugtoken.bar");
}

} // namespace Internal
} // namespace Qnx
