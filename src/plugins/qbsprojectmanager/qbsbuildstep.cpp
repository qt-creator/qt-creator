// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbsbuildstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagertr.h"
#include "qbsrequest.h"
#include "qbssession.h"
#include "qbssettings.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorertr.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/guard.h>
#include <utils/layoutbuilder.h>
#include <utils/outputformatter.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QThread>

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

const char QBS_CONFIG[] = "Qbs.Configuration";
const char QBS_KEEP_GOING[] = "Qbs.DryKeepGoing";
const char QBS_MAXJOBCOUNT[] = "Qbs.MaxJobs";
const char QBS_SHOWCOMMANDLINES[] = "Qbs.ShowCommandLines";
const char QBS_INSTALL[] = "Qbs.Install";
const char QBS_CLEAN_INSTALL_ROOT[] = "Qbs.CleanInstallRoot";

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace QbsProjectManager::Internal {

ArchitecturesAspect::ArchitecturesAspect(AspectContainer *container)
    : MultiSelectionAspect(container)
{
    m_abisToArchMap = {
        {ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A, "armv7a"},
        {ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A, "arm64"},
        {ProjectExplorer::Constants::ANDROID_ABI_X86, "x86"},
        {ProjectExplorer::Constants::ANDROID_ABI_X86_64, "x86_64"}};
    setAllValues(m_abisToArchMap.keys());
}

void ArchitecturesAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    MultiSelectionAspect::addToLayoutImpl(parent);
    const auto changeHandler = [this] {
        const QtVersion *qtVersion = QtKitAspect::qtVersion(m_kit);
        if (!qtVersion) {
            setVisibleDynamic(false);
            return;
        }
        const Abis abis = qtVersion->qtAbis();
        if (abis.size() <= 1) {
            setVisibleDynamic(false);
            return;
        }
        bool isAndroid = Utils::anyOf(abis, [](const Abi &abi) {
            return abi.osFlavor() == Abi::OSFlavor::AndroidLinuxFlavor;
        });
        if (!isAndroid) {
            setVisibleDynamic(false);
            return;
        }

        setVisibleDynamic(true);
    };
    connect(KitManager::instance(), &KitManager::kitsChanged, this, changeHandler);
    connect(this, &ArchitecturesAspect::changed, this, changeHandler);
    changeHandler();
}

QStringList ArchitecturesAspect::selectedArchitectures() const
{
    QStringList architectures;
    for (const auto &abi : value()) {
        if (m_abisToArchMap.contains(abi))
            architectures << m_abisToArchMap[abi];
    }
    return architectures;
}

void ArchitecturesAspect::setVisibleDynamic(bool visible)
{
    MultiSelectionAspect::setVisible(visible);
    m_isManagedByTarget = visible;
}

void ArchitecturesAspect::setSelectedArchitectures(const QStringList& architectures)
{
    QStringList newValue;
    for (auto i = m_abisToArchMap.constBegin(); i != m_abisToArchMap.constEnd(); ++i) {
        if (architectures.contains(i.value()))
            newValue << i.key();
    }
    if (newValue != value())
        setValue(newValue);
}

class QbsBuildStepConfigWidget : public QWidget
{
    Q_OBJECT
public:
    QbsBuildStepConfigWidget(QbsBuildStep *step);

private:
    void updateState();
    void updatePropertyEdit(const Store &data);

    void changeUseDefaultInstallDir(bool useDefault);
    void changeInstallDir();
    void applyCachedProperties();

    QbsBuildStep *qbsStep() const;

    Result<> validateProperties(const QString &text);

    class Property
    {
    public:
        Property() = default;
        Property(const QString &n, const QString &v, const QString &e) :
            name(n), value(v), effectiveValue(e)
        {}
        bool operator==(const Property &other) const
        {
            return name == other.name
                    && value == other.value
                    && effectiveValue == other.effectiveValue;
        }

        QString name;
        QString value;
        QString effectiveValue;
    };

    QbsBuildStep *m_qbsStep;
    QList<Property> m_propertyCache;
    Guard m_ignoreChanges;

