// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "buildtargetinfo.h"
#include "project.h"
#include "projectnodes.h"

#include <QObject>

namespace Utils {
class CommandLine;
}

namespace ProjectExplorer {

class BuildConfiguration;
class BuildStepList;
class ExtraCompiler;
class Node;

struct TestCaseInfo
{
    QString name;
    int number = -1;
    Utils::FilePath path;
    int line = 0;
};

// Extra infomation needed by QmlJS tools and editor.
struct QmlCodeModelInfo
{
    QList<Utils::FilePath> sourceFiles;
    Utils::FilePaths qmlImportPaths;
    QList<Utils::FilePath> activeResourceFiles;
    QList<Utils::FilePath> allResourceFiles;
    QList<Utils::FilePath> generatedQrcFiles;
    QHash<Utils::FilePath, QString> resourceFileContents;
    QList<Utils::FilePath> applicationDirectories;
    QHash<QString, QString> moduleMappings; // E.g.: QtQuick.Controls -> MyProject.MyControls

    // whether trying to run qmldump makes sense
    bool tryQmlDump = false;
    bool qmlDumpHasRelocatableFlag = true;
    Utils::FilePath qmlDumpPath;
    Utils::Environment qmlDumpEnvironment;

    Utils::FilePath qtQmlPath;
    Utils::FilePath qmllsPath;
    QString qtVersionString;
};

// --------------------------------------------------------------------
// BuildSystem:
// --------------------------------------------------------------------

// Check buildsystem.md for more information
class PROJECTEXPLORER_EXPORT BuildSystem : public QObject
{
    Q_OBJECT

public:
    explicit BuildSystem(BuildConfiguration *bc);
    ~BuildSystem() override;

    QString name() const;
    Project *project() const;
    Target *target() const;
    Kit *kit() const;
    BuildConfiguration *buildConfiguration() const;

    Utils::FilePath projectFilePath() const;
    Utils::FilePath projectDirectory() const;

    bool isWaitingForParse() const;

    void requestParse();
    void requestDelayedParse();
    void cancelDelayedParseRequest();

    bool isParsing() const;
    bool hasParsingData() const;

    Utils::Environment activeParseEnvironment() const;

    virtual void requestDebugging() {}
    virtual bool addFiles(Node *context,
                          const Utils::FilePaths &filePaths,
                          Utils::FilePaths *notAdded = nullptr);
    virtual RemovedFilesFromProject removeFiles(Node *context,
                                                const Utils::FilePaths &filePaths,
                                                Utils::FilePaths *notRemoved = nullptr);
    virtual bool deleteFiles(Node *context, const Utils::FilePaths &filePaths);
    virtual bool canRenameFile(Node *context,
                               const Utils::FilePath &oldFilePath,
                               const Utils::FilePath &newFilePath);
    virtual bool renameFiles(
        Node *context, const Utils::FilePairs &filesToRename, Utils::FilePaths *notRenamed);
    virtual bool addDependencies(Node *context, const QStringList &dependencies);
    virtual bool supportsAction(Node *context, ProjectAction action, const Node *node) const;
    virtual void buildNamedTarget(const QString &target) { Q_UNUSED(target) }

    // Owned by the build system. Use only in main thread. Can go away at any time.
    ExtraCompiler *extraCompilerForSource(const Utils::FilePath &source) const;
    ExtraCompiler *extraCompilerForTarget(const Utils::FilePath &target) const;

    virtual MakeInstallCommand makeInstallCommand(const Utils::FilePath &installRoot) const;

    virtual Utils::FilePaths filesGeneratedFrom(const Utils::FilePath &sourceFile) const;
    virtual QVariant additionalData(Utils::Id id) const;
    virtual QList<QPair<Utils::Id, QString>> generators() const { return {}; }
    virtual void runGenerator(Utils::Id) {}

    void setDeploymentData(const DeploymentData &deploymentData);
    DeploymentData deploymentData() const;

    void setApplicationTargets(const QList<BuildTargetInfo> &appTargets);
    const QList<BuildTargetInfo> applicationTargets() const;
    BuildTargetInfo buildTarget(const QString &buildKey) const;

    void setRootProjectNode(std::unique_ptr<ProjectNode> &&root);

    virtual const QList<TestCaseInfo> testcasesInfo() const { return {}; }
    virtual Utils::CommandLine commandLineForTests(const QList<QString> &tests,
                                                   const QStringList &options) const;

    class PROJECTEXPLORER_EXPORT ParseGuard
    {
        friend class BuildSystem;
        explicit ParseGuard(BuildSystem *p);

        void release();

    public:
        ParseGuard() = default;
        ~ParseGuard() { release(); }

        void markAsSuccess() const { m_success = true; }
        bool isSuccess() const { return m_success; }
        bool guardsProject() const { return m_buildSystem; }

        ParseGuard(const ParseGuard &other) = delete;
        ParseGuard &operator=(const ParseGuard &other) = delete;
        ParseGuard(ParseGuard &&other);
        ParseGuard &operator=(ParseGuard &&other);

    private:
        BuildSystem *m_buildSystem = nullptr;
        mutable bool m_success = false;
    };

    void emitBuildSystemUpdated();

    void setExtraData(const QString &buildKey, Utils::Id dataKey, const QVariant &data);
    QVariant extraData(const QString &buildKey, Utils::Id dataKey) const;

    static void startNewBuildSystemOutput(const QString &message);
    static void appendBuildSystemOutput(const QString &message);

public:
    // FIXME: Make this private and the BuildSystem a friend
    ParseGuard guardParsingRun() { return ParseGuard(this); }

    QString disabledReason(const QString &buildKey) const;

    virtual void triggerParsing() = 0;

    void updateQmlCodeModel();
    virtual void updateQmlCodeModelInfo(QmlCodeModelInfo &projectInfo);

signals:
    void parsingStarted();
    void parsingFinished(bool success);
    void updated(); // FIXME: Redundant with parsingFinished()?
    void testInformationUpdated();
    void debuggingStarted();
    void errorOccurred(const QString &message);
    void warningOccurred(const QString &message);
    void deploymentDataChanged();

protected:
    // Helper methods to manage parsing state and signalling
    // Call in GUI thread before the actual parsing starts
    void emitParsingStarted();
    // Call in GUI thread right after the actual parsing is done
    void emitParsingFinished(bool success);

    using ExtraCompilerFilter = std::function<bool(const ExtraCompiler *)>;

private:
    void requestParseHelper(int delay); // request a (delayed!) parser run.

    virtual ExtraCompiler *findExtraCompiler(const ExtraCompilerFilter &filter) const;

    class BuildSystemPrivate *d = nullptr;
};

PROJECTEXPLORER_EXPORT BuildSystem *activeBuildSystem(const Project *project);
PROJECTEXPLORER_EXPORT BuildSystem *activeBuildSystemForActiveProject();
PROJECTEXPLORER_EXPORT BuildSystem *activeBuildSystemForCurrentProject();

} // namespace ProjectExplorer
