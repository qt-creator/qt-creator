// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxsettingspage.h"

#include "qnxqtversion.h"
#include "qnxtoolchain.h"
#include "qnxtr.h"
#include "qnxutils.h"
#include "qnxversionnumber.h"

#include <coreplugin/icore.h>

#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitaspect.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitaspect.h>

#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDomDocument>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;
using namespace Debugger;

namespace Qnx::Internal {

const char QNXEnvFileKey[] = "EnvFile";
const char QNXVersionKey[] = "QNXVersion";
// For backward compatibility
const char SdpEnvFileKey[] = "NDKEnvFile";

const char QNXConfiguration[] = "QNX_CONFIGURATION";
const char QNXTarget[] = "QNX_TARGET";
const char QNXHost[] = "QNX_HOST";

const char QNXConfigDataKey[] = "QNXConfiguration.";
const char QNXConfigCountKey[] = "QNXConfiguration.Count";
const char QNXConfigsFileVersionKey[] = "Version";

static FilePath qnxConfigSettingsFileName()
{
    return Core::ICore::userResourcePath("qnx/qnxconfigurations.xml");
}

class QnxConfiguration
{
public:
    QnxConfiguration() = default;
    explicit QnxConfiguration(const FilePath &envFile) { m_envFile = envFile; }

    void fromMap(const Store &data)
    {
        QString envFilePath = data.value(QNXEnvFileKey).toString();
        if (envFilePath.isEmpty())
            envFilePath = data.value(SdpEnvFileKey).toString();

        m_version = QnxVersionNumber(data.value(QNXVersionKey).toString());
        m_envFile = FilePath::fromString(envFilePath);
    }

    Store toMap() const
    {
        Store data;
        data.insert(QNXEnvFileKey, m_envFile.toString());
        data.insert(QNXVersionKey, m_version.toString());
        return data;
    }

    bool isValid() const
    {
        return !m_qccCompiler.isEmpty() && !m_targets.isEmpty();
    }

    bool isActive() const
    {
        const bool hasToolChain = ToolChainManager::toolChain(Utils::equal(&ToolChain::compilerCommand,
                                                                           m_qccCompiler));
        const bool hasDebugger = Utils::contains(DebuggerItemManager::debuggers(), [this](const DebuggerItem &di) {
            return findTargetByDebuggerPath(di.command());
        });
        return hasToolChain && hasDebugger;
    }

    void activate();
    void deactivate();

    void ensureContents() const;
    void mutableEnsureContents();

    QString architectureNames() const
    {
        return transform(m_targets, &QnxTarget::shortDescription).join(", ");
    }

    EnvironmentItems qnxEnvironmentItems() const;

    QnxQtVersion *qnxQtVersion(const QnxTarget &target) const;

    void createKit(const QnxTarget &target);
    QVariant createDebugger(const QnxTarget &target);
    Toolchains createToolChains(const QnxTarget &target);

    const QnxTarget *findTargetByDebuggerPath(const Utils::FilePath &path) const;

    bool m_hasContents = false;
    QString m_configName;

    FilePath m_envFile;
    FilePath m_qnxConfiguration;
    FilePath m_qnxTarget;
    FilePath m_qnxHost;
    FilePath m_qccCompiler;
    EnvironmentItems m_qnxEnv;
    QnxVersionNumber m_version;