    FancyLineEdit *propertyEdit;
    PathChooser *installDirChooser;
    QCheckBox *defaultInstallDirCheckBox;
};

// --------------------------------------------------------------------
// QbsBuildStep:
// --------------------------------------------------------------------

QbsBuildStep::QbsBuildStep(BuildStepList *bsl, Id id) :
    BuildStep(bsl, id)
{
    setDisplayName(QbsProjectManager::Tr::tr("Qbs Build"));
    setSummaryText(QbsProjectManager::Tr::tr("<b>Qbs:</b> %1").arg("build"));

    setQbsConfiguration(Store());

    auto qbsBuildConfig = qobject_cast<QbsBuildConfiguration *>(buildConfiguration());
    QTC_CHECK(qbsBuildConfig);
    connect(this, &QbsBuildStep::qbsConfigurationChanged,
            qbsBuildConfig, &QbsBuildConfiguration::qbsConfigurationChanged);

    buildVariantHolder.setDisplayName(QbsProjectManager::Tr::tr("Build variant:"));
    buildVariantHolder.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    buildVariantHolder.addOption({ProjectExplorer::Tr::tr("Debug"), {}, Constants::QBS_VARIANT_DEBUG});
    buildVariantHolder.addOption({ProjectExplorer::Tr::tr("Release"), {},
                                  Constants::QBS_VARIANT_RELEASE});
    buildVariantHolder.addOption({ProjectExplorer::Tr::tr("Profile"), {},
                                  Constants::QBS_VARIANT_PROFILING});

    selectedAbis.setLabelText(QbsProjectManager::Tr::tr("ABIs:"));
    selectedAbis.setDisplayStyle(MultiSelectionAspect::DisplayStyle::ListView);
    selectedAbis.setKit(kit());

    keepGoing.setSettingsKey(QBS_KEEP_GOING);
    keepGoing.setToolTip(
              QbsProjectManager::Tr::tr("Keep going when errors occur (if at all possible)."));
    keepGoing.setLabel(QbsProjectManager::Tr::tr("Keep going"),
                        BoolAspect::LabelPlacement::AtCheckBox);

    maxJobCount.setSettingsKey(QBS_MAXJOBCOUNT);
    maxJobCount.setLabel(QbsProjectManager::Tr::tr("Parallel jobs:"));
    maxJobCount.setToolTip(QbsProjectManager::Tr::tr("Number of concurrent build jobs."));
    maxJobCount.setValue(QThread::idealThreadCount());

    showCommandLines.setSettingsKey(QBS_SHOWCOMMANDLINES);
    showCommandLines.setLabel(QbsProjectManager::Tr::tr("Show command lines"),
                              BoolAspect::LabelPlacement::AtCheckBox);

    install.setSettingsKey(QBS_INSTALL);
    install.setValue(true);
    install.setLabel(QbsProjectManager::Tr::tr("Install"), BoolAspect::LabelPlacement::AtCheckBox);

    cleanInstallRoot.setSettingsKey(QBS_CLEAN_INSTALL_ROOT);
    cleanInstallRoot.setLabel(QbsProjectManager::Tr::tr("Clean install root"),
                              BoolAspect::LabelPlacement::AtCheckBox);

    forceProbes.setSettingsKey("Qbs.forceProbesKey");
    forceProbes.setLabel(QbsProjectManager::Tr::tr("Force probes"),
                          BoolAspect::LabelPlacement::AtCheckBox);

    commandLine.setDisplayStyle(StringAspect::TextEditDisplay);
    commandLine.setLabelText(QbsProjectManager::Tr::tr("Equivalent command line:"));
    commandLine.setReadOnly(true);

    connect(&maxJobCount, &BaseAspect::changed, this, &QbsBuildStep::updateState);
    connect(&keepGoing, &BaseAspect::changed, this, &QbsBuildStep::updateState);
    connect(&showCommandLines, &BaseAspect::changed, this, &QbsBuildStep::updateState);
    connect(&install, &BaseAspect::changed, this, &QbsBuildStep::updateState);
    connect(&cleanInstallRoot, &BaseAspect::changed, this, &QbsBuildStep::updateState);
    connect(&forceProbes, &BaseAspect::changed, this, &QbsBuildStep::updateState);

    connect(&buildVariantHolder, &BaseAspect::changed, this, [this] {
        const QString variant = buildVariantHolder.itemValue().toString();
        if (m_qbsConfiguration.value(Constants::QBS_CONFIG_VARIANT_KEY).toString() == variant)
            return;
        m_qbsConfiguration.insert(Constants::QBS_CONFIG_VARIANT_KEY, variant);
        emit qbsConfigurationChanged();
        if (BuildConfiguration *bc = buildConfiguration())
            emit bc->buildTypeChanged();
    });
    connect(&selectedAbis, &BaseAspect::changed, this, [this] {
        const QStringList architectures = selectedAbis.selectedArchitectures();
        if (configuredArchitectures() == architectures)
            return;
        if (architectures.isEmpty())
            m_qbsConfiguration.remove(Constants::QBS_ARCHITECTURES);
        else
            m_qbsConfiguration.insert(Constants::QBS_ARCHITECTURES, architectures.join(','));
        emit qbsConfigurationChanged();
    });
}

