/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "builddirreader.h"
#include "servermode.h"
#include "cmakeparser.h"

#include <QList>

#include <memory>

namespace ProjectExplorer { class ProjectNode; }

namespace CMakeProjectManager {
namespace Internal {

class ServerModeReader : public BuildDirReader
{
    Q_OBJECT

public:
    ServerModeReader();
    ~ServerModeReader() final;

    void setParameters(const BuildDirParameters &p) final;

    bool isCompatible(const BuildDirParameters &p) final;
    void resetData() final;
    void parse(bool forceConfiguration) final;
    void stop() final;

    bool isReady() const final;
    bool isParsing() const final;

    QList<CMakeBuildTarget> takeBuildTargets() final;
    CMakeConfig takeParsedConfiguration() final;
    void generateProjectTree(CMakeProjectNode *root,
                             const QList<const ProjectExplorer::FileNode *> &allFiles) final;
    void updateCodeModel(CppTools::RawProjectParts &rpps) final;

private:
    void handleReply(const QVariantMap &data, const QString &inReplyTo);
    void handleError(const QString &message);
    void handleProgress(int min, int cur, int max, const QString &inReplyTo);
    void handleSignal(const QString &signal, const QVariantMap &data);

    int calculateProgress(const int minRange, const int min,
                          const int cur,
                          const int max, const int maxRange);

    struct Target;
    struct Project;

    struct IncludePath {
        Utils::FileName path;
        bool isSystem;
    };

    struct FileGroup {
        ~FileGroup() { qDeleteAll(includePaths); includePaths.clear(); }

        Target *target = nullptr;
        QString compileFlags;
        ProjectExplorer::Macros macros;
        QList<IncludePath *> includePaths;
        QString language;
        QList<Utils::FileName> sources;
        bool isGenerated;
    };

    struct BacktraceItem {
        int line = -1;
        QString path;
        QString name;
    };

    struct CrossReference {
        ~CrossReference() { qDeleteAll(backtrace); backtrace.clear(); }
        QList<BacktraceItem *> backtrace;
        enum Type { TARGET, LIBRARIES, DEFINES, INCLUDES, UNKNOWN };
        Type type;
    };

    struct Target {
        ~Target() {
            qDeleteAll(fileGroups);
            fileGroups.clear();
            qDeleteAll(crossReferences);
            crossReferences.clear();
        }

        Project *project = nullptr;
        QString name;
        QString type;
        QList<Utils::FileName> artifacts;
        Utils::FileName sourceDirectory;
        Utils::FileName buildDirectory;
        QList<FileGroup *> fileGroups;
        QList<CrossReference *> crossReferences;
    };

    struct Project {
        ~Project() { qDeleteAll(targets); targets.clear(); }
        QString name;
        Utils::FileName sourceDirectory;
        QList<Target *> targets;
    };

    void extractCodeModelData(const QVariantMap &data);
    void extractConfigurationData(const QVariantMap &data);
    Project *extractProjectData(const QVariantMap &data, QSet<QString> &knownTargets);
    Target *extractTargetData(const QVariantMap &data, Project *p, QSet<QString> &knownTargets);
    FileGroup *extractFileGroupData(const QVariantMap &data, const QDir &srcDir, Target *t);
    QList<CrossReference *> extractCrossReferences(const QVariantMap &data);
    QList<BacktraceItem *> extractBacktrace(const QVariantList &data);
    BacktraceItem *extractBacktraceItem(const QVariantMap &data);
    void extractCMakeInputsData(const QVariantMap &data);
    void extractCacheData(const QVariantMap &data);

    void fixTarget(Target *target) const;

    QHash<Utils::FileName, ProjectExplorer::ProjectNode *>
    addCMakeLists(CMakeProjectNode *root, const QList<ProjectExplorer::FileNode *> &cmakeLists);
    void addProjects(const QHash<Utils::FileName, ProjectExplorer::ProjectNode *> &cmakeListsNodes,
                     const QList<Project *> &projects,
                     QList<ProjectExplorer::FileNode *> &knownHeaderNodes);
    void addTargets(const QHash<Utils::FileName, ProjectExplorer::ProjectNode *> &cmakeListsNodes,
                    const QList<Target *> &targets,
                    QList<ProjectExplorer::FileNode *> &knownHeaderNodes);
    void addFileGroups(ProjectExplorer::ProjectNode *targetRoot,
                       const Utils::FileName &sourceDirectory,
                       const Utils::FileName &buildDirectory, const QList<FileGroup *> &fileGroups,
                       QList<ProjectExplorer::FileNode *> &knowHeaderNodes);

    void addHeaderNodes(ProjectExplorer::ProjectNode *root,
                        const QList<ProjectExplorer::FileNode *> knownHeaders,
                        const QList<const ProjectExplorer::FileNode *> &allFiles);

    std::unique_ptr<ServerMode> m_cmakeServer;
    std::unique_ptr<QFutureInterface<void>> m_future;

    int m_progressStepMinimum = 0;
    int m_progressStepMaximum = 1000;

    CMakeConfig m_cmakeConfiguration;

    QSet<Utils::FileName> m_cmakeFiles;
    QList<ProjectExplorer::FileNode *> m_cmakeInputsFileNodes;

    QList<Project *> m_projects;
    QList<Target *> m_targets;
    QList<FileGroup *> m_fileGroups;

    CMakeParser m_parser;

#if defined(WITH_TESTS)
    friend class CMakeProjectPlugin;
#endif
};

} // namespace Internal
} // namespace CMakeProjectManager
