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

// --------------------------------------------------------------------
// BuildSystem:
// --------------------------------------------------------------------

// Check buildsystem.md for more information
class PROJECTEXPLORER_EXPORT BuildSystem : public QObject
{
    Q_OBJECT

public:
    explicit BuildSystem(Target *target);
    explicit BuildSystem(BuildConfiguration *bc);
    ~BuildSystem() override;

    Project *project() const;
    Target *target() const;
    Kit *kit() const;
    BuildConfiguration *buildConfiguration() const;

    Utils::FilePath projectFilePath() const;
    Utils::FilePath projectDirectory() const;

    bool isWaitingForParse() const;

    void requestParse();
    void requestDelayedParse();
    void requestParseWithCustomDelay(int delayInMs = 1000);
    void cancelDelayedParseRequest();
    void setParseDelay(int delayInMs);
    int parseDelay() const;

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
    virtual bool renameFile(Node *context,
                            const Utils::FilePath &oldFilePath,
                            const Utils::FilePath &newFilePath);
    virtual bool addDependencies(Node *context, const QStringList &dependencies);
    virtual bool supportsAction(Node *context, ProjectAction action, const Node *node) const;
    virtual QString name() const = 0;

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

signals:
    void parsingStarted();
    void parsingFinished(bool success);
    void testInformationUpdated();
    void debuggingStarted();

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

} // namespace ProjectExplorer