    QList<QnxTarget> m_targets;
};

void QnxConfiguration::activate()
{
    ensureContents();

    if (!isValid()) {
        QStringList errorStrings
                = {Tr::tr("The following errors occurred while activating the QNX configuration:")};
        if (m_qccCompiler.isEmpty())
            errorStrings << Tr::tr("- No GCC compiler found.");
        if (m_targets.isEmpty())
            errorStrings << Tr::tr("- No targets found.");
        const QString msg = errorStrings.join('\n');

        QMessageBox::warning(Core::ICore::dialogParent(), Tr::tr("Cannot Set Up QNX Configuration"),
                             msg, QMessageBox::Ok);
        return;
    }

    for (const QnxTarget &target : std::as_const(m_targets))
        createKit(target);
}

void QnxConfiguration::deactivate()
{
    QTC_ASSERT(isActive(), return);

    const Toolchains toolChainsToRemove =
        ToolChainManager::toolchains(Utils::equal(&ToolChain::compilerCommand, m_qccCompiler));

    QList<DebuggerItem> debuggersToRemove;
    const QList<DebuggerItem> debuggerItems = DebuggerItemManager::debuggers();
    for (const DebuggerItem &debuggerItem : debuggerItems) {
        if (findTargetByDebuggerPath(debuggerItem.command()))
            debuggersToRemove.append(debuggerItem);
    }

    const QList<Kit *> kits = KitManager::kits();
    for (Kit *kit : kits) {
        if (kit->isAutoDetected()
                && DeviceTypeKitAspect::deviceTypeId(kit) == Constants::QNX_QNX_OS_TYPE
                && toolChainsToRemove.contains(ToolChainKitAspect::cxxToolChain(kit))) {
            KitManager::deregisterKit(kit);
        }
    }

    for (ToolChain *tc : toolChainsToRemove)
        ToolChainManager::deregisterToolChain(tc);

    for (const DebuggerItem &debuggerItem : std::as_const(debuggersToRemove))
        DebuggerItemManager::deregisterDebugger(debuggerItem.id());
}

QnxQtVersion *QnxConfiguration::qnxQtVersion(const QnxTarget &target) const
{
    const QtVersions versions = QtVersionManager::versions(
                Utils::equal(&QtVersion::type, QString::fromLatin1(Constants::QNX_QNX_QT)));
    for (QtVersion *version : versions) {
        auto qnxQt = dynamic_cast<QnxQtVersion *>(version);
        if (qnxQt && qnxQt->sdpPath() == m_envFile.parentDir()) {
            const Abis abis = version->qtAbis();
            for (const Abi &qtAbi : abis) {
                if (qtAbi == target.m_abi && qnxQt->cpuDir() == target.cpuDir())
                    return qnxQt;
            }
        }
    }
    return nullptr;
}

QVariant QnxConfiguration::createDebugger(const QnxTarget &target)
{
    Environment sysEnv = m_qnxHost.deviceEnvironment();
    sysEnv.modify(qnxEnvironmentItems());

    Debugger::DebuggerItem debugger;
    debugger.setCommand(target.m_debuggerPath);
    debugger.reinitializeFromFile(nullptr, &sysEnv);
    debugger.setUnexpandedDisplayName(Tr::tr("Debugger for %1 (%2)")
                .arg(m_configName)
                .arg(target.shortDescription()));
    return Debugger::DebuggerItemManager::registerDebugger(debugger);
}

Toolchains QnxConfiguration::createToolChains(const QnxTarget &target)
{
    Toolchains toolChains;

    for (const Id language : {ProjectExplorer::Constants::C_LANGUAGE_ID,
                              ProjectExplorer::Constants::CXX_LANGUAGE_ID}) {
        auto toolChain = new QnxToolChain;
        toolChain->setDetection(ToolChain::ManualDetection);
        toolChain->setLanguage(language);
        toolChain->setTargetAbi(target.m_abi);
        toolChain->setDisplayName(Tr::tr("QCC for %1 (%2)")
                    .arg(m_configName)
                    .arg(target.shortDescription()));
        toolChain->sdpPath.setValue(m_envFile.parentDir());
        toolChain->cpuDir.setValue(target.cpuDir());
        toolChain->resetToolChain(m_qccCompiler);
        ToolChainManager::registerToolChain(toolChain);

        toolChains.append(toolChain);
    }

    return toolChains;
}

void QnxConfiguration::createKit(const QnxTarget &target)
{
    Toolchains toolChains = createToolChains(target);
    QVariant debugger = createDebugger(target);

    QnxQtVersion *qnxQt = qnxQtVersion(target); // nullptr is ok.

    const auto init = [&](Kit *k) {
        QtKitAspect::setQtVersion(k, qnxQt);
        ToolChainKitAspect::setToolChain(k, toolChains[0]);
        ToolChainKitAspect::setToolChain(k, toolChains[1]);

        if (debugger.isValid())
            DebuggerKitAspect::setDebugger(k, debugger);

        DeviceTypeKitAspect::setDeviceTypeId(k, Constants::QNX_QNX_OS_TYPE);
        // TODO: Add sysroot?

        k->setUnexpandedDisplayName(Tr::tr("Kit for %1 (%2)")
                    .arg(m_configName)
                    .arg(target.shortDescription()));

        k->setAutoDetected(false);
        k->setAutoDetectionSource(m_envFile.toString());
        k->setMutable(DeviceKitAspect::id(), true);

        k->setSticky(ToolChainKitAspect::id(), true);
        k->setSticky(DeviceTypeKitAspect::id(), true);
        k->setSticky(SysRootKitAspect::id(), true);
        k->setSticky(DebuggerKitAspect::id(), true);
        k->setSticky(QmakeProjectManager::Constants::KIT_INFORMATION_ID, true);

        EnvironmentKitAspect::setEnvironmentChanges(k, qnxEnvironmentItems());
    };

    // add kit with device and qt version not sticky
    KitManager::registerKit(init);
}

void QnxConfiguration::ensureContents() const
{
    if (!m_hasContents)
        const_cast<QnxConfiguration *>(this)->mutableEnsureContents();
}

void QnxConfiguration::mutableEnsureContents()
{
    QTC_ASSERT(!m_envFile.isEmpty(), return);
    m_hasContents = true;

    m_qnxEnv = QnxUtils::qnxEnvironmentFromEnvFile(m_envFile);
    for (const EnvironmentItem &item : std::as_const(m_qnxEnv)) {
        if (item.name == QNXConfiguration)
            m_qnxConfiguration = m_envFile.withNewPath(item.value).canonicalPath();
        else if (item.name == QNXTarget)
            m_qnxTarget = m_envFile.withNewPath(item.value).canonicalPath();
        else if (item.name == QNXHost)
            m_qnxHost = m_envFile.withNewPath(item.value).canonicalPath();
    }

    const FilePath qccPath = m_qnxHost.pathAppended("usr/bin/qcc").withExecutableSuffix();
    if (qccPath.exists())
        m_qccCompiler = qccPath;

    // Some fallback in case the qconfig dir with .xml files is not found later.
    if (m_configName.isEmpty())
        m_configName = QString("%1 - %2").arg(m_qnxHost.fileName(), m_qnxTarget.fileName());

    m_targets = QnxUtils::findTargets(m_qnxTarget);

    // Assign debuggers.
    const FilePath hostUsrBinDir = m_qnxHost.pathAppended("usr/bin");
    QString pattern = "nto*-gdb";
    if (m_qnxHost.osType() == Utils::OsTypeWindows)
        pattern += ".exe";

    const FilePaths debuggerNames = hostUsrBinDir.dirEntries({{pattern}, QDir::Files});
    Environment sysEnv = m_qnxHost.deviceEnvironment();
    sysEnv.modify(qnxEnvironmentItems());

    for (const FilePath &debuggerPath : debuggerNames) {
        DebuggerItem item;
        item.setCommand(debuggerPath);
        item.reinitializeFromFile(nullptr, &sysEnv);
        bool found = false;
        for (const Abi &abi : item.abis()) {
            for (QnxTarget &target : m_targets) {
                if (target.m_abi.isCompatibleWith(abi)) {
                    found = true;

                    if (target.m_debuggerPath.isEmpty()) {
                        target.m_debuggerPath = debuggerPath;
                    } else {
                        qWarning() << debuggerPath << "has the same ABI as" << target.m_debuggerPath
                                   << "... discarded";
                        break;
                    }
                }
            }
        }
        if (!found)
            qWarning() << "No target found for" << debuggerPath.toUserOutput() << "... discarded";
    }

    // Remove debuggerless targets.
    Utils::erase(m_targets, [](const QnxTarget &target) {
        if (target.m_debuggerPath.isEmpty())
            qWarning() << "No debugger found for" << target.m_path << "... discarded";
        return target.m_debuggerPath.isEmpty();
    });

    const FilePath configPath = m_qnxConfiguration / "qconfig";
    if (!configPath.isDir())
        return;

    configPath.iterateDirectory([this, configPath](const FilePath &sdpFile) {
        QFile xmlFile(sdpFile.toFSPathString());
        if (!xmlFile.open(QIODevice::ReadOnly))
            return IterationPolicy::Continue;

        QDomDocument doc;
        if (!doc.setContent(&xmlFile))  // Skip error message
            return IterationPolicy::Continue;

        QDomElement docElt = doc.documentElement();
        if (docElt.tagName() != QLatin1String("qnxSystemDefinition"))
            return IterationPolicy::Continue;

        QDomElement childElt = docElt.firstChildElement(QLatin1String("installation"));
        // The file contains only one installation node
        if (childElt.isNull()) // The file contains only one base node
            return IterationPolicy::Continue;

        FilePath host = configPath.withNewPath(
            childElt.firstChildElement(QLatin1String("host")).text()).canonicalPath();
        if (m_qnxHost != host)
            return IterationPolicy::Continue;

        FilePath target = configPath.withNewPath(
            childElt.firstChildElement(QLatin1String("target")).text()).canonicalPath();
        if (m_qnxTarget != target)
            return IterationPolicy::Continue;

        m_configName = childElt.firstChildElement(QLatin1String("name")).text();
        QString version = childElt.firstChildElement(QLatin1String("version")).text();
        m_version = QnxVersionNumber(version);
        return IterationPolicy::Stop;
    }, {{"*.xml"}, QDir::Files});
}

EnvironmentItems QnxConfiguration::qnxEnvironmentItems() const
{
    ensureContents();
    return {
        {QNXConfiguration, m_qnxConfiguration.path()},
        {QNXTarget, m_qnxTarget.path()},
        {QNXHost, m_qnxHost.path()}
    };
}

const QnxTarget *QnxConfiguration::findTargetByDebuggerPath(
        const FilePath &path) const
{
    const auto it = std::find_if(m_targets.begin(), m_targets.end(),
                           [path](const QnxTarget &target) { return target.m_debuggerPath == path; });
    return it == m_targets.end() ? nullptr : &(*it);
}


// QnxSettingsPagePrivate

class QnxSettingsPagePrivate : public QObject
{
public:
    QnxSettingsPagePrivate()
    {
        connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
                this, &QnxSettingsPagePrivate::saveConfigs);
        // Can't do yet as not all devices are around.
        connect(DeviceManager::instance(), &DeviceManager::devicesLoaded,
                this, &QnxSettingsPagePrivate::restoreConfigurations);
    }

