// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdsettings.h"

#include "clangdiagnosticconfigsmodel.h"
#include "clangdiagnosticconfigsselectionwidget.h"
#include "clangdiagnosticconfigswidget.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cpptoolsreuse.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/session.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectsettingswidget.h>

#include <utils/clangutils.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/qtcprocess.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QInputDialog>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStringListModel>
#include <QTimer>
#include <QVBoxLayout>
#include <QVersionNumber>

#include <limits>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor {

static FilePath g_defaultClangdFilePath;
static FilePath fallbackClangdFilePath()
{
    if (g_defaultClangdFilePath.exists())
        return g_defaultClangdFilePath;
    return Environment::systemEnvironment().searchInPath("clangd");
}

static Id initialClangDiagnosticConfigId() { return Constants::CPP_CLANG_DIAG_CONFIG_BUILDSYSTEM; }

static Key clangdSettingsKey() { return "ClangdSettings"; }
static Key useClangdKey() { return "UseClangdV7"; }
static Key clangdPathKey() { return "ClangdPath"; }
static Key clangdIndexingKey() { return "ClangdIndexing"; }
static Key clangdProjectIndexPathKey() { return "ClangdProjectIndexPath"; }
static Key clangdSessionIndexPathKey() { return "ClangdSessionIndexPath"; }
static Key clangdIndexingPriorityKey() { return "ClangdIndexingPriority"; }
static Key clangdHeaderSourceSwitchModeKey() { return "ClangdHeaderSourceSwitchMode"; }
static Key clangdCompletionRankingModelKey() { return "ClangdCompletionRankingModel"; }
static Key clangdHeaderInsertionKey() { return "ClangdHeaderInsertion"; }
static Key clangdThreadLimitKey() { return "ClangdThreadLimit"; }
static Key clangdDocumentThresholdKey() { return "ClangdDocumentThreshold"; }
static Key clangdSizeThresholdEnabledKey() { return "ClangdSizeThresholdEnabled"; }
static Key clangdSizeThresholdKey() { return "ClangdSizeThreshold"; }
static Key useGlobalSettingsKey() { return "useGlobalSettings"; }
static Key clangdblockIndexingSettingsKey() { return "blockIndexing"; }
static Key sessionsWithOneClangdKey() { return "SessionsWithOneClangd"; }
static Key diagnosticConfigIdKey() { return "diagnosticConfigId"; }
static Key checkedHardwareKey() { return "checkedHardware"; }
static Key completionResultsKey() { return "completionResults"; }
static Key updateDependentSourcesKey() { return "updateDependentSources"; }

QString ClangdSettings::priorityToString(const IndexingPriority &priority)
{
    switch (priority) {
    case IndexingPriority::Background: return "background";
    case IndexingPriority::Normal: return "normal";
    case IndexingPriority::Low: return "low";
    case IndexingPriority::Off: return {};
    }
    return {};
}

QString ClangdSettings::priorityToDisplayString(const IndexingPriority &priority)
{
    switch (priority) {
    case IndexingPriority::Background: return Tr::tr("Background Priority");
    case IndexingPriority::Normal: return Tr::tr("Normal Priority");
    case IndexingPriority::Low: return Tr::tr("Low Priority");
    case IndexingPriority::Off: return Tr::tr("Off");
    }
    return {};
}

QString ClangdSettings::headerSourceSwitchModeToDisplayString(HeaderSourceSwitchMode mode)
{
    switch (mode) {
    case HeaderSourceSwitchMode::BuiltinOnly: return Tr::tr("Use Built-in Only");
    case HeaderSourceSwitchMode::ClangdOnly: return Tr::tr("Use Clangd Only");
    case HeaderSourceSwitchMode::Both: return Tr::tr("Try Both");
    }
    return {};
}

QString ClangdSettings::rankingModelToCmdLineString(CompletionRankingModel model)
{
    switch (model) {
    case CompletionRankingModel::Default: break;
    case CompletionRankingModel::DecisionForest: return "decision_forest";
    case CompletionRankingModel::Heuristics: return "heuristics";
    }
    QTC_ASSERT(false, return {});
}

QString ClangdSettings::rankingModelToDisplayString(CompletionRankingModel model)
{
    switch (model) {
    case CompletionRankingModel::Default: return Tr::tr("Default");
    case CompletionRankingModel::DecisionForest: return Tr::tr("Decision Forest");
    case CompletionRankingModel::Heuristics: return Tr::tr("Heuristics");
    }
    QTC_ASSERT(false, return {});
}

QString ClangdSettings::defaultProjectIndexPathTemplate()
{
    return QDir::toNativeSeparators("%{BuildConfig:BuildDirectory:FilePath}/.qtc_clangd");
}

QString ClangdSettings::defaultSessionIndexPathTemplate()
{
    return QDir::toNativeSeparators("%{IDE:UserResourcePath}/.qtc_clangd/%{Session:FileBaseName}");
}

ClangdSettings &ClangdSettings::instance()
{
    static ClangdSettings settings;
    return settings;
}

ClangdSettings::ClangdSettings()
{
    loadSettings();
    const auto sessionMgr = Core::SessionManager::instance();
    connect(sessionMgr, &Core::SessionManager::sessionRemoved, this, [this](const QString &name) {
        m_data.sessionsWithOneClangd.removeOne(name);
    });
    connect(sessionMgr,
            &Core::SessionManager::sessionRenamed,
            this,
            [this](const QString &oldName, const QString &newName) {
                const auto index = m_data.sessionsWithOneClangd.indexOf(oldName);
                if (index != -1)
                    m_data.sessionsWithOneClangd[index] = newName;
            });
}

bool ClangdSettings::useClangd() const
{
    return m_data.useClangd && clangdVersion(clangdFilePath()) >= minimumClangdVersion();
}

