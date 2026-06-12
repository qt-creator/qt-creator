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
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectsettings.h>
#include <projectexplorer/useglobalaspect.h>

#include <utils/clangutils.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/qtcprocess.h>

#include <QDesktopServices>
#include <QGroupBox>
#include <QGuiApplication>
#include <QInputDialog>
#include <QPushButton>
#include <QStandardPaths>
#include <QStringListModel>
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
static Key clangdCompletionStyleKey() { return "ClangdCompletionStyle"; }
static Key clangdHeaderInsertionKey() { return "ClangdHeaderInsertion"; }
static Key clangdThreadLimitKey() { return "ClangdThreadLimit"; }
static Key clangdDocumentThresholdKey() { return "ClangdDocumentThreshold"; }
static Key clangdSizeThresholdEnabledKey() { return "ClangdSizeThresholdEnabled"; }
static Key clangdSizeThresholdKey() { return "ClangdSizeThreshold"; }
static Key useGlobalSettingsKey() { return "useGlobalSettings"; }
static Key sessionsWithOneClangdKey() { return "SessionsWithOneClangd"; }
static Key diagnosticConfigIdKey() { return "diagnosticConfigId"; }
static Key checkedHardwareKey() { return "checkedHardware"; }
static Key completionResultsKey() { return "completionResults"; }
static Key updateDependentSourcesKey() { return "updateDependentSources"; }
static Key useExternalCompilationDbKey() { return "ClangdUseExternalCompilationDb"; }

const char blockProjectIndexingProperty[] = "ClangBlockProjectIndexing";

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

QString ClangdSettings::completionStyleToCmdLineString(CompletionStyle style)
{
    switch (style) {
    case CompletionStyle::Default: break;
    case CompletionStyle::Detailed: return "detailed";
    case CompletionStyle::Bundled: return "bundled";
    }
    QTC_ASSERT(false, return {});
}