    void saveConfigs()
    {
        Store data;
        data.insert(QNXConfigsFileVersionKey, 1);
        int count = 0;
        for (const QnxConfiguration &config : std::as_const(m_configurations)) {
            Store tmp = config.toMap();
            if (tmp.isEmpty())
                continue;

            data.insert(numberedKey(QNXConfigDataKey, count), variantFromStore(tmp));
            ++count;
        }

        data.insert(QNXConfigCountKey, count);
        m_writer.save(data, Core::ICore::dialogParent());
    }

    void restoreConfigurations()
    {
        PersistentSettingsReader reader;
        if (!reader.load(qnxConfigSettingsFileName()))
            return;

        Store data = reader.restoreValues();
        int count = data.value(QNXConfigCountKey, 0).toInt();
        for (int i = 0; i < count; ++i) {
            const Key key = numberedKey(QNXConfigDataKey, i);
            if (!data.contains(key))
                continue;

            QnxConfiguration config;
            config.fromMap(storeFromVariant(data.value(key)));
            m_configurations[config.m_envFile] = config;
        }
    }

    QnxConfiguration *configurationFromEnvFile(const FilePath &envFile)
    {
        auto it = m_configurations.find(envFile);
        return it == m_configurations.end() ? nullptr : &*it;
    }

