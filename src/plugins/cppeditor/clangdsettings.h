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

    static QString priorityToString(const IndexingPriority &priority);
    static QString priorityToDisplayString(const IndexingPriority &priority);
    static QString headerSourceSwitchModeToDisplayString(HeaderSourceSwitchMode mode);
    static QString rankingModelToCmdLineString(CompletionRankingModel model);
    static QString rankingModelToDisplayString(CompletionRankingModel model);
    static QString defaultProjectIndexPathTemplate();
    static QString defaultSessionIndexPathTemplate();

    class CPPEDITOR_EXPORT Data
    {
    public:
        Utils::Store toMap() const;
        void fromMap(const Utils::Store &map);

        friend bool operator==(const Data &s1, const Data &s2)
        {
            return s1.useClangd == s2.useClangd
                   && s1.executableFilePath == s2.executableFilePath
                   && s1.projectIndexPathTemplate == s2.projectIndexPathTemplate
                   && s1.sessionIndexPathTemplate == s2.sessionIndexPathTemplate
                   && s1.sessionsWithOneClangd == s2.sessionsWithOneClangd
                   && s1.customDiagnosticConfigs == s2.customDiagnosticConfigs
                   && s1.diagnosticConfigId == s2.diagnosticConfigId
                   && s1.workerThreadLimit == s2.workerThreadLimit
                   && s1.indexingPriority == s2.indexingPriority
                   && s1.headerSourceSwitchMode == s2.headerSourceSwitchMode
                   && s1.completionRankingModel == s2.completionRankingModel
                   && s1.autoIncludeHeaders == s2.autoIncludeHeaders
                   && s1.documentUpdateThreshold == s2.documentUpdateThreshold
                   && s1.sizeThresholdEnabled == s2.sizeThresholdEnabled
                   && s1.sizeThresholdInKb == s2.sizeThresholdInKb
                   && s1.haveCheckedHardwareReqirements == s2.haveCheckedHardwareReqirements
                   && s1.updateDependentSources == s2.updateDependentSources
                   && s1.completionResults == s2.completionResults;
        }
        friend bool operator!=(const Data &s1, const Data &s2) { return !(s1 == s2); }

        Utils::FilePath clangdFilePath(const ProjectExplorer::Kit *kit) const;
        bool useGoodClangd(const ProjectExplorer::Kit *kit) const;
        bool useExternalCompilationDb() const { return false; }
        Utils::FilePath clangdIncludePath(const ProjectExplorer::Kit *kit) const;
        bool sizeIsOkay(const Utils::FilePath &fp) const;
        ClangDiagnosticConfig diagnosticConfig() const;
        Utils::Id diagnosticConfigIdOrDefault() const;

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
        bool autoIncludeHeaders = false;
        bool sizeThresholdEnabled = false;
        bool haveCheckedHardwareReqirements = false;
        bool updateDependentSources = false;
        int completionResults = defaultCompletionResults();

    private:
        static int defaultCompletionResults();
    };

    ClangdSettings(const Data &data) : m_data(data) {}

    static ClangdSettings &instance();
    static void setUseClangd(bool use);
    static void setUseClangdAndSave(bool use);

    static bool hardwareFulfillsRequirements();
    static bool haveCheckedHardwareRequirements();

    static void setDefaultClangdPath(const Utils::FilePath &filePath);
    static void setCustomDiagnosticConfigs(const ClangDiagnosticConfigs &configs);
    static ClangDiagnosticConfigsModel diagnosticConfigsModel();

    Utils::FilePath projectIndexPath(const Utils::MacroExpander &expander) const;
    Utils::FilePath sessionIndexPath(const Utils::MacroExpander &expander) const;
    ClangDiagnosticConfigs customDiagnosticConfigs() const;

    enum class Granularity { Project, Session };
    Granularity granularity() const;

    void setData(const Data &data, bool saveAndEmitSignal = true);
    Data data() const { return m_data; }

    static Utils::FilePath clangdUserConfigFilePath();

#ifdef WITH_TESTS
    static void setClangdFilePath(const Utils::FilePath &filePath);
#endif

private:
    ClangdSettings();

    void loadSettings();
    void saveSettings();

    Data m_data;
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