bool QbsBuildStep::init()
{
    const auto bc = qbsBuildConfiguration();
    if (!bc)
        return false;

    m_changedFiles = bc->changedFiles();
    m_activeFileTags = bc->activeFileTags();
    m_products = bc->products();
    return true;
}

void QbsBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParsers(kit()->createOutputParsers());
    BuildStep::setupOutputFormatter(formatter);
}

QWidget *QbsBuildStep::createConfigWidget()
{
    return new QbsBuildStepConfigWidget(this);
}

Store QbsBuildStep::qbsConfiguration(VariableHandling variableHandling) const
{
    Store config = m_qbsConfiguration;
    const auto qbsBuildConfig = qbsBuildConfiguration();
    config.insert(Constants::QBS_FORCE_PROBES_KEY, forceProbes());

    const auto store = [&config](TriState ts, const Key &key) {
        if (ts == TriState::Enabled)
            config.insert(key, true);
        else if (ts == TriState::Disabled)
            config.insert(key, false);
        else
            config.remove(key);
    };

    store(qbsBuildConfig->separateDebugInfoSetting(),
          Constants::QBS_CONFIG_SEPARATE_DEBUG_INFO_KEY);

    store(qbsBuildConfig->qmlDebuggingSetting(),
          Constants::QBS_CONFIG_QUICK_DEBUG_KEY);

    store(qbsBuildConfig->qtQuickCompilerSetting(),
          Constants::QBS_CONFIG_QUICK_COMPILER_KEY);

    if (variableHandling == ExpandVariables) {
        const MacroExpander * const expander = macroExpander();
        for (auto it = config.begin(), end = config.end(); it != end; ++it) {
            const QString rawString = it.value().toString();
            const QString expandedString = expander->expand(rawString);
            it.value() = expandedString;
        }
    }
    return config;
}

void QbsBuildStep::setQbsConfiguration(const Store &config)
{
    Store tmp = config;
    tmp.insert(Constants::QBS_CONFIG_PROFILE_KEY, qbsBuildSystem()->profile());
    QString buildVariant = tmp.value(Constants::QBS_CONFIG_VARIANT_KEY).toString();
    if (buildVariant.isEmpty()) {
        buildVariant = Constants::QBS_VARIANT_DEBUG;
        tmp.insert(Constants::QBS_CONFIG_VARIANT_KEY, buildVariant);
    }
    if (tmp == m_qbsConfiguration)
        return;
    m_qbsConfiguration = tmp;
    buildVariantHolder.setValue(buildVariantHolder.indexForItemValue(buildVariant));
    if (BuildConfiguration *bc = buildConfiguration())
        emit bc->buildTypeChanged();
    emit qbsConfigurationChanged();
}