    QHash<FilePath, QnxConfiguration> m_configurations;
    PersistentSettingsWriter m_writer{qnxConfigSettingsFileName(), "QnxConfigurations"};
};

static QnxSettingsPagePrivate *dd = nullptr;


// QnxSettingsWidget

class ArchitecturesList final : public QWidget
{
public:
    void setConfiguration(const FilePath &envFile)
    {
        m_envFile = envFile;
        delete layout();

        QnxConfiguration *config = dd->configurationFromEnvFile(envFile);
        if (!config)
            return;

        auto l = new QHBoxLayout(this);
        for (const QnxTarget &target : config->m_targets) {
            auto button = new QPushButton(Tr::tr("Create Kit for %1").arg(target.cpuDir()));
            connect(button, &QPushButton::clicked, this, [config, target] {
                config->createKit(target);
            });
            l->addWidget(button);
        }
    }

    FilePath m_envFile;
};

class QnxSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    QnxSettingsWidget();

    enum State {
        Activated,
        Deactivated,
        Added,
        Removed
    };

    class ConfigState {
    public:
        bool operator ==(const ConfigState &cs) const
        {
            return envFile == cs.envFile && state == cs.state;
        }

        FilePath envFile;
        State state;
    };

    void apply() final;

    void addConfiguration();
    void removeConfiguration();
    void updateInformation();
    void populateConfigsCombo();

    void setConfigState(const FilePath &envFile, State state);