void ClangdSettings::setUseClangd(bool use) { instance().m_data.useClangd = use; }

void ClangdSettings::setUseClangdAndSave(bool use)
{
    setUseClangd(use);
    instance().saveSettings();
}

bool ClangdSettings::hardwareFulfillsRequirements()
{
    instance().m_data.haveCheckedHardwareReqirements = true;
    instance().saveSettings();
    const quint64 minRam = quint64(12) * 1024 * 1024 * 1024;
    const std::optional<quint64> totalRam = Utils::HostOsInfo::totalMemoryInstalledInBytes();
    return !totalRam || *totalRam >= minRam;
}

bool ClangdSettings::haveCheckedHardwareRequirements()
{
    return instance().data().haveCheckedHardwareReqirements;
}

void ClangdSettings::setDefaultClangdPath(const FilePath &filePath)
{
    g_defaultClangdFilePath = filePath;
}

void ClangdSettings::setCustomDiagnosticConfigs(const ClangDiagnosticConfigs &configs)
{
    if (instance().customDiagnosticConfigs() == configs)
        return;
    instance().m_data.customDiagnosticConfigs = configs;
    instance().saveSettings();
}

ClangDiagnosticConfigsModel ClangdSettings::diagnosticConfigsModel()
{
    const ClangDiagnosticConfigs &customConfigs = instance().customDiagnosticConfigs();
    ClangDiagnosticConfigsModel model;
    model.addBuiltinConfigs();
    for (const ClangDiagnosticConfig &config : customConfigs)
        model.appendOrUpdate(config);
    return model;
}

FilePath ClangdSettings::clangdFilePath() const
{
    if (!m_data.executableFilePath.isEmpty())
        return m_data.executableFilePath;
    return fallbackClangdFilePath();
}

FilePath ClangdSettings::projectIndexPath(const MacroExpander &expander) const
{
    return FilePath::fromUserInput(expander.expand(m_data.projectIndexPathTemplate));
}

FilePath ClangdSettings::sessionIndexPath(const MacroExpander &expander) const
{
    return FilePath::fromUserInput(expander.expand(m_data.sessionIndexPathTemplate));
}

bool ClangdSettings::sizeIsOkay(const FilePath &fp) const
{
    return !sizeThresholdEnabled() || sizeThresholdInKb() * 1024 >= fp.fileSize();
}

ClangDiagnosticConfigs ClangdSettings::customDiagnosticConfigs() const
{
    return m_data.customDiagnosticConfigs;
}

Id ClangdSettings::diagnosticConfigId() const
{
    if (!diagnosticConfigsModel().hasConfigWithId(m_data.diagnosticConfigId))
        return initialClangDiagnosticConfigId();
    return m_data.diagnosticConfigId;
}

ClangDiagnosticConfig ClangdSettings::diagnosticConfig() const
{
    return diagnosticConfigsModel().configWithId(diagnosticConfigId());
}

ClangdSettings::Granularity ClangdSettings::granularity() const
{
    if (m_data.sessionsWithOneClangd.contains(Core::SessionManager::activeSession()))
        return Granularity::Session;
    return Granularity::Project;
}

void ClangdSettings::setData(const Data &data, bool saveAndEmitSignal)
{
    if (this == &instance() && data != m_data) {
        m_data = data;
        if (saveAndEmitSignal) {
            saveSettings();
            emit changed();
        }
    }
}

static FilePath getClangHeadersPathFromClang(const FilePath &clangdFilePath)
{
    const FilePath clangFilePath = clangdFilePath.absolutePath().pathAppended("clang")
                                       .withExecutableSuffix();
    if (!clangFilePath.exists())
        return {};
    Process clang;
    clang.setCommand({clangFilePath, {"-print-resource-dir"}});
    clang.start();
    if (!clang.waitForFinished())
        return {};
    const FilePath resourceDir = FilePath::fromUserInput(QString::fromLocal8Bit(
        clang.rawStdOut().trimmed()));
    if (resourceDir.isEmpty() || !resourceDir.exists())
        return {};
    const FilePath includeDir = resourceDir.pathAppended("include");
    if (!includeDir.exists())
        return {};
    return includeDir;
}

static FilePath getClangHeadersPath(const FilePath &clangdFilePath)
{
    const FilePath headersPath = getClangHeadersPathFromClang(clangdFilePath);
    if (!headersPath.isEmpty())
        return headersPath;

    const QVersionNumber version = Utils::clangdVersion(clangdFilePath);
    QTC_ASSERT(!version.isNull(), return {});
    static const QStringList libDirs{"lib", "lib64"};
    const QStringList versionStrings{QString::number(version.majorVersion()), version.toString()};
    for (const QString &libDir : libDirs) {
        for (const QString &versionString : versionStrings) {
            const FilePath includePath = clangdFilePath.absolutePath().parentDir()
                                             .pathAppended(libDir).pathAppended("clang")
                                             .pathAppended(versionString).pathAppended("include");
            if (includePath.exists())
                return includePath;
        }
    }
    QTC_CHECK(false);
    return {};
}

FilePath ClangdSettings::clangdIncludePath() const
{
    QTC_ASSERT(useClangd(), return {});
    FilePath clangdPath = clangdFilePath();
    QTC_ASSERT(!clangdPath.isEmpty() && clangdPath.exists(), return {});
    static QHash<FilePath, FilePath> headersPathCache;
    const auto it = headersPathCache.constFind(clangdPath);
    if (it != headersPathCache.constEnd())
        return *it;
    const FilePath headersPath = getClangHeadersPath(clangdPath);
    if (!headersPath.isEmpty())
        headersPathCache.insert(clangdPath, headersPath);
    return headersPath;
}