bool QbsBuildStep::hasCustomInstallRoot() const
{
    return m_qbsConfiguration.contains(Constants::QBS_INSTALL_ROOT_KEY);
}

FilePath QbsBuildStep::installRoot(VariableHandling variableHandling) const
{
    const QString root =
            qbsConfiguration(variableHandling).value(Constants::QBS_INSTALL_ROOT_KEY).toString();
    if (!root.isNull())
        return FilePath::fromUserInput(root);
    QString defaultInstallDir = QbsSettings::defaultInstallDirTemplate();
    if (variableHandling == VariableHandling::ExpandVariables)
        defaultInstallDir = macroExpander()->expand(defaultInstallDir);
    return FilePath::fromUserInput(defaultInstallDir);
}

int QbsBuildStep::maxJobs() const
{
    if (maxJobCount() > 0)
        return maxJobCount();
    return QThread::idealThreadCount();
}

void QbsBuildStep::fromMap(const Store &map)
{
    BuildStep::fromMap(map);
    if (hasError())
        return;
    setQbsConfiguration(storeFromVariant(map.value(QBS_CONFIG)));
}

void QbsBuildStep::toMap(Store &map) const
{
    ProjectExplorer::BuildStep::toMap(map);
    map.insert(QBS_CONFIG, variantFromStore(m_qbsConfiguration));
}

QString QbsBuildStep::buildVariant() const
{
    return qbsConfiguration(PreserveVariables).value(Constants::QBS_CONFIG_VARIANT_KEY).toString();
}

QbsBuildConfiguration *QbsBuildStep::qbsBuildConfiguration() const
{
    return static_cast<QbsBuildConfiguration *>(buildConfiguration());
}

QbsBuildSystem *QbsBuildStep::qbsBuildSystem() const
{
    return static_cast<QbsBuildSystem *>(buildSystem());
}

Tasking::GroupItem QbsBuildStep::runRecipe()
{
    using namespace Tasking;
    const auto onPreParserSetup = [this](QbsRequest &request) {
        request.setParseData({qbsBuildSystem(), {}});
    };
    const auto onBuildSetup = [this](QbsRequest &request) {
        QbsSession *session = qbsBuildSystem()->session();
        if (!session) {
            emit addOutput(Tr::tr("No qbs session exists for this target."),
                           OutputFormat::ErrorMessage);
            return SetupResult::StopWithError;
        }
        QJsonObject requestData;
        requestData.insert("type", "build-project");
        requestData.insert("max-job-count", maxJobs());
        requestData.insert("keep-going", keepGoing());
        requestData.insert("command-echo-mode", showCommandLines() ? "command-line" : "summary");
        requestData.insert("install", install());
        QbsSession::insertRequestedModuleProperties(requestData);
        requestData.insert("clean-install-root", cleanInstallRoot());
        if (!m_products.isEmpty())
            requestData.insert("products", QJsonArray::fromStringList(m_products));
        if (!m_changedFiles.isEmpty()) {
            const auto changedFilesArray = QJsonArray::fromStringList(m_changedFiles);
            requestData.insert("changed-files", changedFilesArray);
            requestData.insert("files-to-consider", changedFilesArray);
        }
        if (!m_activeFileTags.isEmpty())
            requestData.insert("active-file-tags", QJsonArray::fromStringList(m_activeFileTags));
        requestData.insert("data-mode", "only-if-changed");

        request.setSession(session);
        request.setRequestData(requestData);
        connect(&request, &QbsRequest::progressChanged, this, &BuildStep::progress);
        connect(&request, &QbsRequest::outputAdded, this,
                [this](const QString &output, OutputFormat format) {
            emit addOutput(output, format);
        });
        connect(&request, &QbsRequest::taskAdded, this, [this](const Task &task) {
            emit addTask(task, 1);
        });
        return SetupResult::Continue;
    };

    const Group root {
        // We need a pre-build parsing step in order not to lose project file changes done
        // right before building (but before the delay has elapsed).
        QbsRequestTask(onPreParserSetup),
        Group {
            continueOnError,
            QbsRequestTask(onBuildSetup),
            // Building can uncover additional target artifacts.
            Sync([this] { qbsBuildSystem()->updateAfterBuild(); }),
        }
    };

    return root;
}