private:
    QComboBox *m_configsCombo = new QComboBox;
    QLabel *m_configName = new QLabel;
    QLabel *m_configVersion = new QLabel;
    QLabel *m_configHost = new QLabel;
    QLabel *m_configTarget = new QLabel;
    QLabel *m_compiler = new QLabel;
    QLabel *m_architectures = new QLabel;

    ArchitecturesList *m_kitCreation = new ArchitecturesList;

    QList<ConfigState> m_changedConfigs;
};

QnxSettingsWidget::QnxSettingsWidget()
{
    using namespace Layouting;

    Row {
        Column {
            m_configsCombo,
            Group {
                title(Tr::tr("Configuration Information:")),
                Form {
                    Tr::tr("Name:"), m_configName, br,
                    Tr::tr("Version:"), m_configVersion, br,
                    Tr::tr("Host:"), m_configHost, br,
                    Tr::tr("Target:"), m_configTarget, br,
                    Tr::tr("Compiler:"), m_compiler, br,
                    Tr::tr("Architectures:"), m_architectures
                }
            },
            Row { m_kitCreation, st },
            st
        },
        Column {
            PushButton {
                text(Tr::tr("Add...")),
                onClicked([this] { addConfiguration(); }, this)
            },
            PushButton {
                text(Tr::tr("Remove")),
                onClicked([this] { removeConfiguration(); }, this)
            },
            st
        }
    }.attachTo(this);

    populateConfigsCombo();

    connect(m_configsCombo, &QComboBox::currentIndexChanged,
            this, &QnxSettingsWidget::updateInformation);
}

void QnxSettingsWidget::addConfiguration()
{
    QString filter;
    if (HostOsInfo::isWindowsHost())
        filter = "*.bat file";
    else
        filter = "*.sh file";

    const FilePath envFile = FileUtils::getOpenFilePath(this, Tr::tr("Select QNX Environment File"),
                                                        {}, filter);
    if (envFile.isEmpty())
        return;

    if (dd->m_configurations.contains(envFile)) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             Tr::tr("Warning"),
                             Tr::tr("Configuration already exists."));
        return;
    }

    // Temporary to be able to check
    QnxConfiguration config(envFile);
    config.ensureContents();
    if (!config.isValid()) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             Tr::tr("Warning"),
                             Tr::tr("Configuration is not valid."));
        return;
    }

    setConfigState(envFile, Added);
    m_configsCombo->addItem(config.m_configName, QVariant::fromValue(envFile));
}

void QnxSettingsWidget::removeConfiguration()
{
    const FilePath envFile =  m_configsCombo->currentData().value<FilePath>();
    QTC_ASSERT(!envFile.isEmpty(), return);

    QnxConfiguration *config = dd->configurationFromEnvFile(envFile);
    QTC_ASSERT(config, return);

    config->ensureContents();

    QMessageBox::StandardButton button =
            QMessageBox::question(Core::ICore::dialogParent(),
                                  Tr::tr("Remove QNX Configuration"),
                                  Tr::tr("Are you sure you want to remove:\n %1?")
                                    .arg(config->m_configName),
                                  QMessageBox::Yes | QMessageBox::No);

    if (button == QMessageBox::Yes) {
        setConfigState(envFile, Removed);
        m_configsCombo->removeItem(m_configsCombo->currentIndex());
        updateInformation();
    }
}

