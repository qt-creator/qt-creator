// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangdiagnosticconfig.h"
#include "cppeditor_global.h"

#include <utils/clangutils.h>
#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/store.h>
#include <utils/qtcsettings.h>

#include <QObject>
#include <QStringList>
#include <QVersionNumber>

namespace ProjectExplorer { class Project; }

namespace CppEditor {

class CPPEDITOR_EXPORT CppCodeModelSettings : public QObject
{
    Q_OBJECT

public:
    enum PCHUsage {
        PchUse_None = 1,
        PchUse_BuildSystem = 2
    };

public:
    void fromSettings(Utils::QtcSettings *s);
    void toSettings(Utils::QtcSettings *s);

public:
    bool enableLowerClazyLevels() const;
    void setEnableLowerClazyLevels(bool yesno);

    PCHUsage pchUsage() const;
    void setPCHUsage(PCHUsage pchUsage);

    bool interpretAmbigiousHeadersAsCHeaders() const;
    void setInterpretAmbigiousHeadersAsCHeaders(bool yesno);

    bool skipIndexingBigFiles() const;
    void setSkipIndexingBigFiles(bool yesno);

    bool useBuiltinPreprocessor() const { return m_useBuiltinPreprocessor; }
    void setUseBuiltinPreprocessor(bool useBuiltin) { m_useBuiltinPreprocessor = useBuiltin; }

    int indexerFileSizeLimitInMb() const;
    void setIndexerFileSizeLimitInMb(int sizeInMB);

    void setCategorizeFindReferences(bool categorize) { m_categorizeFindReferences = categorize; }
    bool categorizeFindReferences() const { return m_categorizeFindReferences; }

    bool ignoreFiles() const;
    void setIgnoreFiles(bool ignoreFiles);
    QString ignorePattern() const;
    void setIgnorePattern(const QString& ignorePattern);

signals:
    void clangDiagnosticConfigsInvalidated(const QVector<Utils::Id> &configId);
    void changed();

private:
    PCHUsage m_pchUsage = PchUse_BuildSystem;
    bool m_interpretAmbigiousHeadersAsCHeaders = false;
    bool m_skipIndexingBigFiles = true;
    bool m_useBuiltinPreprocessor = true;
    int m_indexerFileSizeLimitInMB = 5;
    bool m_enableLowerClazyLevels = true; // For UI behavior only
    bool m_categorizeFindReferences = false; // Ephemeral!
    bool m_ignoreFiles = false;
    QString m_ignorePattern;
};

class CPPEDITOR_EXPORT ClangdSettings : public QObject
{
    Q_OBJECT
public:
    enum class IndexingPriority { Off, Background, Normal, Low, };
    enum class HeaderSourceSwitchMode { BuiltinOnly, ClangdOnly, Both };
    enum class CompletionRankingModel { Default, DecisionForest, Heuristics };

    static QString priorityToString(const IndexingPriority &priority);
    static QString priorityToDisplayString(const IndexingPriority &priority);
    static QString headerSourceSwitchModeToDisplayString(HeaderSourceSwitchMode mode);
    static QString rankingModelToCmdLineString(CompletionRankingModel model);
    static QString rankingModelToDisplayString(CompletionRankingModel model);

    class CPPEDITOR_EXPORT Data
    {
    public:
        Utils::Store toMap() const;
        void fromMap(const Utils::Store &map);

        friend bool operator==(const Data &s1, const Data &s2)
        {
            return s1.useClangd == s2.useClangd
                    && s1.executableFilePath == s2.executableFilePath
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
                    && s1.completionResults == s2.completionResults;
        }
        friend bool operator!=(const Data &s1, const Data &s2) { return !(s1 == s2); }

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
        HeaderSourceSwitchMode headerSourceSwitchMode = HeaderSourceSwitchMode::Both;
        CompletionRankingModel completionRankingModel = CompletionRankingModel::Default;
        bool autoIncludeHeaders = false;
        bool sizeThresholdEnabled = false;
        bool haveCheckedHardwareReqirements = false;
        int completionResults = defaultCompletionResults();
    };

    ClangdSettings(const Data &data) : m_data(data) {}

    static ClangdSettings &instance();
    bool useClangd() const;
    static void setUseClangd(bool use);
    static void setUseClangdAndSave(bool use);

    static bool hardwareFulfillsRequirements();
    static bool haveCheckedHardwareRequirements();

    static void setDefaultClangdPath(const Utils::FilePath &filePath);
    static void setCustomDiagnosticConfigs(const ClangDiagnosticConfigs &configs);
    Utils::FilePath clangdFilePath() const;
    IndexingPriority indexingPriority() const { return m_data.indexingPriority; }
    HeaderSourceSwitchMode headerSourceSwitchMode() const { return m_data.headerSourceSwitchMode; }
    CompletionRankingModel completionRankingModel() const { return m_data.completionRankingModel; }
    bool autoIncludeHeaders() const { return m_data.autoIncludeHeaders; }
    int workerThreadLimit() const { return m_data.workerThreadLimit; }
    int documentUpdateThreshold() const { return m_data.documentUpdateThreshold; }
    qint64 sizeThresholdInKb() const { return m_data.sizeThresholdInKb; }
    bool sizeThresholdEnabled() const { return m_data.sizeThresholdEnabled; }
    int completionResults() const { return m_data.completionResults; }
    bool sizeIsOkay(const Utils::FilePath &fp) const;
    ClangDiagnosticConfigs customDiagnosticConfigs() const;
    Utils::Id diagnosticConfigId() const;
    ClangDiagnosticConfig diagnosticConfig() const;

    enum class Granularity { Project, Session };
    Granularity granularity() const;

    void setData(const Data &data);
    Data data() const { return m_data; }

    QVersionNumber clangdVersion() const { return Utils::clangdVersion(clangdFilePath()); }
    Utils::FilePath clangdIncludePath() const;
    static Utils::FilePath clangdUserConfigFilePath();

#ifdef WITH_TESTS
    static void setClangdFilePath(const Utils::FilePath &filePath);
#endif

signals:
    void changed();

private:
    ClangdSettings();

    void loadSettings();
    void saveSettings();

    Data m_data;
};

class CPPEDITOR_EXPORT ClangdProjectSettings
{
public:
    ClangdProjectSettings(ProjectExplorer::Project *project);

    ClangdSettings::Data settings() const;
    void setSettings(const ClangdSettings::Data &data);
    bool useGlobalSettings() const { return m_useGlobalSettings; }
    void setUseGlobalSettings(bool useGlobal);
    void setDiagnosticConfigId(Utils::Id configId);
    void blockIndexing();
    void unblockIndexing();

private:
    void loadSettings();
    void saveSettings();

    ProjectExplorer::Project * const m_project;
    ClangdSettings::Data m_customSettings;
    bool m_useGlobalSettings = true;
    bool m_blockIndexing = false;
};

} // namespace CppEditor