void QbsBuildStep::updateState()
{
    emit qbsConfigurationChanged();
}

QStringList QbsBuildStep::configuredArchitectures() const
{
    return m_qbsConfiguration[Constants::QBS_ARCHITECTURES].toString().split(',',
            Qt::SkipEmptyParts);
}

QbsBuildStepData QbsBuildStep::stepData() const
{
    QbsBuildStepData data;
    data.command = "build";
    data.dryRun = false;
    data.keepGoing = keepGoing();
    data.forceProbeExecution = forceProbes();
    data.showCommandLines = showCommandLines();
    data.noInstall = !install();
    data.noBuild = false;
    data.cleanInstallRoot = cleanInstallRoot();
    data.jobCount = maxJobs();
    data.installRoot = installRoot();
    return data;
}

// --------------------------------------------------------------------
// QbsBuildStepConfigWidget:
// --------------------------------------------------------------------

QbsBuildStepConfigWidget::QbsBuildStepConfigWidget(QbsBuildStep *step)
    : m_qbsStep(step)
{
    connect(step, &ProjectConfiguration::displayNameChanged,
            this, &QbsBuildStepConfigWidget::updateState);
    connect(step->qbsBuildConfiguration(), &QbsBuildConfiguration::qbsConfigurationChanged,
            this, &QbsBuildStepConfigWidget::updateState);
    connect(&QbsSettings::instance(), &QbsSettings::settingsChanged,
            this, &QbsBuildStepConfigWidget::updateState);
    connect(step->buildConfiguration(), &BuildConfiguration::buildDirectoryChanged,
            this, &QbsBuildStepConfigWidget::updateState);

    setContentsMargins(0, 0, 0, 0);

    propertyEdit = new FancyLineEdit(this);
    propertyEdit->setToolTip(QbsProjectManager::Tr::tr("Properties to pass to the project."));
    propertyEdit->setValidationFunction([this](const QString &text) {
        return validateProperties(text);
    });

    defaultInstallDirCheckBox = new QCheckBox(this);
    defaultInstallDirCheckBox->setText(QbsProjectManager::Tr::tr("Use default location"));

    installDirChooser = new PathChooser(this);
    installDirChooser->setExpectedKind(PathChooser::Directory);

    using namespace Layouting;
    Form {
        step->buildVariantHolder, br,
        step->selectedAbis, br,
        step->maxJobCount, br,
        QbsProjectManager::Tr::tr("Properties:"), propertyEdit, br,

        QbsProjectManager::Tr::tr("Flags:"),
        step->keepGoing,
        step->showCommandLines,
        step->forceProbes, br,

        QbsProjectManager::Tr::tr("Installation flags:"),
        step->install,
        step->cleanInstallRoot,
        defaultInstallDirCheckBox, br,

        QbsProjectManager::Tr::tr("Installation directory:"), installDirChooser, br,
        step->commandLine, br,
        noMargin,
    }.attachTo(this);

    connect(defaultInstallDirCheckBox, &QCheckBox::toggled, this,
            &QbsBuildStepConfigWidget::changeUseDefaultInstallDir);

    connect(installDirChooser, &PathChooser::rawPathChanged, this,
            &QbsBuildStepConfigWidget::changeInstallDir);

    updateState();
}