void QnxSettingsWidget::updateInformation()
{
    const FilePath envFile = m_configsCombo->currentData().value<FilePath>();

    if (QnxConfiguration *config = dd->configurationFromEnvFile(envFile)) {
        config->ensureContents();
        m_configName->setText(config->m_configName);
        m_configVersion->setText(config->m_version.toString());
        m_configHost->setText(config->m_qnxHost.toString());
        m_configTarget->setText(config->m_qnxTarget.toString());
        m_compiler->setText(config->m_qccCompiler.toUserOutput());
        m_architectures->setText(config->architectureNames());
        m_kitCreation->setConfiguration(envFile);
    } else {
        m_configName->setText({});
        m_configVersion->setText({});
        m_configHost->setText({});
        m_compiler->setText({});
        m_architectures->setText({});
        m_kitCreation->setConfiguration({});
    }
}

void QnxSettingsWidget::populateConfigsCombo()
{
    m_configsCombo->clear();
    for (const QnxConfiguration &config : std::as_const(dd->m_configurations)) {
        config.ensureContents();
        m_configsCombo->addItem(config.m_configName, QVariant::fromValue(config.m_envFile));
    }
    updateInformation();
}

void QnxSettingsWidget::setConfigState(const FilePath &envFile, State state)
{
    State stateToRemove = Activated;
    switch (state) {
    case Added :
        stateToRemove = Removed;
        break;
    case Removed:
        stateToRemove = Added;
        break;
    case Activated:
        stateToRemove = Deactivated;
        break;
    case Deactivated:
        stateToRemove = Activated;
        break;
    }

    for (const ConfigState &configState : std::as_const(m_changedConfigs)) {
        if (configState.envFile == envFile && configState.state == stateToRemove)
            m_changedConfigs.removeAll(configState);
    }

    m_changedConfigs.append(ConfigState{envFile, state});
}

void QnxSettingsWidget::apply()
{
    for (const ConfigState &configState : std::as_const(m_changedConfigs)) {
        switch (configState.state) {
        case Activated: {
            QnxConfiguration *config = dd->configurationFromEnvFile(configState.envFile);
            QTC_ASSERT(config, break);
            config->activate();
            break;
        }
        case Deactivated: {
            QnxConfiguration *config = dd->configurationFromEnvFile(configState.envFile);
            QTC_ASSERT(config, break);
            config->deactivate();
            break;
        }
        case Added: {
            QnxConfiguration config(configState.envFile);
            config.ensureContents();
            dd->m_configurations.insert(configState.envFile, config);
            break;
        }
        case Removed:
            QnxConfiguration *config = dd->configurationFromEnvFile(configState.envFile);
            QTC_ASSERT(config, break);
            config->deactivate();
            dd->m_configurations.remove(configState.envFile);
            break;
        }
    }

    m_changedConfigs.clear();
    populateConfigsCombo();
}

// QnxSettingsPage

QList<ToolChain *> QnxSettingsPage::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> result;
    for (const QnxConfiguration &config : std::as_const(dd->m_configurations)) {
        config.ensureContents();
        for (const QnxTarget &target : std::as_const(config.m_targets)) {
            result +=  Utils::filtered(alreadyKnown, [config, target](ToolChain *tc) {
                return tc->typeId() == Constants::QNX_TOOLCHAIN_ID
                       && tc->targetAbi() == target.m_abi
                       && tc->compilerCommand() == config.m_qccCompiler;
            });
        }
    }
    return result;
}

QnxSettingsPage::QnxSettingsPage()
{
    setId("DD.Qnx Configuration");
    setDisplayName(Tr::tr("QNX"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new QnxSettingsWidget; });

    dd = new QnxSettingsPagePrivate;
}

QnxSettingsPage::~QnxSettingsPage()
{
    delete dd;
}

} // Qnx::Internal
