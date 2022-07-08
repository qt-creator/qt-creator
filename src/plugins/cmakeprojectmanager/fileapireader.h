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

#include "builddirparameters.h"
#include "cmakebuildtarget.h"
#include "cmakeprojectnodes.h"
#include "fileapidataextractor.h"

#include <projectexplorer/rawprojectpart.h>
#include <projectexplorer/treescanner.h>

#include <utils/filesystemwatcher.h>
#include <utils/optional.h>

#include <QDateTime>
#include <QFuture>
#include <QObject>

#include <memory>

namespace ProjectExplorer {
class ProjectNode;
}

namespace CMakeProjectManager {
namespace Internal {

class CMakeProcess;
class FileApiQtcData;

class FileApiReader final : public QObject
{
    Q_OBJECT

public:
    FileApiReader();
    ~FileApiReader();

    void setParameters(const BuildDirParameters &p);

    void resetData();
    void parse(bool forceCMakeRun, bool forceInitialConfiguration, bool forceExtraConfiguration);
    void stop();
    void stopCMakeRun();

    bool isParsing() const;

    QSet<Utils::FilePath> projectFilesToWatch() const;
    QList<CMakeBuildTarget> takeBuildTargets(QString &errorMessage);
    CMakeConfig takeParsedConfiguration(QString &errorMessage);
    QString ctestPath() const;
    ProjectExplorer::RawProjectParts createRawProjectParts(QString &errorMessage);

    bool isMultiConfig() const;
    bool usesAllCapsTargets() const;

    int lastCMakeExitCode() const;

    std::unique_ptr<CMakeProjectNode> rootProjectNode();

    Utils::FilePath topCmakeFile() const;

signals:
    void configurationStarted() const;
    void dataAvailable(bool restoredFromBackup) const;
    void dirty() const;
    void errorOccurred(const QString &message) const;

private:
    void startState();
    void endState(const Utils::FilePath &replyFilePath, bool restoredFromBackup);
    void startCMakeState(const QStringList &configurationArguments);
    void cmakeFinishedState();

    void replyDirectoryHasChanged(const QString &directory) const;
    void makeBackupConfiguration(bool store);

    void writeConfigurationIntoBuildDirectory(const QStringList &configuration);

    std::unique_ptr<CMakeProcess> m_cmakeProcess;

    // cmake data:
    CMakeConfig m_cache;
    QSet<CMakeFileInfo> m_cmakeFiles;
    QList<CMakeBuildTarget> m_buildTargets;
    ProjectExplorer::RawProjectParts m_projectParts;
    std::unique_ptr<CMakeProjectNode> m_rootProjectNode;
    QString m_ctestPath;
    bool m_isMultiConfig = false;
    bool m_usesAllCapsTargets = false;
    int m_lastCMakeExitCode = 0;

    Utils::optional<QFuture<std::shared_ptr<FileApiQtcData>>> m_future;

    // Update related:
    bool m_isParsing = false;
    BuildDirParameters m_parameters;

    // Notification on changes outside of creator:
    Utils::FileSystemWatcher m_watcher;
    QDateTime m_lastReplyTimestamp;
};

} // namespace Internal
} // namespace CMakeProjectManager