void QbsBuildStepConfigWidget::updateState()
{
    if (!m_ignoreChanges.isLocked()) {
        updatePropertyEdit(m_qbsStep->qbsConfiguration(QbsBuildStep::PreserveVariables));
        installDirChooser->setFilePath(m_qbsStep->installRoot(QbsBuildStep::PreserveVariables));
        defaultInstallDirCheckBox->setChecked(!m_qbsStep->hasCustomInstallRoot());
        m_qbsStep->selectedAbis.setSelectedArchitectures(m_qbsStep->configuredArchitectures());
    }

    const auto qbsBuildConfig = m_qbsStep->qbsBuildConfiguration();

    QString command = qbsBuildConfig->equivalentCommandLine(m_qbsStep->stepData());

    for (int i = 0; i < m_propertyCache.count(); ++i) {
        command += ' ' + m_propertyCache.at(i).name + ':' + m_propertyCache.at(i).effectiveValue;
    }

    if (m_qbsStep->selectedAbis.isManagedByTarget()) {
        QStringList selectedArchitectures = m_qbsStep->configuredArchitectures();
        if (!selectedArchitectures.isEmpty()) {
            command += ' ' + QLatin1String(Constants::QBS_ARCHITECTURES) + ':' +
                    selectedArchitectures.join(',');
        }
    }

    const auto addToCommand = [&command](TriState ts, const QString &key) {
        if (ts == TriState::Enabled)
            command.append(' ').append(key).append(":true");
        else if (ts == TriState::Disabled)
            command.append(' ').append(key).append(":false");
        // Do nothing for TriState::Default
    };

    addToCommand(qbsBuildConfig->separateDebugInfoSetting(),
                 Constants::QBS_CONFIG_SEPARATE_DEBUG_INFO_KEY);

    addToCommand(qbsBuildConfig->qmlDebuggingSetting(),
                 Constants::QBS_CONFIG_QUICK_DEBUG_KEY);

    addToCommand(qbsBuildConfig->qtQuickCompilerSetting(),
                 Constants::QBS_CONFIG_QUICK_COMPILER_KEY);

    m_qbsStep->commandLine.setValue(command);
}


void QbsBuildStepConfigWidget::updatePropertyEdit(const Store &data)
{
    Store editable = data;

    // remove data that is edited with special UIs:
    editable.remove(Constants::QBS_CONFIG_PROFILE_KEY);
    editable.remove(Constants::QBS_CONFIG_VARIANT_KEY);
    editable.remove(Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY); // For existing .user files
    editable.remove(Constants::QBS_CONFIG_SEPARATE_DEBUG_INFO_KEY);
    editable.remove(Constants::QBS_CONFIG_QUICK_DEBUG_KEY);
    editable.remove(Constants::QBS_CONFIG_QUICK_COMPILER_KEY);
    editable.remove(Constants::QBS_FORCE_PROBES_KEY);
    editable.remove(Constants::QBS_INSTALL_ROOT_KEY);
    if (m_qbsStep->selectedAbis.isManagedByTarget())
        editable.remove(Constants::QBS_ARCHITECTURES);

    QStringList propertyList;
    for (Store::const_iterator i = editable.constBegin(); i != editable.constEnd(); ++i)
        propertyList.append(QString::fromUtf8(i.key().toByteArray()) + ':' + i.value().toString());

    propertyEdit->setText(ProcessArgs::joinArgs(propertyList));
}

void QbsBuildStepConfigWidget::changeUseDefaultInstallDir(bool useDefault)
{
    const GuardLocker locker(m_ignoreChanges);
    Store config = m_qbsStep->qbsConfiguration(QbsBuildStep::PreserveVariables);
    installDirChooser->setEnabled(!useDefault);
    if (useDefault)
        config.remove(Constants::QBS_INSTALL_ROOT_KEY);
    else
        config.insert(Constants::QBS_INSTALL_ROOT_KEY, installDirChooser->unexpandedFilePath().toUrlishString());
    m_qbsStep->setQbsConfiguration(config);
}

void QbsBuildStepConfigWidget::changeInstallDir()
{
    if (!m_qbsStep->hasCustomInstallRoot())
        return;
    const GuardLocker locker(m_ignoreChanges);
    Store config = m_qbsStep->qbsConfiguration(QbsBuildStep::PreserveVariables);
    config.insert(Constants::QBS_INSTALL_ROOT_KEY, installDirChooser->unexpandedFilePath().toUrlishString());
    m_qbsStep->setQbsConfiguration(config);
}