FilePath ClangdSettings::clangdUserConfigFilePath()
{
    return FilePath::fromString(
               QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
           / "clangd/config.yaml";
}

void ClangdSettings::loadSettings()
{
    const auto settings = Core::ICore::settings();

    m_data.fromMap(Utils::storeFromSettings(clangdSettingsKey(), settings));

    settings->beginGroup(Constants::CPPEDITOR_SETTINGSGROUP);
    m_data.customDiagnosticConfigs = diagnosticConfigsFromSettings(settings);

    // Pre-8.0 compat
    static const Key oldKey("ClangDiagnosticConfig");
    const QVariant configId = settings->value(oldKey);
    if (configId.isValid()) {
        m_data.diagnosticConfigId = Id::fromSetting(configId);
        settings->setValue(oldKey, {});
    }

    settings->endGroup();
}

void ClangdSettings::saveSettings()
{
    const auto settings = Core::ICore::settings();
    const ClangdSettings::Data defaultData;
    Utils::storeToSettingsWithDefault(clangdSettingsKey(),
                                      settings,
                                      m_data.toMap(),
                                      defaultData.toMap());
    settings->beginGroup(Constants::CPPEDITOR_SETTINGSGROUP);
    diagnosticConfigsToSettings(settings, m_data.customDiagnosticConfigs);
    settings->endGroup();
}

#ifdef WITH_TESTS
void ClangdSettings::setClangdFilePath(const FilePath &filePath)
{
    instance().m_data.executableFilePath = filePath;
}
#endif

ClangdProjectSettings::ClangdProjectSettings(Project *project) : m_project(project)
{
    loadSettings();
}

ClangdProjectSettings::ClangdProjectSettings(BuildConfiguration *bc)
    : ClangdProjectSettings(bc ? bc->project() : nullptr)
{}

ClangdSettings::Data ClangdProjectSettings::settings() const
{
    const ClangdSettings::Data globalData = ClangdSettings::instance().data();
    ClangdSettings::Data data = globalData;
    if (!m_useGlobalSettings) {
        data = m_customSettings;
        // This property is global by definition.
        data.sessionsWithOneClangd = ClangdSettings::instance().data().sessionsWithOneClangd;

        // This list exists only once.
        data.customDiagnosticConfigs = ClangdSettings::instance().data().customDiagnosticConfigs;
    }
    if (m_blockIndexing)
        data.indexingPriority = ClangdSettings::IndexingPriority::Off;
    return data;
}

void ClangdProjectSettings::setSettings(const ClangdSettings::Data &data)
{
    m_customSettings = data;
    saveSettings();
    ClangdSettings::setCustomDiagnosticConfigs(data.customDiagnosticConfigs);
    emit ClangdSettings::instance().changed();
}

void ClangdProjectSettings::setUseGlobalSettings(bool useGlobal)
{
    m_useGlobalSettings = useGlobal;
    saveSettings();
    emit ClangdSettings::instance().changed();
}

void ClangdProjectSettings::setDiagnosticConfigId(Utils::Id configId)
{
    m_customSettings.diagnosticConfigId = configId;
    saveSettings();
    emit ClangdSettings::instance().changed();
}

void ClangdProjectSettings::blockIndexing()
{
    if (m_blockIndexing)
        return;
    m_blockIndexing = true;
    saveSettings();
    emit ClangdSettings::instance().changed();
}

void ClangdProjectSettings::unblockIndexing()
{
    if (!m_blockIndexing)
        return;
    m_blockIndexing = false;
    saveSettings();
    // Do not emit changed here since that would restart clients with blocked indexing
}

void ClangdProjectSettings::loadSettings()
{
    if (!m_project)
        return;
    const Store data = storeFromVariant(m_project->namedSettings(clangdSettingsKey()));
    m_useGlobalSettings = data.value(useGlobalSettingsKey(), true).toBool();
    m_blockIndexing = data.value(clangdblockIndexingSettingsKey(), false).toBool();
    if (!m_useGlobalSettings)
        m_customSettings.fromMap(data);
}

void ClangdProjectSettings::saveSettings()
{
    if (!m_project)
        return;
    Store data;
    if (!m_useGlobalSettings)
        data = m_customSettings.toMap();
    data.insert(useGlobalSettingsKey(), m_useGlobalSettings);
    data.insert(clangdblockIndexingSettingsKey(), m_blockIndexing);
    m_project->setNamedSettings(clangdSettingsKey(), variantFromStore(data));
}

Store ClangdSettings::Data::toMap() const
{
    Store map;

    map.insert(useClangdKey(), useClangd);

    map.insert(clangdPathKey(),
               executableFilePath != fallbackClangdFilePath() ? executableFilePath.toUrlishString()
                                                              : QString());

    map.insert(clangdIndexingKey(), indexingPriority != IndexingPriority::Off);
    map.insert(clangdIndexingPriorityKey(), int(indexingPriority));
    map.insert(clangdProjectIndexPathKey(), projectIndexPathTemplate);
    map.insert(clangdSessionIndexPathKey(), sessionIndexPathTemplate);
    map.insert(clangdHeaderSourceSwitchModeKey(), int(headerSourceSwitchMode));
    map.insert(clangdCompletionRankingModelKey(), int(completionRankingModel));
    map.insert(clangdHeaderInsertionKey(), autoIncludeHeaders);
    map.insert(clangdThreadLimitKey(), workerThreadLimit);
    map.insert(clangdDocumentThresholdKey(), documentUpdateThreshold);
    map.insert(clangdSizeThresholdEnabledKey(), sizeThresholdEnabled);
    map.insert(clangdSizeThresholdKey(), sizeThresholdInKb);
    map.insert(sessionsWithOneClangdKey(), sessionsWithOneClangd);
    map.insert(diagnosticConfigIdKey(), diagnosticConfigId.toSetting());
    map.insert(checkedHardwareKey(), haveCheckedHardwareReqirements);
    map.insert(completionResultsKey(), completionResults);
    map.insert(updateDependentSourcesKey(), updateDependentSources);
    return map;
}

void ClangdSettings::Data::fromMap(const Store &map)
{
    useClangd = map.value(useClangdKey(), true).toBool();
    executableFilePath = FilePath::fromString(map.value(clangdPathKey()).toString());
    indexingPriority = IndexingPriority(
        map.value(clangdIndexingPriorityKey(), int(this->indexingPriority)).toInt());
    const auto it = map.find(clangdIndexingKey());
    if (it != map.end() && !it->toBool())
        indexingPriority = IndexingPriority::Off;
    projectIndexPathTemplate
        = map.value(clangdProjectIndexPathKey(), defaultProjectIndexPathTemplate()).toString();
    sessionIndexPathTemplate
        = map.value(clangdSessionIndexPathKey(), defaultSessionIndexPathTemplate()).toString();
    headerSourceSwitchMode = HeaderSourceSwitchMode(map.value(clangdHeaderSourceSwitchModeKey(),
                                                              int(headerSourceSwitchMode)).toInt());
    completionRankingModel = CompletionRankingModel(map.value(clangdCompletionRankingModelKey(),
                                                              int(completionRankingModel)).toInt());
    autoIncludeHeaders = map.value(clangdHeaderInsertionKey(), false).toBool();
    workerThreadLimit = map.value(clangdThreadLimitKey(), 0).toInt();
    documentUpdateThreshold = map.value(clangdDocumentThresholdKey(), 500).toInt();
    sizeThresholdEnabled = map.value(clangdSizeThresholdEnabledKey(), false).toBool();
    sizeThresholdInKb = map.value(clangdSizeThresholdKey(), 1024).toLongLong();
    sessionsWithOneClangd = map.value(sessionsWithOneClangdKey()).toStringList();
    diagnosticConfigId = Id::fromSetting(map.value(diagnosticConfigIdKey(),
                                                   initialClangDiagnosticConfigId().toSetting()));
    haveCheckedHardwareReqirements = map.value(checkedHardwareKey(), false).toBool();
    updateDependentSources = map.value(updateDependentSourcesKey(), false).toBool();
    completionResults = map.value(completionResultsKey(), defaultCompletionResults()).toInt();
}

int ClangdSettings::Data::defaultCompletionResults()
{
    // Default clangd --limit-results value is 100
    bool ok = false;
    const int userValue = qtcEnvironmentVariableIntValue("QTC_CLANGD_COMPLETION_RESULTS", &ok);
    return ok ? userValue : 100;
}

namespace Internal {
class ClangdSettingsWidget final : public QWidget
{
    Q_OBJECT

public:
    ClangdSettingsWidget(const ClangdSettings::Data &settingsData, bool isForProject);

    ClangdSettings::Data settingsData() const;

signals:
    void settingsDataChanged();

private:
    QCheckBox m_useClangdCheckBox;
    QComboBox m_indexingComboBox;
    Utils::FancyLineEdit m_projectIndexPathTemplateLineEdit;
    Utils::FancyLineEdit m_sessionIndexPathTemplateLineEdit;
    QComboBox m_headerSourceSwitchComboBox;
    QComboBox m_completionRankingModelComboBox;
    QCheckBox m_autoIncludeHeadersCheckBox;
    QCheckBox m_updateDependentSourcesCheckBox;
    QCheckBox m_sizeThresholdCheckBox;
    QSpinBox m_threadLimitSpinBox;
    QSpinBox m_documentUpdateThreshold;
    QSpinBox m_sizeThresholdSpinBox;
    QSpinBox m_completionResults;
    Utils::PathChooser m_clangdChooser;
    Utils::InfoLabel m_versionWarningLabel;
    ClangDiagnosticConfigsSelectionWidget *m_configSelectionWidget = nullptr;
    QGroupBox *m_sessionsGroupBox = nullptr;
    QStringListModel m_sessionsModel;
};

ClangdSettingsWidget::ClangdSettingsWidget(const ClangdSettings::Data &settingsData,
                                           bool isForProject)
{
    const ClangdSettings settings(settingsData);
    const QString indexingToolTip = Tr::tr(
        "<p>If background indexing is enabled, global symbol searches will yield more accurate "
        "results, at the cost of additional CPU load when the project is first opened. The "
        "indexing result is persisted in the project's build directory. If you disable background "
        "indexing, a faster, but less accurate, built-in indexer is used instead. The thread "
        "priority for building the background index can be adjusted since clangd 15.</p>"
        "<p>Background Priority: Minimum priority, runs on idle CPUs. May leave 'performance' "
        "cores unused.</p>"
        "<p>Normal Priority: Reduced priority compared to interactive work.</p>"
        "<p>Low Priority: Same priority as other clangd work.</p>");
    const QString projectIndexPathToolTip = Tr::tr(
        "The location of the per-project clangd index.<p>"
        "This is also where the compile_commands.json file will go.");
    const QString sessionIndexPathToolTip = Tr::tr(
        "The location of the per-session clangd index.<p>"
        "This is also where the compile_commands.json file will go.");
    const QString headerSourceSwitchToolTip = Tr::tr(
        "<p>The C/C++ backend to use for switching between header and source files.</p>"
        "<p>While the clangd implementation has more capabilities than the built-in "
        "code model, it tends to find false positives.</p>"
        "<p>When \"Try Both\" is selected, clangd is used only if the built-in variant "
        "does not find anything.</p>");
    using RankingModel = ClangdSettings::CompletionRankingModel;
    const QString completionRankingModelToolTip = Tr::tr(
                                                      "<p>Which model clangd should use to rank possible completions.</p>"
                                                      "<p>This determines the order of candidates in the combo box when doing code completion.</p>"
                                                      "<p>The \"%1\" model used by default results from (pre-trained) machine learning and "
                                                      "provides superior results on average.</p>"
                                                      "<p>If you feel that its suggestions stray too much from your expectations for your "
                                                      "code base, you can try switching to the hand-crafted \"%2\" model.</p>").arg(
                                                          ClangdSettings::rankingModelToDisplayString(RankingModel::DecisionForest),
                                                          ClangdSettings::rankingModelToDisplayString(RankingModel::Heuristics));
    const QString workerThreadsToolTip = Tr::tr(
        "Number of worker threads used by clangd. Background indexing also uses this many "
        "worker threads.");
    const QString autoIncludeToolTip = Tr::tr(
        "Controls whether clangd may insert header files as part of symbol completion.");
    const QString updateDependentSourcesToolTip = Tr::tr(
        "<p>Controls whether when editing a header file, clangd should re-parse all source files "
        "including that header.</p>"
        "<p>Note that enabling this option can cause considerable CPU load when editing widely "
        "included headers.</p>"
        "<p>If this option is disabled, the dependent source files are only re-parsed when the "
        "header file is saved.</p>");
    const QString documentUpdateToolTip
        //: %1 is the application name (Qt Creator)
        = Tr::tr("Defines the amount of time %1 waits before sending document changes to the "
                 "server.\n"
                 "If the document changes again while waiting, this timeout resets.")
              .arg(QGuiApplication::applicationDisplayName());
    const QString sizeThresholdToolTip = Tr::tr(
        "Files greater than this will not be opened as documents in clangd.\n"
        "The built-in code model will handle highlighting, completion and so on.");
    const QString completionResultToolTip = Tr::tr(
        "The maximum number of completion results returned by clangd.");

    m_useClangdCheckBox.setText(Tr::tr("Use clangd"));
    m_useClangdCheckBox.setChecked(settings.useClangd());
    m_clangdChooser.setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_clangdChooser.setFilePath(settings.clangdFilePath());
    m_clangdChooser.setAllowPathFromDevice(true);
    m_clangdChooser.setEnabled(m_useClangdCheckBox.isChecked());
    m_clangdChooser.setCommandVersionArguments({"--version"});
    using Priority = ClangdSettings::IndexingPriority;
    for (Priority prio : {Priority::Off, Priority::Background, Priority::Low, Priority::Normal}) {
        m_indexingComboBox.addItem(ClangdSettings::priorityToDisplayString(prio), int(prio));
        if (prio == settings.indexingPriority())
            m_indexingComboBox.setCurrentIndex(m_indexingComboBox.count() - 1);
    }
    m_indexingComboBox.setToolTip(indexingToolTip);
    m_projectIndexPathTemplateLineEdit.setText(settings.data().projectIndexPathTemplate);
    m_sessionIndexPathTemplateLineEdit.setText(settings.data().sessionIndexPathTemplate);
    using SwitchMode = ClangdSettings::HeaderSourceSwitchMode;
    for (SwitchMode mode : {SwitchMode::BuiltinOnly, SwitchMode::ClangdOnly, SwitchMode::Both}) {
        m_headerSourceSwitchComboBox.addItem(
            ClangdSettings::headerSourceSwitchModeToDisplayString(mode), int(mode));
        if (mode == settings.headerSourceSwitchMode())
            m_headerSourceSwitchComboBox.setCurrentIndex(
                m_headerSourceSwitchComboBox.count() - 1);
    }
    m_headerSourceSwitchComboBox.setToolTip(headerSourceSwitchToolTip);
    for (RankingModel model : {RankingModel::Default, RankingModel::DecisionForest,
                               RankingModel::Heuristics}) {
        m_completionRankingModelComboBox.addItem(
            ClangdSettings::rankingModelToDisplayString(model), int(model));
        if (model == settings.completionRankingModel())
            m_completionRankingModelComboBox.setCurrentIndex(
                m_completionRankingModelComboBox.count() - 1);
    }
    m_completionRankingModelComboBox.setToolTip(completionRankingModelToolTip);

    m_autoIncludeHeadersCheckBox.setText(Tr::tr("Insert header files on completion"));
    m_autoIncludeHeadersCheckBox.setChecked(settings.autoIncludeHeaders());
    m_autoIncludeHeadersCheckBox.setToolTip(autoIncludeToolTip);
    m_updateDependentSourcesCheckBox.setText(Tr::tr("Update dependent sources"));
    m_updateDependentSourcesCheckBox.setChecked(settings.updateDependentSources());
    m_updateDependentSourcesCheckBox.setToolTip(updateDependentSourcesToolTip);
    m_threadLimitSpinBox.setValue(settings.workerThreadLimit());
    m_threadLimitSpinBox.setSpecialValueText(Tr::tr("Automatic"));
    m_threadLimitSpinBox.setToolTip(workerThreadsToolTip);
    m_documentUpdateThreshold.setMinimum(50);
    m_documentUpdateThreshold.setMaximum(10000);
    m_documentUpdateThreshold.setValue(settings.documentUpdateThreshold());
    m_documentUpdateThreshold.setSingleStep(100);
    m_documentUpdateThreshold.setSuffix(" ms");
    m_documentUpdateThreshold.setToolTip(documentUpdateToolTip);
    m_sizeThresholdCheckBox.setText(Tr::tr("Ignore files greater than"));
    m_sizeThresholdCheckBox.setChecked(settings.sizeThresholdEnabled());
    m_sizeThresholdCheckBox.setToolTip(sizeThresholdToolTip);
    m_sizeThresholdSpinBox.setMinimum(1);
    m_sizeThresholdSpinBox.setMaximum(std::numeric_limits<int>::max());
    m_sizeThresholdSpinBox.setSuffix(" KB");
    m_sizeThresholdSpinBox.setValue(settings.sizeThresholdInKb());
    m_sizeThresholdSpinBox.setToolTip(sizeThresholdToolTip);

    const auto completionResultsLabel = new QLabel(Tr::tr("Completion results:"));
    completionResultsLabel->setToolTip(completionResultToolTip);
    m_completionResults.setMinimum(0);
    m_completionResults.setMaximum(std::numeric_limits<int>::max());
    m_completionResults.setValue(settings.completionResults());
    m_completionResults.setToolTip(completionResultToolTip);
    m_completionResults.setSpecialValueText(Tr::tr("No limit"));

    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(&m_useClangdCheckBox);

    const auto formLayout = new QFormLayout;
    const auto chooserLabel = new QLabel(Tr::tr("Path to executable:"));
    formLayout->addRow(chooserLabel, &m_clangdChooser);
    formLayout->addRow(QString(), &m_versionWarningLabel);

    const auto indexingPriorityLayout = new QHBoxLayout;
    indexingPriorityLayout->addWidget(&m_indexingComboBox);
    indexingPriorityLayout->addStretch(1);
    const auto indexingPriorityLabel = new QLabel(Tr::tr("Background indexing:"));
    indexingPriorityLabel->setToolTip(indexingToolTip);
    formLayout->addRow(indexingPriorityLabel, indexingPriorityLayout);

    for (const auto &[text, edit, toolTip, defaultValue] :
         {std::make_tuple(Tr::tr("Per-project index location:"),
                          &m_projectIndexPathTemplateLineEdit,
                          projectIndexPathToolTip,
                          ClangdSettings::defaultProjectIndexPathTemplate()),
          std::make_tuple(Tr::tr("Per-session index location:"),
                          &m_sessionIndexPathTemplateLineEdit,
                          sessionIndexPathToolTip,
                          ClangdSettings::defaultSessionIndexPathTemplate())}) {
        if (isForProject && edit == &m_sessionIndexPathTemplateLineEdit)
            continue;

        const auto chooser = new Utils::VariableChooser(edit);
        chooser->addSupportedWidget(edit);
        chooser->addMacroExpanderProvider([] { return Utils::globalMacroExpander(); });

        const auto resetButton = new QPushButton(Tr::tr("Reset"));
        connect(resetButton, &QPushButton::clicked, [e = edit, v = defaultValue] { e->setText(v); });
        const auto layout = new QHBoxLayout;
        const auto label = new QLabel(text);
        label->setToolTip(toolTip);
        edit->setToolTip(toolTip);
        layout->addWidget(edit);
        layout->addWidget(resetButton);
        formLayout->addRow(label, layout);
    }

    const auto headerSourceSwitchLayout = new QHBoxLayout;
    headerSourceSwitchLayout->addWidget(&m_headerSourceSwitchComboBox);
    headerSourceSwitchLayout->addStretch(1);
    const auto headerSourceSwitchLabel = new QLabel(Tr::tr("Header/source switch mode:"));
    headerSourceSwitchLabel->setToolTip(headerSourceSwitchToolTip);
    formLayout->addRow(headerSourceSwitchLabel, headerSourceSwitchLayout);

    const auto threadLimitLayout = new QHBoxLayout;
    threadLimitLayout->addWidget(&m_threadLimitSpinBox);
    threadLimitLayout->addStretch(1);
    const auto threadLimitLabel = new QLabel(Tr::tr("Worker thread count:"));
    threadLimitLabel->setToolTip(workerThreadsToolTip);
    formLayout->addRow(threadLimitLabel, threadLimitLayout);

    formLayout->addRow(QString(), &m_autoIncludeHeadersCheckBox);
    formLayout->addRow(QString(), &m_updateDependentSourcesCheckBox);
    const auto limitResultsLayout = new QHBoxLayout;
    limitResultsLayout->addWidget(&m_completionResults);
    limitResultsLayout->addStretch(1);
    formLayout->addRow(completionResultsLabel, limitResultsLayout);

    const auto completionRankingModelLayout = new QHBoxLayout;
    completionRankingModelLayout->addWidget(&m_completionRankingModelComboBox);
    completionRankingModelLayout->addStretch(1);
    const auto completionRankingModelLabel = new QLabel(Tr::tr("Completion ranking model:"));
    completionRankingModelLabel->setToolTip(completionRankingModelToolTip);
    formLayout->addRow(completionRankingModelLabel, completionRankingModelLayout);

    const auto documentUpdateThresholdLayout = new QHBoxLayout;
    documentUpdateThresholdLayout->addWidget(&m_documentUpdateThreshold);
    documentUpdateThresholdLayout->addStretch(1);
    const auto documentUpdateThresholdLabel = new QLabel(Tr::tr("Document update threshold:"));
    documentUpdateThresholdLabel->setToolTip(documentUpdateToolTip);
    formLayout->addRow(documentUpdateThresholdLabel, documentUpdateThresholdLayout);
    const auto sizeThresholdLayout = new QHBoxLayout;
    sizeThresholdLayout->addWidget(&m_sizeThresholdSpinBox);
    sizeThresholdLayout->addStretch(1);
    formLayout->addRow(&m_sizeThresholdCheckBox, sizeThresholdLayout);

    m_configSelectionWidget = new ClangDiagnosticConfigsSelectionWidget(formLayout);
    m_configSelectionWidget->refresh(
        ClangdSettings::diagnosticConfigsModel(),
        settings.diagnosticConfigId(),
        [](const ClangDiagnosticConfigs &configs, const Utils::Id &configToSelect) {
            return new CppEditor::ClangDiagnosticConfigsWidget(configs, configToSelect);
        });

    layout->addLayout(formLayout);
    if (!isForProject) {
        m_sessionsModel.setStringList(settingsData.sessionsWithOneClangd);
        m_sessionsModel.sort(0);
        m_sessionsGroupBox = new QGroupBox(Tr::tr("Sessions with a Single Clangd Instance"));
        const auto sessionsView = new Utils::ListView;
        sessionsView->setModel(&m_sessionsModel);
        sessionsView->setToolTip(
            Tr::tr("By default, Qt Creator runs one clangd process per project.\n"
                   "If you have sessions with tightly coupled projects that should be\n"
                   "managed by the same clangd process, add them here."));
        const auto outerSessionsLayout = new QHBoxLayout;
        const auto innerSessionsLayout = new QHBoxLayout(m_sessionsGroupBox);
        const auto buttonsLayout = new QVBoxLayout;
        const auto addButton = new QPushButton(Tr::tr("Add ..."));
        const auto removeButton = new QPushButton(Tr::tr("Remove"));
        buttonsLayout->addWidget(addButton);
        buttonsLayout->addWidget(removeButton);
        buttonsLayout->addStretch(1);
        innerSessionsLayout->addWidget(sessionsView);
        innerSessionsLayout->addLayout(buttonsLayout);
        outerSessionsLayout->addWidget(m_sessionsGroupBox);
        outerSessionsLayout->addStretch(1);

        const auto separator = new QFrame;
        separator->setFrameShape(QFrame::HLine);
        layout->addWidget(separator);
        layout->addLayout(outerSessionsLayout);

        const auto updateRemoveButtonState = [removeButton, sessionsView] {
            removeButton->setEnabled(sessionsView->selectionModel()->hasSelection());
        };
        connect(sessionsView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, updateRemoveButtonState);
        updateRemoveButtonState();
        connect(removeButton, &QPushButton::clicked, this, [this, sessionsView] {
            const QItemSelection selection = sessionsView->selectionModel()->selection();
            QTC_ASSERT(!selection.isEmpty(), return);
            m_sessionsModel.removeRow(selection.indexes().first().row());
        });

        connect(addButton, &QPushButton::clicked, this, [this, sessionsView] {
            QInputDialog dlg(sessionsView);
            QStringList sessions = Core::SessionManager::sessions();
            QStringList currentSessions = m_sessionsModel.stringList();
            for (const QString &s : std::as_const(currentSessions))
                sessions.removeOne(s);
            if (sessions.isEmpty())
                return;
            sessions.sort();
            dlg.setLabelText(Tr::tr("Choose a session:"));
            dlg.setComboBoxItems(sessions);
            if (dlg.exec() == QDialog::Accepted) {
                currentSessions << dlg.textValue();
                m_sessionsModel.setStringList(currentSessions);
                m_sessionsModel.sort(0);
            }
        });
    }

    const auto configFilesHelpLabel = new QLabel;
    configFilesHelpLabel->setText(Tr::tr("Additional settings are available via "
                                         "<a href=\"https://clangd.llvm.org/config\"> clangd configuration files</a>.<br>"
                                         "User-specific settings go <a href=\"%1\">here</a>, "
                                         "project-specific settings can be configured by putting a .clangd file into "
                                         "the project source tree.")
                                      .arg(ClangdSettings::clangdUserConfigFilePath().toUserOutput()));
    configFilesHelpLabel->setWordWrap(true);
    connect(configFilesHelpLabel, &QLabel::linkHovered, configFilesHelpLabel, &QLabel::setToolTip);
    connect(configFilesHelpLabel, &QLabel::linkActivated, [](const QString &link) {
        if (link.startsWith("https"))
            QDesktopServices::openUrl(link);
        else
            Core::EditorManager::openEditor(Utils::FilePath::fromString(link));
    });
    layout->addWidget(Layouting::createHr());
    layout->addWidget(configFilesHelpLabel);

    layout->addStretch(1);

    static const auto setWidgetsEnabled = [](QLayout *layout, bool enabled, const auto &f) -> void {
        for (int i = 0; i < layout->count(); ++i) {
            if (QWidget * const w = layout->itemAt(i)->widget())
                w->setEnabled(enabled);
            else if (QLayout * const l = layout->itemAt(i)->layout())
                f(l, enabled, f);
        }
    };
    const auto toggleEnabled = [this, formLayout](const bool checked) {
        setWidgetsEnabled(formLayout, checked, setWidgetsEnabled);
        if (m_sessionsGroupBox)
            m_sessionsGroupBox->setEnabled(checked);
    };
    connect(&m_useClangdCheckBox, &QCheckBox::toggled, toggleEnabled);
    toggleEnabled(m_useClangdCheckBox.isChecked());
    m_threadLimitSpinBox.setEnabled(m_useClangdCheckBox.isChecked());

    m_versionWarningLabel.setType(Utils::InfoLabel::Warning);
    const auto updateWarningLabel = [this] {
        class WarningLabelSetter {
        public:
            WarningLabelSetter(QLabel &label) : m_label(label) { m_label.clear(); }
            ~WarningLabelSetter() { m_label.setVisible(!m_label.text().isEmpty()); }
            void setWarning(const QString &text) { m_label.setText(text); }
        private:
            QLabel &m_label;
        };
        WarningLabelSetter labelSetter(m_versionWarningLabel);

        if (!m_clangdChooser.isValid())
            return;
        const FilePath clangdPath = m_clangdChooser.filePath();
        if (Result<> res = Utils::checkClangdVersion(clangdPath); !res)
            labelSetter.setWarning(res.error());
    };
    connect(&m_clangdChooser, &Utils::PathChooser::textChanged, this, updateWarningLabel);
    connect(&m_clangdChooser, &Utils::PathChooser::validChanged, this, updateWarningLabel);
    updateWarningLabel();

    connect(&m_useClangdCheckBox, &QCheckBox::toggled,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&m_indexingComboBox, &QComboBox::currentIndexChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&m_projectIndexPathTemplateLineEdit, &QLineEdit::textChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&m_sessionIndexPathTemplateLineEdit, &QLineEdit::textChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&m_headerSourceSwitchComboBox, &QComboBox::currentIndexChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&m_completionRankingModelComboBox, &QComboBox::currentIndexChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&m_autoIncludeHeadersCheckBox, &QCheckBox::toggled,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&m_updateDependentSourcesCheckBox, &QCheckBox::toggled,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&m_threadLimitSpinBox, &QSpinBox::valueChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&m_sizeThresholdCheckBox, &QCheckBox::toggled,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&m_sizeThresholdSpinBox, &QSpinBox::valueChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&m_documentUpdateThreshold, &QSpinBox::valueChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&m_clangdChooser, &Utils::PathChooser::textChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(m_configSelectionWidget, &ClangDiagnosticConfigsSelectionWidget::changed,
            this, &ClangdSettingsWidget::settingsDataChanged);
    connect(&m_completionResults, &QSpinBox::valueChanged,
            this, &ClangdSettingsWidget::settingsDataChanged);
}

ClangdSettings::Data ClangdSettingsWidget::settingsData() const
{
    ClangdSettings::Data data;
    data.useClangd = m_useClangdCheckBox.isChecked();
    data.executableFilePath = m_clangdChooser.filePath();
    data.indexingPriority = ClangdSettings::IndexingPriority(
        m_indexingComboBox.currentData().toInt());
    data.projectIndexPathTemplate = m_projectIndexPathTemplateLineEdit.text();
    data.sessionIndexPathTemplate = m_sessionIndexPathTemplateLineEdit.text();
    data.headerSourceSwitchMode = ClangdSettings::HeaderSourceSwitchMode(
        m_headerSourceSwitchComboBox.currentData().toInt());
    data.completionRankingModel = ClangdSettings::CompletionRankingModel(
        m_completionRankingModelComboBox.currentData().toInt());
    data.autoIncludeHeaders = m_autoIncludeHeadersCheckBox.isChecked();
    data.updateDependentSources = m_updateDependentSourcesCheckBox.isChecked();
    data.workerThreadLimit = m_threadLimitSpinBox.value();
    data.documentUpdateThreshold = m_documentUpdateThreshold.value();
    data.sizeThresholdEnabled = m_sizeThresholdCheckBox.isChecked();
    data.sizeThresholdInKb = m_sizeThresholdSpinBox.value();
    data.sessionsWithOneClangd = m_sessionsModel.stringList();
    data.customDiagnosticConfigs = m_configSelectionWidget->customConfigs();
    data.diagnosticConfigId = m_configSelectionWidget->currentConfigId();
    data.completionResults = m_completionResults.value();
    return data;
}

class ClangdSettingsPageWidget final : public Core::IOptionsPageWidget
{
public:
    ClangdSettingsPageWidget() : m_widget(ClangdSettings::instance().data(), false)
    {
        const auto layout = new QVBoxLayout(this);
        layout->addWidget(&m_widget);
    }

private:
    void apply() final { ClangdSettings::instance().setData(m_widget.settingsData()); }

    ClangdSettingsWidget m_widget;
};

class ClangdSettingsPage final : public Core::IOptionsPage
{
public:
    ClangdSettingsPage()
    {
        setId(Constants::CPP_CLANGD_SETTINGS_ID);
        setDisplayName(Tr::tr("Clangd"));
        setCategory(Constants::CPP_SETTINGS_CATEGORY);
        setWidgetCreator([] { return new ClangdSettingsPageWidget; });
    }
};

void setupClangdSettingsPage()
{
    static ClangdSettingsPage theClangdSettingsPage;
}

class ClangdProjectSettingsWidget : public ProjectSettingsWidget
{
public:
    ClangdProjectSettingsWidget(const ClangdProjectSettings &settings)
        : m_settings(settings), m_widget(settings.settings(), true)
    {
        setGlobalSettingsId(Constants::CPP_CLANGD_SETTINGS_ID);
        const auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(&m_widget);

        const auto updateGlobalSettingsCheckBox = [this] {
            if (ClangdSettings::instance().granularity() == ClangdSettings::Granularity::Session) {
                setUseGlobalSettingsCheckBoxEnabled(false);
                setUseGlobalSettings(true);
            } else {
                setUseGlobalSettingsCheckBoxEnabled(true);
                setUseGlobalSettings(m_settings.useGlobalSettings());
            }
            m_widget.setEnabled(!useGlobalSettings());
        };

        updateGlobalSettingsCheckBox();
        connect(&ClangdSettings::instance(), &ClangdSettings::changed,
                this, updateGlobalSettingsCheckBox);

        connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged, this,
                [this](bool checked) {
                    m_widget.setEnabled(!checked);
                    m_settings.setUseGlobalSettings(checked);
                    if (!checked)
                        m_settings.setSettings(m_widget.settingsData());
                });

        const auto timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(5000);
        connect(timer, &QTimer::timeout, this, [this] {
            m_settings.setSettings(m_widget.settingsData());
        });
        connect(&m_widget, &ClangdSettingsWidget::settingsDataChanged,
                timer, qOverload<>(&QTimer::start));
    }

private:
    ClangdProjectSettings m_settings;
    ClangdSettingsWidget m_widget;
};

class ClangdProjectSettingsPanelFactory final : public ProjectPanelFactory
{
public:
    ClangdProjectSettingsPanelFactory()
    {
        setPriority(100);
        setDisplayName(Tr::tr("Clangd"));
        setCreateWidgetFunction([](Project *project) {
            return new ClangdProjectSettingsWidget(project);
        });
    }
};

void setupClangdProjectSettingsPanel()
{
    static ClangdProjectSettingsPanelFactory theClangdProjectSettingsPanelFactory;
}

} // namespace Internal
} // namespace CppEditor

#include <clangdsettings.moc>