QString ClangdSettings::completionStyleToDisplayString(CompletionStyle style)
{
    switch (style) {
    case CompletionStyle::Default: return Tr::tr("Default");
    case CompletionStyle::Detailed: return Tr::tr("Detailed");
    case CompletionStyle::Bundled: return Tr::tr("Bundled");
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

static Layouting::Layout clangdSettingsLayout(ClangdSettings *s)
{
    auto *versionWarning = new InfoLabel;
    versionWarning->setType(InfoLabel::Warning);
    versionWarning->setVisible(false);
    const auto updateWarning = [s, versionWarning] {
        const FilePath path = s->clangdPath();
        if (path.isEmpty()) {
            versionWarning->setVisible(false);
            return;
        }
        const Result<> res = checkClangdVersion(path);
        versionWarning->setVisible(!res);
        if (!res)
            versionWarning->setText(res.error());
    };
    s->clangdPath.addOnChanged(versionWarning, updateWarning);
    updateWarning();

    using namespace Layouting;
    // clang-format off
    return Column {
        s->useClangd,
        Form {
            Tr::tr("Path to executable:"),
                s->clangdPath, br,
            empty, versionWarning, br,
            Tr::tr("Background indexing:"),
                Row { s->indexingPriority, st }, br,
            Tr::tr("Per-project index location:"),
                s->projectIndexPathTemplate, br,
            Tr::tr("Per-session index location:"),
                s->sessionIndexPathTemplate, br,
            Tr::tr("Header/source switch mode:"),
                Row { s->headerSourceSwitchMode, st }, br,
            Tr::tr("Worker thread count:"),
                Row { s->workerThreadLimit, st }, br,
            empty, s->autoIncludeHeaders, br,
            empty, s->updateDependentSources, br,
            empty, s->useExternalCompilationDb, br,
            Tr::tr("Completion results:"),
                Row { s->completionResults, st }, br,
            Tr::tr("Completion ranking model:"),
                Row { s->completionRankingModel, st }, br,
            Tr::tr("Completion style:"),
                Row { s->completionStyle, st }, br,
            Tr::tr("Document update threshold:"),
                Row { s->documentUpdateThreshold, st }, br,
            s->sizeThresholdEnabled,
                Row { s->sizeThresholdInKb, st }, br,
        },
        s->diagnosticConfigId,
        noMargin,
    };
    // clang-format on
}

ClangdSettings::ClangdSettings()
{
    setSettingsGroup("ClangdSettings");

    useClangd.setSettingsKey(useClangdKey());
    useClangd.setDefaultValue(true);
    useClangd.setLabelText(Tr::tr("Use clangd"));

    clangdPath.setSettingsKey(clangdPathKey());
    clangdPath.setExpectedKind(PathChooser::ExistingCommand);
    clangdPath.setAllowPathFromDevice(true);
    clangdPath.setCommandVersionArguments({"--version"});

    autoIncludeHeaders.setSettingsKey(clangdHeaderInsertionKey());
    autoIncludeHeaders.setDefaultValue(false);
    autoIncludeHeaders.setLabelText(Tr::tr("Insert header files on completion"));
    autoIncludeHeaders.setToolTip(
        Tr::tr("Controls whether clangd may insert header files as part of symbol completion."));

    sizeThresholdEnabled.setSettingsKey(clangdSizeThresholdEnabledKey());
    sizeThresholdEnabled.setDefaultValue(false);
    sizeThresholdEnabled.setLabelText(Tr::tr("Ignore files greater than"));

    updateDependentSources.setSettingsKey(updateDependentSourcesKey());
    updateDependentSources.setDefaultValue(false);
    updateDependentSources.setLabelText(Tr::tr("Update dependent sources"));
    updateDependentSources.setToolTip(Tr::tr(
        "<p>Controls whether when editing a header file, clangd should re-parse all source files "
        "including that header.</p>"
        "<p>Note that enabling this option can cause considerable CPU load when editing widely "
        "included headers.</p>"
        "<p>If this option is disabled, the dependent source files are only re-parsed when the "
        "header file is saved.</p>"));

    useExternalCompilationDb.setSettingsKey(useExternalCompilationDbKey());
    useExternalCompilationDb.setDefaultValue(false);
    useExternalCompilationDb.setLabelText(
        Tr::tr("Use externally provided compilation database"));
    useExternalCompilationDb.setToolTip(Tr::tr(
        "<p>Controls whether clangd will use an existing compile_commands.json file, rather than "
        "one set up by Qt Creator, which is the default.</p>"
        "<p>When enabling this option, the user is responsible for providing a suitable file at "
        "the index location specified above, as well as for keeping that file in sync with the "
        "project state.</p>"));

    workerThreadLimit.setSettingsKey(clangdThreadLimitKey());
    workerThreadLimit.setDefaultValue(0);
    workerThreadLimit.setSpecialValueText(Tr::tr("Automatic"));
    workerThreadLimit.setToolTip(Tr::tr(
        "Number of worker threads used by clangd. Background indexing also uses this many "
        "worker threads."));

    documentUpdateThreshold.setSettingsKey(clangdDocumentThresholdKey());
    documentUpdateThreshold.setDefaultValue(500);
    documentUpdateThreshold.setRange(50, 10000);
    documentUpdateThreshold.setSingleStep(100);
    documentUpdateThreshold.setSuffix(" ms");
    documentUpdateThreshold.setToolTip(
        //: %1 is the application name (Qt Creator)
        Tr::tr("Defines the amount of time %1 waits before sending document changes to the "
               "server.\n"
               "If the document changes again while waiting, this timeout resets.")
            .arg(QGuiApplication::applicationDisplayName()));

    sizeThresholdInKb.setSettingsKey(clangdSizeThresholdKey());
    sizeThresholdInKb.setDefaultValue(1024);
    sizeThresholdInKb.setRange(1, std::numeric_limits<int>::max());
    sizeThresholdInKb.setSuffix(" KB");

    const QString sizeThresholdToolTip = Tr::tr(
        "Files greater than this will not be opened as documents in clangd.\n"
        "The built-in code model will handle highlighting, completion and so on.");
    sizeThresholdEnabled.setToolTip(sizeThresholdToolTip);
    sizeThresholdInKb.setToolTip(sizeThresholdToolTip);

    completionResults.setSettingsKey(completionResultsKey());
    completionResults.setDefaultValue(Data::defaultCompletionResults());
    completionResults.setRange(0, std::numeric_limits<int>::max());
    completionResults.setSpecialValueText(Tr::tr("No limit"));
    completionResults.setToolTip(
        Tr::tr("The maximum number of completion results returned by clangd."));

    projectIndexPathTemplate.setSettingsKey(clangdProjectIndexPathKey());
    projectIndexPathTemplate.setDefaultValue(defaultProjectIndexPathTemplate());
    projectIndexPathTemplate.setDisplayStyle(StringAspect::LineEditDisplay);
    projectIndexPathTemplate.setUseResetButton();
    projectIndexPathTemplate.setToolTip(
        Tr::tr("The location of the per-project clangd index.<p>"
               "This is also where the compile_commands.json file will go."));

    sessionIndexPathTemplate.setSettingsKey(clangdSessionIndexPathKey());
    sessionIndexPathTemplate.setDefaultValue(defaultSessionIndexPathTemplate());
    sessionIndexPathTemplate.setDisplayStyle(StringAspect::LineEditDisplay);
    sessionIndexPathTemplate.setUseResetButton();
    sessionIndexPathTemplate.setToolTip(
        Tr::tr("The location of the per-session clangd index.<p>"
               "This is also where the compile_commands.json file will go."));

    // IndexingPriority: options added in enum-value order so stored int == index
    using Priority = IndexingPriority;
    indexingPriority.setSettingsKey(clangdIndexingPriorityKey());
    indexingPriority.setDefaultValue(IndexingPriority::Low);
    indexingPriority.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    for (Priority p : {Priority::Off, Priority::Background, Priority::Normal, Priority::Low})
        indexingPriority.addOption(priorityToDisplayString(p));
    indexingPriority.setToolTip(Tr::tr(
        "<p>If background indexing is enabled, global symbol searches will yield more accurate "
        "results, at the cost of additional CPU load when the project is first opened. The "
        "indexing result is persisted in the project's build directory. If you disable background "
        "indexing, a faster, but less accurate, built-in indexer is used instead. The thread "
        "priority for building the background index can be adjusted since clangd 15.</p>"
        "<p>Background Priority: Minimum priority, runs on idle CPUs. May leave 'performance' "
        "cores unused.</p>"
        "<p>Normal Priority: Reduced priority compared to interactive work.</p>"
        "<p>Low Priority: Same priority as other clangd work.</p>"));

    using SwitchMode = HeaderSourceSwitchMode;
    headerSourceSwitchMode.setSettingsKey(clangdHeaderSourceSwitchModeKey());
    headerSourceSwitchMode.setDefaultValue(HeaderSourceSwitchMode::Both);
    headerSourceSwitchMode.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    for (SwitchMode m : {SwitchMode::BuiltinOnly, SwitchMode::ClangdOnly, SwitchMode::Both})
        headerSourceSwitchMode.addOption(headerSourceSwitchModeToDisplayString(m));
    headerSourceSwitchMode.setToolTip(Tr::tr(
        "<p>The C/C++ backend to use for switching between header and source files.</p>"
        "<p>While the clangd implementation has more capabilities than the built-in "
        "code model, it tends to find false positives.</p>"
        "<p>When \"Try Both\" is selected, clangd is used only if the built-in variant "
        "does not find anything.</p>"));

    using RankingModel = CompletionRankingModel;
    completionRankingModel.setSettingsKey(clangdCompletionRankingModelKey());
    completionRankingModel.setDefaultValue(CompletionRankingModel::Default);
    completionRankingModel.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    for (RankingModel m : {RankingModel::Default, RankingModel::DecisionForest,
                           RankingModel::Heuristics})
        completionRankingModel.addOption(rankingModelToDisplayString(m));
    completionRankingModel.setToolTip(
        Tr::tr("<p>Which model clangd should use to rank possible completions.</p>"
               "<p>This determines the order of candidates in the combo box when doing code "
               "completion.</p>"
               "<p>The \"%1\" model used by default results from (pre-trained) machine learning "
               "and provides superior results on average.</p>"
               "<p>If you feel that its suggestions stray too much from your expectations for "
               "your code base, you can try switching to the hand-crafted \"%2\" model.</p>")
            .arg(rankingModelToDisplayString(RankingModel::DecisionForest),
                 rankingModelToDisplayString(RankingModel::Heuristics)));

    using Style = CompletionStyle;
    completionStyle.setSettingsKey(clangdCompletionStyleKey());
    completionStyle.setDefaultValue(CompletionStyle::Default);
    completionStyle.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    for (Style s : {Style::Default, Style::Detailed, Style::Bundled})
        completionStyle.addOption(completionStyleToDisplayString(s));
    completionStyle.setToolTip(
        Tr::tr("<p>Which granularity to use for completion items.</p>"
               "<p>Determines whether to use one item per overload or bundle them "
               "together.</p>"));

    diagnosticConfigId.setSettingsKey(diagnosticConfigIdKey());
    diagnosticConfigId.setDefaultValue(initialClangDiagnosticConfigId());
    diagnosticConfigId.setModelFactory([this] { return diagnosticConfigsModel(); });
    diagnosticConfigId.setEditWidgetFactory(
        [](const ClangDiagnosticConfigs &configs, const Id &id) {
            return new ClangDiagnosticConfigsWidget(configs, id);
        });

    for (BaseAspect *aspect : aspects()) {
        if (aspect != &useClangd)
            aspect->setEnabler(&useClangd);
    }
    sizeThresholdInKb.setEnabler(&sizeThresholdEnabled);

    setLayouter([this] { return clangdSettingsLayout(this); });

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

bool ClangdSettings::Data::useGoodClangd(const Kit *kit) const
{
    return useClangd && clangdVersion(clangdFilePath(kit)) >= minimumClangdVersion();
}

void ClangdSettings::setUseClangd(bool use)
{
    instance().useClangd.setValue(use);
}

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
    const std::optional<quint64> totalRam = HostOsInfo::totalMemoryInstalledInBytes();
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
    if (instance().m_data.customDiagnosticConfigs == configs)
        return;
    instance().m_data.customDiagnosticConfigs = configs;
    instance().saveSettings();
}

ClangDiagnosticConfigsModel ClangdSettings::diagnosticConfigsModel()
{
    const ClangDiagnosticConfigs &customConfigs = instance().m_data.customDiagnosticConfigs;
    ClangDiagnosticConfigsModel model;
    model.addBuiltinConfigs();
    for (const ClangDiagnosticConfig &config : customConfigs)
        model.appendOrUpdate(config);
    return model;
}

FilePath ClangdSettings::Data::clangdFilePath(const Kit *kit) const
{
    if (kit && BuildDeviceTypeKitAspect::deviceTypeId(kit)
                   != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        if (const IDeviceConstPtr buildDevice = BuildDeviceKitAspect::device(kit)) {
            FilePath clangd = buildDevice->deviceToolPath(CppEditor::Constants::CLANGD_TOOL_ID);
            if (!clangd.isEmpty())
                return clangd;
        }
    }

    if (!executableFilePath.isEmpty())
        return executableFilePath;
    return fallbackClangdFilePath();
}

FilePath ClangdSettings::Data::projectIndexPath(const MacroExpander &expander) const
{
    return FilePath::fromUserInput(expander.expand(projectIndexPathTemplate));
}

FilePath ClangdSettings::Data::sessionIndexPath(const MacroExpander &expander) const
{
    return FilePath::fromUserInput(expander.expand(sessionIndexPathTemplate));
}

bool ClangdSettings::Data::sizeIsOkay(const FilePath &fp) const
{
    return !sizeThresholdEnabled || sizeThresholdInKb * 1024 >= fp.fileSize();
}

Id ClangdSettings::Data::diagnosticConfigIdOrDefault() const
{
    if (diagnosticConfigsModel().hasConfigWithId(diagnosticConfigId))
        return diagnosticConfigId;
    return initialClangDiagnosticConfigId();
}

ClangDiagnosticConfig ClangdSettings::Data::diagnosticConfig() const
{
    return diagnosticConfigsModel().configWithId(diagnosticConfigIdOrDefault());
}

bool ClangdSettings::Data::isSessionMode() const
{
    return granularity() == Granularity::Session;
}

ClangdSettings::Data::Granularity ClangdSettings::Data::granularity() const
{
    if (sessionsWithOneClangd.contains(Core::SessionManager::activeSession()))
        return Granularity::Session;
    return Granularity::Project;
}

ClangdSettings::Data ClangdSettings::data() const
{
    Data d;
    d.useClangd = useClangd();
    d.executableFilePath = clangdPath();
    d.autoIncludeHeaders = autoIncludeHeaders();
    d.sizeThresholdEnabled = sizeThresholdEnabled();
    d.updateDependentSources = updateDependentSources();
    d.useExternalCompilationDb = useExternalCompilationDb();
    d.workerThreadLimit = int(workerThreadLimit());
    d.documentUpdateThreshold = int(documentUpdateThreshold());
    d.sizeThresholdInKb = sizeThresholdInKb();
    d.completionResults = int(completionResults());
    d.projectIndexPathTemplate = projectIndexPathTemplate();
    d.sessionIndexPathTemplate = sessionIndexPathTemplate();
    d.indexingPriority = indexingPriority();
    d.headerSourceSwitchMode = headerSourceSwitchMode();
    d.completionRankingModel = completionRankingModel();
    d.completionStyle = completionStyle();
    d.sessionsWithOneClangd = m_data.sessionsWithOneClangd;
    d.customDiagnosticConfigs = m_data.customDiagnosticConfigs;
    d.diagnosticConfigId = diagnosticConfigId();
    d.haveCheckedHardwareReqirements = m_data.haveCheckedHardwareReqirements;
    return d;
}

void ClangdSettings::setData(const Data &data, bool saveAndEmitSignal)
{
    if (this == &instance() && data != this->data()) {
        useClangd.setValue(data.useClangd);
        clangdPath.setValue(data.executableFilePath);
        autoIncludeHeaders.setValue(data.autoIncludeHeaders);
        sizeThresholdEnabled.setValue(data.sizeThresholdEnabled);
        updateDependentSources.setValue(data.updateDependentSources);
        useExternalCompilationDb.setValue(data.useExternalCompilationDb);
        workerThreadLimit.setValue(data.workerThreadLimit);
        documentUpdateThreshold.setValue(data.documentUpdateThreshold);
        sizeThresholdInKb.setValue(data.sizeThresholdInKb);
        completionResults.setValue(data.completionResults);
        projectIndexPathTemplate.setValue(data.projectIndexPathTemplate);
        sessionIndexPathTemplate.setValue(data.sessionIndexPathTemplate);
        indexingPriority.setValue(data.indexingPriority);
        headerSourceSwitchMode.setValue(data.headerSourceSwitchMode);
        completionRankingModel.setValue(data.completionRankingModel);
        completionStyle.setValue(data.completionStyle);
        diagnosticConfigId.setValue(data.diagnosticConfigId);
        m_data.sessionsWithOneClangd = data.sessionsWithOneClangd;
        m_data.customDiagnosticConfigs = data.customDiagnosticConfigs;
        m_data.haveCheckedHardwareReqirements = data.haveCheckedHardwareReqirements;
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

FilePath ClangdSettings::Data::clangdIncludePath(const Kit *kit) const
{
    QTC_ASSERT(useGoodClangd(kit), return {});
    FilePath clangdPath = clangdFilePath(kit);
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

    // Read aspects from QtcSettings (respects the "ClangdSettings" group)
    AspectContainer::readSettings();

    // Pre-8.0 compat: old boolean indexing key
    settings->beginGroup(clangdSettingsKey());
    if (settings->contains(clangdIndexingKey())
            && !settings->value(clangdIndexingKey()).toBool()) {
        indexingPriority.setValue(IndexingPriority::Off);
    }
    settings->endGroup();

    // Complex fields not covered by aspects
    const Store store = storeFromSettings(clangdSettingsKey(), settings);
    m_data.sessionsWithOneClangd =
        store.value(sessionsWithOneClangdKey()).toStringList();
    m_data.haveCheckedHardwareReqirements =
        store.value(checkedHardwareKey(), false).toBool();

    settings->beginGroup(Constants::CPPEDITOR_SETTINGSGROUP);
    m_data.customDiagnosticConfigs = diagnosticConfigsFromSettings(settings);

    // Pre-8.0 compat
    static const Key oldKey("ClangDiagnosticConfig");
    const QVariant configId = settings->value(oldKey);
    if (configId.isValid()) {
        diagnosticConfigId.setValue(Id::fromSetting(configId));
        settings->setValue(oldKey, {});
    }

    settings->endGroup();
}

void ClangdSettings::saveSettings()
{
    const auto settings = Core::ICore::settings();

    // Write aspects (respects the "ClangdSettings" group)
    AspectContainer::writeSettings();

    // Complex fields alongside aspect data
    settings->beginGroup(clangdSettingsKey());
    settings->setValue(sessionsWithOneClangdKey(), m_data.sessionsWithOneClangd);
    settings->setValue(checkedHardwareKey(), m_data.haveCheckedHardwareReqirements);
    settings->endGroup();

    settings->beginGroup(Constants::CPPEDITOR_SETTINGSGROUP);
    diagnosticConfigsToSettings(settings, m_data.customDiagnosticConfigs);
    settings->endGroup();
}

#ifdef WITH_TESTS
void ClangdSettings::setClangdFilePath(const FilePath &filePath)
{
    instance().clangdPath.setValue(filePath);
}
#endif

Store ClangdSettings::Data::toMap() const
{
    Store map;

    map.insert(useClangdKey(), useClangd);

    map.insert(clangdPathKey(),
               executableFilePath != fallbackClangdFilePath() ? executableFilePath.toSettings()
                                                              : QVariant());

    map.insert(clangdIndexingKey(), indexingPriority != IndexingPriority::Off);
    map.insert(clangdIndexingPriorityKey(), int(indexingPriority));
    map.insert(clangdProjectIndexPathKey(), projectIndexPathTemplate);
    map.insert(clangdSessionIndexPathKey(), sessionIndexPathTemplate);
    map.insert(clangdHeaderSourceSwitchModeKey(), int(headerSourceSwitchMode));
    map.insert(clangdCompletionRankingModelKey(), int(completionRankingModel));
    map.insert(clangdCompletionStyleKey(), int(completionStyle));
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
    map.insert(useExternalCompilationDbKey(), useExternalCompilationDb);
    return map;
}

void ClangdSettings::Data::fromMap(const Store &map)
{
    useClangd = map.value(useClangdKey(), true).toBool();
    executableFilePath = FilePath::fromSettings(map.value(clangdPathKey()));
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
    completionStyle = CompletionStyle(
        map.value(clangdCompletionStyleKey(), int(completionStyle)).toInt());
    autoIncludeHeaders = map.value(clangdHeaderInsertionKey(), false).toBool();
    useExternalCompilationDb = map.value(useExternalCompilationDbKey(), false).toBool();
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

class ClangdProjectSettings : public ClangdSettings
{
public:
    explicit ClangdProjectSettings(Project *project)
        : m_project(project)
    {
        // Base constructor loaded global settings into aspects; now override
        // with project-specific values if applicable.
        setAutoApply(true);
        setLayouter([this] { return clangdSettingsLayout(this); });

        const Store store =
            storeFromVariant(project->namedSettings(clangdSettingsKey()));
        const bool global =
            store.value(useGlobalSettingsKey(), true).toBool();
        useGlobalSettings.setValue(global);
        if (!global) {
            Data d;
            d.fromMap(store);
            useClangd.setValue(d.useClangd);
            clangdPath.setValue(d.executableFilePath);
            autoIncludeHeaders.setValue(d.autoIncludeHeaders);
            sizeThresholdEnabled.setValue(d.sizeThresholdEnabled);
            updateDependentSources.setValue(d.updateDependentSources);
            useExternalCompilationDb.setValue(d.useExternalCompilationDb);
            workerThreadLimit.setValue(d.workerThreadLimit);
            documentUpdateThreshold.setValue(d.documentUpdateThreshold);
            sizeThresholdInKb.setValue(d.sizeThresholdInKb);
            completionResults.setValue(d.completionResults);
            projectIndexPathTemplate.setValue(d.projectIndexPathTemplate);
            indexingPriority.setValue(d.indexingPriority);
            headerSourceSwitchMode.setValue(d.headerSourceSwitchMode);
            completionRankingModel.setValue(d.completionRankingModel);
            completionStyle.setValue(d.completionStyle);
            diagnosticConfigId.setValue(d.diagnosticConfigId);
            m_data.customDiagnosticConfigs = d.customDiagnosticConfigs;
        }

        setEnabled(!useGlobalSettings());

        useGlobalSettings.addOnChanged(this, [this] {
            setEnabled(!useGlobalSettings());
            save();
            emit ClangdSettings::instance().changed();
        });
        addOnChanged(this, [this] {
            if (!useGlobalSettings())
                save();
        });

        connect(&diagnosticConfigId, &BaseAspect::changed, this, [this] {
            m_ownDiagConfigChange = true;
            emit ClangdSettings::instance().changed();
            m_ownDiagConfigChange = false;
        });
        connect(&ClangdSettings::instance(), &ClangdSettings::changed, this, [this] {
            if (data().isSessionMode())
                useGlobalSettings.setValue(true);
            if (!m_ownDiagConfigChange) {
                if (useGlobalSettings())
                    diagnosticConfigId.setValue(ClangdSettings::instance().diagnosticConfigId(),
                                                BaseAspect::BeQuiet);
                diagnosticConfigId.refresh();
            }
        });
    }

    void save()
    {
        // Only sync customConfigs from the widget when it has been shown;
        // otherwise keep whatever was loaded from settings.
        if (diagnosticConfigId.hasWidget())
            m_data.customDiagnosticConfigs = diagnosticConfigId.customConfigs();
        Store store;
        store.insert(useGlobalSettingsKey(), useGlobalSettings());
        if (!useGlobalSettings()) {
            store = data().toMap();
            store.insert(useGlobalSettingsKey(), false);
        }
        m_project->setNamedSettings(clangdSettingsKey(), variantFromStore(store));
    }

    static Key extraDataKey() { return "ClangdProjectSettings"; }

    UseGlobalAspect useGlobalSettings{Constants::CPP_CLANGD_SETTINGS_ID};

private:
    Project * const m_project;
    bool m_ownDiagConfigChange = false;
};

static ClangdProjectSettings *clangdProjectSettings(Project *project)
{
    return projectSettings<ClangdProjectSettings>(project);
}

ClangdSettings::Data clangdSettingsForProject(Project *project)
{
    if (!project)
        return ClangdSettings::instance().data();
    const auto *ps = clangdProjectSettings(project);
    ClangdSettings::Data d = ps->useGlobalSettings()
        ? ClangdSettings::instance().data() : ps->data();
    if (project->property(blockProjectIndexingProperty).toBool())
        d.indexingPriority = ClangdSettings::IndexingPriority::Off;
    return d;
}

ClangdSettings::Data clangdSettingsForProject(BuildConfiguration *bc)
{
    return clangdSettingsForProject(bc ? bc->project() : nullptr);
}

void clangdBlockIndexingForProject(Project *project)
{
    QTC_ASSERT(project, return);
    project->setProperty(blockProjectIndexingProperty, true);

    emit ClangdSettings::instance().changed();
}

void clangdUnblockIndexingForProject(Project *project)
{
    QTC_ASSERT(project, return);
    project->setProperty(blockProjectIndexingProperty, false);
}

void clangdSetDiagnosticConfigId(Project *project, Id id)
{
    QTC_ASSERT(project, return);
    ClangdProjectSettings *ps = clangdProjectSettings(project);
    ps->diagnosticConfigId.setValue(id, BaseAspect::BeQuiet);
    ps->useGlobalSettings.setValue(false, BaseAspect::BeQuiet);
    ps->save();
    emit ClangdSettings::instance().changed();
}

namespace Internal {

class ClangdSettingsPageWidget final : public Core::IOptionsPageWidget
{
public:
    ClangdSettingsPageWidget()
    {
        ClangdSettings &s = ClangdSettings::instance();

        m_sessionsModel.setStringList(s.m_data.sessionsWithOneClangd);
        m_sessionsModel.sort(0);

        auto sessionsView = new ListView;
        sessionsView->setModel(&m_sessionsModel);
        sessionsView->setToolTip(Tr::tr(
            "By default, Qt Creator runs one clangd process per project.\n"
            "If you have sessions with tightly coupled projects that should be\n"
            "managed by the same clangd process, add them here."));
        auto addButton = new QPushButton(Tr::tr("Add ..."));
        auto removeButton = new QPushButton(Tr::tr("Remove"));
        const auto updateRemove = [removeButton, sessionsView] {
            removeButton->setEnabled(
                sessionsView->selectionModel()->hasSelection());
        };
        connect(sessionsView->selectionModel(),
                &QItemSelectionModel::selectionChanged,
                this, updateRemove);
        updateRemove();
        connect(removeButton, &QPushButton::clicked, this,
                [this, sessionsView] {
                    const QItemSelection sel =
                        sessionsView->selectionModel()->selection();
                    QTC_ASSERT(!sel.isEmpty(), return);
                    m_sessionsModel.removeRow(sel.indexes().first().row());
                    checkSettingsDirty();
                });
        connect(addButton, &QPushButton::clicked, this,
                [this, sessionsView] {
                    QInputDialog dlg(sessionsView);
                    QStringList sessions = Core::SessionManager::sessions();
                    QStringList current = m_sessionsModel.stringList();
                    for (const QString &s : std::as_const(current))
                        sessions.removeOne(s);
                    if (sessions.isEmpty())
                        return;
                    sessions.sort();
                    dlg.setLabelText(Tr::tr("Choose a session:"));
                    dlg.setComboBoxItems(sessions);
                    if (dlg.exec() == QDialog::Accepted) {
                        current << dlg.textValue();
                        m_sessionsModel.setStringList(current);
                        m_sessionsModel.sort(0);
                        checkSettingsDirty();
                    }
                });
        auto configFilesHelpLabel = new QLabel(
            Tr::tr("Additional settings are available via "
                   "<a href=\"https://clangd.llvm.org/config\">"
                   " clangd configuration files</a>.<br>"
                   "User-specific settings go <a href=\"%1\">here</a>, "
                   "project-specific settings can be configured by putting"
                   " a .clangd file into the project source tree.")
                .arg(ClangdSettings::clangdUserConfigFilePath().toUserOutput()));
        configFilesHelpLabel->setWordWrap(true);
        connect(configFilesHelpLabel, &QLabel::linkHovered,
                configFilesHelpLabel, &QLabel::setToolTip);
        connect(configFilesHelpLabel, &QLabel::linkActivated,
                [](const QString &link) {
                    if (link.startsWith("https"))
                        QDesktopServices::openUrl(link);
                    else
                        Core::EditorManager::openEditor(FilePath::fromString(link));
                });

        using namespace Layouting;
        Column {
            s,
            createHr(),
            Group {
                title(Tr::tr("Sessions with a Single Clangd Instance")),
                Row {
                    sessionsView,
                    Column { addButton, removeButton, st },
                },
            },
            createHr(),
            configFilesHelpLabel,
            st,
        }.attachTo(this);

        connect(&s, &AspectContainer::volatileValueChanged, this, [] { checkSettingsDirty(); });

        setDirtyChecker([this] {
            const ClangdSettings &s = ClangdSettings::instance();
            if (s.isDirty())
                return true;
            QStringList stored = s.m_data.sessionsWithOneClangd;
            stored.sort();
            return m_sessionsModel.stringList() != stored;
        });
    }

private:
    void apply() final
    {
        ClangdSettings &s = ClangdSettings::instance();
        s.apply();
        s.m_data.customDiagnosticConfigs = s.diagnosticConfigId.customConfigs();
        s.m_data.sessionsWithOneClangd = m_sessionsModel.stringList();
        s.saveSettings();
        emit s.changed();
    }

    QStringListModel m_sessionsModel;
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
    ClangdSettings::instance().setAutoApply(false);
}

class ClangdProjectSettingsWidget : public QWidget
{
public:
    ClangdProjectSettingsWidget(Project *project)
    {
        ClangdProjectSettings *ps = clangdProjectSettings(project);

        using namespace Layouting;
        auto settingsWidget = Column {
            *ps,
            noMargin,
        }.emerge();
        settingsWidget->setEnabled(!ps->useGlobalSettings());
        ps->useGlobalSettings.addOnChanged(settingsWidget, [ps, settingsWidget] {
            settingsWidget->setEnabled(!ps->useGlobalSettings());
        });

        Column {
            ps->useGlobalSettings,
            settingsWidget,
            noMargin,
            st,
        }.attachTo(this);
    }
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