void QbsBuildStepConfigWidget::applyCachedProperties()
{
    Store data;
    const Store tmp = m_qbsStep->qbsConfiguration(QbsBuildStep::PreserveVariables);

    // Insert values set up with special UIs:
    data.insert(Constants::QBS_CONFIG_PROFILE_KEY,
                tmp.value(Constants::QBS_CONFIG_PROFILE_KEY));
    data.insert(Constants::QBS_CONFIG_VARIANT_KEY,
                tmp.value(Constants::QBS_CONFIG_VARIANT_KEY));
    KeyList additionalSpecialKeys({Constants::QBS_CONFIG_DECLARATIVE_DEBUG_KEY,
                                   Constants::QBS_CONFIG_QUICK_DEBUG_KEY,
                                   Constants::QBS_CONFIG_QUICK_COMPILER_KEY,
                                   Constants::QBS_CONFIG_SEPARATE_DEBUG_INFO_KEY,
                                   Constants::QBS_INSTALL_ROOT_KEY});
    if (m_qbsStep->selectedAbis.isManagedByTarget())
        additionalSpecialKeys << Constants::QBS_ARCHITECTURES;
    for (const Key &key : std::as_const(additionalSpecialKeys)) {
        const auto it = tmp.constFind(key);
        if (it != tmp.cend())
            data.insert(key, it.value());
    }

    for (int i = 0; i < m_propertyCache.count(); ++i) {
        const Property &property = m_propertyCache.at(i);
        data.insert(property.name.toUtf8(), property.value);
    }

    const GuardLocker locker(m_ignoreChanges);
    m_qbsStep->setQbsConfiguration(data);
}

QbsBuildStep *QbsBuildStepConfigWidget::qbsStep() const
{
    return m_qbsStep;
}

Result<> QbsBuildStepConfigWidget::validateProperties(const QString &text)
{
    ProcessArgs::SplitError err;
    const QStringList argList = ProcessArgs::splitArgs(text, HostOsInfo::hostOs(), false, &err);
    if (err != ProcessArgs::SplitOk)
        return ResultError(QbsProjectManager::Tr::tr("Could not split properties."));

    QList<Property> properties;
    const MacroExpander * const expander = m_qbsStep->macroExpander();
    for (const QString &rawArg : argList) {
        int pos = rawArg.indexOf(':');
        if (pos > 0) {
            const QString propertyName = rawArg.left(pos);
            QStringList specialProperties{
                Constants::QBS_CONFIG_PROFILE_KEY, Constants::QBS_CONFIG_VARIANT_KEY,
                Constants::QBS_CONFIG_QUICK_DEBUG_KEY, Constants::QBS_CONFIG_QUICK_COMPILER_KEY,
                Constants::QBS_INSTALL_ROOT_KEY, Constants::QBS_CONFIG_SEPARATE_DEBUG_INFO_KEY,
            };
            if (m_qbsStep->selectedAbis.isManagedByTarget())
                specialProperties << Constants::QBS_ARCHITECTURES;
            if (specialProperties.contains(propertyName)) {
                return ResultError(QbsProjectManager::Tr::tr("Property \"%1\" cannot be set here. "
                                          "Please use the dedicated UI element.").arg(propertyName));
            }
            const QString rawValue = rawArg.mid(pos + 1);
            Property property(propertyName, rawValue, expander->expand(rawValue));
            properties.append(property);
        } else {
            return ResultError(QbsProjectManager::Tr::tr("No \":\" found in property definition."));
        }
    }

    if (m_propertyCache != properties) {
        m_propertyCache = properties;
        applyCachedProperties();
    }
    return ResultOk;
}

// --------------------------------------------------------------------
// QbsBuildStepFactory:
// --------------------------------------------------------------------

QbsBuildStepFactory::QbsBuildStepFactory()
{
    registerStep<QbsBuildStep>(Constants::QBS_BUILDSTEP_ID);
    setDisplayName(QbsProjectManager::Tr::tr("Qbs Build"));
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setSupportedConfiguration(Constants::QBS_BC_ID);
    setSupportedProjectType(Constants::PROJECT_ID);
}

} // namespace QbsProjectManager::Internal

#include "qbsbuildstep.moc"
