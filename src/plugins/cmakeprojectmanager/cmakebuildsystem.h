// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "builddirparameters.h"
#include "cmakebuildtarget.h"
#include "fileapireader.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>

#include <utils/fileutils.h>
#include <utils/temporarydirectory.h>

namespace CppEditor { class CppProjectUpdater; }
namespace ProjectExplorer {
    class ExtraCompiler;
    class FolderNode;
}
namespace Utils { class Process; }

namespace CMakeProjectManager {

class CMakeBuildConfiguration;
class CMakeProject;

namespace Internal {

// --------------------------------------------------------------------
// CMakeBuildSystem:
// --------------------------------------------------------------------

class CMakeBuildSystem final : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    explicit CMakeBuildSystem(CMakeBuildConfiguration *bc);
    ~CMakeBuildSystem() final;

    void triggerParsing() final;

    bool supportsAction(ProjectExplorer::Node *context,
                        ProjectExplorer::ProjectAction action,
                        const ProjectExplorer::Node *node) const final;

    bool addFiles(ProjectExplorer::Node *context,
                  const Utils::FilePaths &filePaths, Utils::FilePaths *) final;

    ProjectExplorer::RemovedFilesFromProject removeFiles(ProjectExplorer::Node *context,
                                                         const Utils::FilePaths &filePaths,
                                                         Utils::FilePaths *notRemoved
                                                         = nullptr) final;

    bool canRenameFile(ProjectExplorer::Node *context,
                       const Utils::FilePath &oldFilePath,
                       const Utils::FilePath &newFilePath) final;
    bool renameFile(ProjectExplorer::Node *context,
                    const Utils::FilePath &oldFilePath,
                    const Utils::FilePath &newFilePath) final;

    Utils::FilePaths filesGeneratedFrom(const Utils::FilePath &sourceFile) const final;
    QString name() const final { return QLatin1String("cmake"); }

    // Actions:
    void runCMake();
    void runCMakeAndScanProjectTree();
    void runCMakeWithExtraArguments();
    void stopCMakeRun();

    bool persistCMakeState();
    void clearCMakeCache();

    // Context menu actions:
    void buildCMakeTarget(const QString &buildTarget);

    // Queries:
    const QList<ProjectExplorer::BuildTargetInfo> appTargets() const;
    QStringList buildTargetTitles() const;
    const QList<CMakeBuildTarget> &buildTargets() const;
    ProjectExplorer::DeploymentData deploymentDataFromFile() const;

    CMakeBuildConfiguration *cmakeBuildConfiguration() const;

    QList<ProjectExplorer::TestCaseInfo> const testcasesInfo() const final;
    Utils::CommandLine commandLineForTests(const QList<QString> &tests,
                                           const QStringList &options) const final;

    ProjectExplorer::MakeInstallCommand makeInstallCommand(
            const Utils::FilePath &installRoot) const final;

    static bool filteredOutTarget(const CMakeBuildTarget &target);

    bool isMultiConfig() const;
    void setIsMultiConfig(bool isMultiConfig);

    bool isMultiConfigReader() const;
    bool usesAllCapsTargets() const;

    CMakeProject *project() const;

    QString cmakeBuildType() const;
    ProjectExplorer::BuildConfiguration::BuildType buildType() const;

    CMakeConfig configurationFromCMake() const;
    CMakeConfig configurationChanges() const;

    QStringList configurationChangesArguments(bool initialParameters = false) const;

    void setConfigurationFromCMake(const CMakeConfig &config);
    void setConfigurationChanges(const CMakeConfig &config);

    QString error() const;
    QString warning() const;

signals:
    void configurationCleared();
    void configurationChanged(const CMakeConfig &config);
    void errorOccurred(const QString &message);
    void warningOccurred(const QString &message);

private:
    CMakeConfig initialCMakeConfiguration() const;

