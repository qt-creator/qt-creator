// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "builddirparameters.h"
#include "cmakebuildtarget.h"
#include "cmakeprojectnodes.h"
#include "fileapidataextractor.h"

#include <projectexplorer/rawprojectpart.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QDateTime>

namespace ProjectExplorer { class ProjectNode; }

namespace CMakeProjectManager::Internal {

class FileApiReader final : public QObject
{
    Q_OBJECT

public:
    ~FileApiReader() override;

    void setParameters(const BuildDirParameters &p);

    void resetData();
    void parse(bool forceCMakeRun,
               bool forceInitialConfiguration,
               bool forceExtraConfiguration,
               bool debugging,
               bool profiling);
    void stop();
    void stopCMakeRun();

    bool isParsing() const;

    QList<CMakeBuildTarget> takeBuildTargets(QString &errorMessage);
    QSet<CMakeFileInfo> takeCMakeFileInfos(QString &errorMessage);
    CMakeConfig takeParsedConfiguration();
    QString ctestPath() const;
    ProjectExplorer::RawProjectParts createRawProjectParts(QString &errorMessage);

    bool isMultiConfig() const;
    bool usesAllCapsTargets() const;

    std::unique_ptr<CMakeProjectNode> rootProjectNode();

    Utils::FilePath topCmakeFile() const;

    QString cmakeGenerator() const;

signals:
    void configurationStarted() const;
    void dataAvailable(bool restoredFromBackup) const;
    void dirty() const;
    void errorOccurred(const QString &message) const;
    void debuggingStarted() const;
    void cancelCMakeRequested() const;

private:
    void handleReplyIndexFileChange(const Utils::FilePath &indexFile);
    bool makeBackupConfiguration(bool store);
    void writeConfigurationIntoBuildDirectory(const QStringList &configuration);
    void setupCMakeFileApi();

    // cmake data:
    CMakeConfig m_cache;
    QSet<CMakeFileInfo> m_cmakeFiles;
    QList<CMakeBuildTarget> m_buildTargets;
    ProjectExplorer::RawProjectParts m_projectParts;
    std::unique_ptr<CMakeProjectNode> m_rootProjectNode;
    QString m_ctestPath;
    QString m_cmakeGenerator;
    bool m_isMultiConfig = false;
    bool m_usesAllCapsTargets = false;
    bool m_lastCMakeFailed = false;

    BuildDirParameters m_parameters;

    // Notification on changes outside of creator:
    std::unique_ptr<Utils::FilePathWatcher> m_watcher;
    QDateTime m_lastReplyTimestamp;
    QtTaskTree::QSingleTaskTreeRunner m_taskTreeRunner;
};

} // CMakeProjectManager::Internal
