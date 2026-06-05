// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangdiagnosticconfig.h"
#include "cppeditor_global.h"

#include <utils/aspects.h>
#include <utils/filepath.h>
#include <utils/store.h>

namespace ProjectExplorer {
class BuildConfiguration;
class Kit;
class Project;
} // ProjectExplorer

namespace Utils { class MacroExpander; }

namespace CppEditor {
class ClangDiagnosticConfigsModel;

// TODO: Can we move this to ClangCodeModel?
class CPPEDITOR_EXPORT ClangdSettings : public Utils::AspectContainer
{
public:
    enum class IndexingPriority { Off, Background, Normal, Low, };
    enum class HeaderSourceSwitchMode { BuiltinOnly, ClangdOnly, Both };
    enum class CompletionRankingModel { Default, DecisionForest, Heuristics };
    enum class CompletionStyle { Default, Detailed, Bundled };

    static QString priorityToString(const IndexingPriority &priority);
    static QString priorityToDisplayString(const IndexingPriority &priority);
    static QString headerSourceSwitchModeToDisplayString(HeaderSourceSwitchMode mode);
    static QString rankingModelToCmdLineString(CompletionRankingModel model);
    static QString rankingModelToDisplayString(CompletionRankingModel model);
    static QString completionStyleToCmdLineString(CompletionStyle style);
    static QString completionStyleToDisplayString(CompletionStyle style);
    static QString defaultProjectIndexPathTemplate();
    static QString defaultSessionIndexPathTemplate();

    class CPPEDITOR_EXPORT Data
    {
    public:
        Utils::Store toMap() const;
        void fromMap(const Utils::Store &map);

        Utils::FilePath clangdFilePath(const ProjectExplorer::Kit *kit) const;
        bool useGoodClangd(const ProjectExplorer::Kit *kit) const;
        Utils::FilePath clangdIncludePath(const ProjectExplorer::Kit *kit) const;
        bool sizeIsOkay(const Utils::FilePath &fp) const;
        ClangDiagnosticConfig diagnosticConfig() const;
        Utils::Id diagnosticConfigIdOrDefault() const;

        Utils::FilePath projectIndexPath(const Utils::MacroExpander &expander) const;
        Utils::FilePath sessionIndexPath(const Utils::MacroExpander &expander) const;

        enum class Granularity { Project, Session };
        Granularity granularity() const;
        bool isSessionMode() const;

        static int defaultCompletionResults();

        Utils::FilePath executableFilePath;
        QStringList sessionsWithOneClangd;
        ClangDiagnosticConfigs customDiagnosticConfigs;
        Utils::Id diagnosticConfigId;

        int workerThreadLimit = 0;
        int documentUpdateThreshold = 500;
        qint64 sizeThresholdInKb = 1024;
        bool useClangd = true;
        IndexingPriority indexingPriority = IndexingPriority::Low;
        QString projectIndexPathTemplate = defaultProjectIndexPathTemplate();
        QString sessionIndexPathTemplate = defaultSessionIndexPathTemplate();
        HeaderSourceSwitchMode headerSourceSwitchMode = HeaderSourceSwitchMode::Both;
        CompletionRankingModel completionRankingModel = CompletionRankingModel::Default;
        CompletionStyle completionStyle = CompletionStyle::Default;
        bool autoIncludeHeaders = false;
        bool sizeThresholdEnabled = false;
        bool haveCheckedHardwareReqirements = false;
        bool updateDependentSources = false;
        bool useExternalCompilationDb = false;
        int completionResults = defaultCompletionResults();

    private:
        friend bool operator==(const Data &, const Data &) = default;
    };

    // Aspects (authoritative storage; m_data used only for complex fields)
    Utils::BoolAspect useClangd{this};
    Utils::FilePathAspect clangdPath{this};
    Utils::BoolAspect autoIncludeHeaders{this};
    Utils::BoolAspect sizeThresholdEnabled{this};
    Utils::BoolAspect updateDependentSources{this};
    Utils::BoolAspect useExternalCompilationDb{this};
    Utils::IntegerAspect workerThreadLimit{this};
    Utils::IntegerAspect documentUpdateThreshold{this};
    Utils::IntegerAspect sizeThresholdInKb{this};
    Utils::IntegerAspect completionResults{this};
    Utils::StringAspect projectIndexPathTemplate{this};
    Utils::StringAspect sessionIndexPathTemplate{this};
    Utils::TypedSelectionAspect<IndexingPriority> indexingPriority{this};
    Utils::TypedSelectionAspect<HeaderSourceSwitchMode> headerSourceSwitchMode{this};
    Utils::TypedSelectionAspect<CompletionRankingModel> completionRankingModel{this};
    Utils::TypedSelectionAspect<CompletionStyle> completionStyle{this};

    static ClangdSettings &instance();
    static void setUseClangd(bool use);
    static void setUseClangdAndSave(bool use);

    static bool hardwareFulfillsRequirements();
    static bool haveCheckedHardwareRequirements();

    static void setDefaultClangdPath(const Utils::FilePath &filePath);
    static void setCustomDiagnosticConfigs(const ClangDiagnosticConfigs &configs);
    static ClangDiagnosticConfigsModel diagnosticConfigsModel();

    void setData(const Data &data, bool saveAndEmitSignal = true);
    Data data() const;

    static Utils::FilePath clangdUserConfigFilePath();

#ifdef WITH_TESTS
    static void setClangdFilePath(const Utils::FilePath &filePath);
#endif

    void saveSettings();

    Data m_data;

protected:
    ClangdSettings();
    void loadSettings();
};

CPPEDITOR_EXPORT ClangdSettings::Data clangdProjectSettings(ProjectExplorer::BuildConfiguration *bc);
CPPEDITOR_EXPORT ClangdSettings::Data clangdProjectSettings(ProjectExplorer::Project *project);

CPPEDITOR_EXPORT void clangdBlockIndexingForProject(ProjectExplorer::Project *project);
CPPEDITOR_EXPORT void clangdUnblockIndexingForProject(ProjectExplorer::Project *project);
CPPEDITOR_EXPORT void clangdSetDiagnosticConfigId(ProjectExplorer::Project *project, Utils::Id id);

namespace Internal {
void setupClangdProjectSettingsPanel();
void setupClangdSettingsPage();
}

} // namespace CppEditor