    QList<QPair<Utils::Id, QString>> generators() const override;
    void runGenerator(Utils::Id id) override;
    ProjectExplorer::ExtraCompiler *findExtraCompiler(
            const ExtraCompilerFilter &filter) const override;

    enum ForceEnabledChanged { False, True };
    void clearError(ForceEnabledChanged fec = ForceEnabledChanged::False);

    void setError(const QString &message);
    void setWarning(const QString &message);

    // Actually ask for parsing:
    enum ReparseParameters {
        REPARSE_DEFAULT = 0, // Nothing special:-)
        REPARSE_FORCE_CMAKE_RUN
        = (1 << 0), // Force cmake to run, apply extraCMakeArguments if non-empty
        REPARSE_FORCE_INITIAL_CONFIGURATION
        = (1 << 1), // Force initial configuration arguments to cmake
        REPARSE_FORCE_EXTRA_CONFIGURATION = (1 << 2), // Force extra configuration arguments to cmake
        REPARSE_URGENT = (1 << 3),                    // Do not delay the parser run by 1s
    };
    void reparse(int reparseParameters);
    QString reparseParametersString(int reparseFlags);
    void setParametersAndRequestParse(const BuildDirParameters &parameters,
                                      const int reparseParameters);

    bool mustApplyConfigurationChangesArguments(const BuildDirParameters &parameters) const;

    // State handling:
    // Parser states:
    void handleParsingSuccess();
    void handleParsingError();

    // Treescanner states:
    void handleTreeScanningFinished();

    // Combining Treescanner and Parser states:
    void combineScanAndParse(bool restoredFromBackup);
    void checkAndReportError(QString &errorMessage);

    void updateCMakeConfiguration(QString &errorMessage);

    void updateProjectData();
    void updateFallbackProjectData();
    QList<ProjectExplorer::ExtraCompiler *> findExtraCompilers();
    void updateQmlJSCodeModel(const QStringList &extraHeaderPaths,
                              const QList<QByteArray> &moduleMappings);
    void updateInitialCMakeExpandableVars();

    void updateFileSystemNodes();

    void handleParsingSucceeded(bool restoredFromBackup);
    void handleParsingFailed(const QString &msg);

    void wireUpConnections();

    void ensureBuildDirectory(const BuildDirParameters &parameters);
    void stopParsingAndClearState();
    void becameDirty();

    void updateReparseParameters(const int parameters);
    int takeReparseParameters();

    void runCTest();

    struct ProjectFileArgumentPosition
    {
        cmListFileArgument argumentPosition;
        Utils::FilePath cmakeFile;
        QString relativeFileName;
        bool fromGlobbing = false;
    };
    std::optional<ProjectFileArgumentPosition> projectFileArgumentPosition(
        const QString &targetName, const QString &fileName);

    ProjectExplorer::TreeScanner m_treeScanner;
    std::shared_ptr<ProjectExplorer::FolderNode> m_allFiles;
    QHash<QString, bool> m_mimeBinaryCache;

    bool m_waitingForParse = false;
    bool m_combinedScanAndParseResult = false;

    bool m_isMultiConfig = false;

    ParseGuard m_currentGuard;

    CppEditor::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;
    QList<CMakeBuildTarget> m_buildTargets;
    QSet<CMakeFileInfo> m_cmakeFiles;

    QHash<QString, ProjectFileArgumentPosition> m_filesToBeRenamed;

    // Parsing state:
    BuildDirParameters m_parameters;
    int m_reparseParameters = REPARSE_DEFAULT;
    FileApiReader m_reader;
    mutable bool m_isHandlingError = false;

    // CTest integration
    Utils::FilePath m_ctestPath;
    std::unique_ptr<Utils::Process> m_ctestProcess;
    QList<ProjectExplorer::TestCaseInfo> m_testNames;

    CMakeConfig m_configurationFromCMake;
    CMakeConfig m_configurationChanges;

    QString m_error;
    QString m_warning;
};

} // namespace Internal
} // namespace CMakeProjectManager
