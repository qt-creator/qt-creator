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

#include "cmakecbpparser.h"
#include "cmakeconfigitem.h"

#include <projectexplorer/task.h>

#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <utils/fileutils.h>

#include <QByteArray>
#include <QFutureInterface>
#include <QObject>
#include <QSet>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QTemporaryDir);
QT_FORWARD_DECLARE_CLASS(QFileSystemWatcher);

namespace ProjectExplorer {
class FileNode;
class IOutputParser;
class Kit;
class Task;
} // namespace ProjectExplorer

namespace CMakeProjectManager {

class CMakeTool;

namespace Internal {

class CMakeBuildConfiguration;

class BuildDirManager : public QObject
{
    Q_OBJECT

public:
    BuildDirManager(CMakeBuildConfiguration *bc);
    ~BuildDirManager() override;

    const ProjectExplorer::Kit *kit() const;
    const Utils::FileName buildDirectory() const;
    const Utils::FileName workDirectory() const;
    const Utils::FileName sourceDirectory() const;
    const CMakeConfig intendedConfiguration() const;
    bool isParsing() const;

    void cmakeFilesChanged();

    void parse();
    void clearCache();
    void forceReparse();
    void maybeForceReparse(); // Only reparse if the configuration has changed...
    void resetData();
    bool persistCMakeState();

    QString projectName() const;
    QList<CMakeBuildTarget> buildTargets() const;
    QList<ProjectExplorer::FileNode *> files();
    QSet<Utils::FileName> cmakeFiles();
    void clearFiles();
    CMakeConfig parsedConfiguration() const;

    void checkConfiguration();

    static CMakeConfig parseConfiguration(const Utils::FileName &cacheFile,
                                          QString *errorMessage);

signals:
    void configurationStarted() const;
    void dataAvailable() const;
    void errorOccured(const QString &err) const;

private:
    void stopProcess();
    void cleanUpProcess();
    void extractData();

    void startCMake(CMakeTool *tool, const QString &generator, const CMakeConfig &config);

    void cmakeFinished(int code, QProcess::ExitStatus status);
    void processCMakeOutput();
    void processCMakeError();

    bool m_hasData = false;

    CMakeBuildConfiguration *m_buildConfiguration = nullptr;
    Utils::QtcProcess *m_cmakeProcess = nullptr;
    QTemporaryDir *m_tempDir = nullptr;
    mutable CMakeConfig m_cmakeCache;

    QSet<Utils::FileName> m_cmakeFiles;
    QString m_projectName;
    QList<CMakeBuildTarget> m_buildTargets;
    QList<ProjectExplorer::FileNode *> m_files;

    // For error reporting:
    ProjectExplorer::IOutputParser *m_parser = nullptr;
    QFutureInterface<void> *m_future = nullptr;

    QTimer m_reparseTimer;
};

} // namespace Internal
} // namespace CMakeProjectManager
