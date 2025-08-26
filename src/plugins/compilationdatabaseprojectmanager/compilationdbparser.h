// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "compilationdatabaseutils.h"

#include <projectexplorer/buildsystem.h>

#include <solutions/tasking/tasktreerunner.h>

#include <QObject>

namespace ProjectExplorer { class FileNode; }

namespace CompilationDatabaseProjectManager::Internal {

enum class ParseResult { Success, Failure, Cached };

class CompilationDbParser : public QObject
{
    Q_OBJECT

public:
    explicit CompilationDbParser(const QString &projectName,
                                 const Utils::FilePath &projectPath,
                                 const Utils::FilePath &rootPath,
                                 MimeBinaryCache &mimeBinaryCache,
                                 ProjectExplorer::BuildSystem::ParseGuard &&guard,
                                 QObject *parent = nullptr);

    void setPreviousProjectFileHash(const QByteArray &fileHash) { m_projectFileHash = fileHash; }
    QByteArray projectFileHash() const { return m_projectFileHash; }

    void start();
    void stop();

    QList<ProjectExplorer::FileNode *> scannedFiles() const { return m_scannedFiles; }
    DbContents dbContents() const { return m_dbContents; }

signals:
    void finished(ParseResult result);

private:
    void finish(ParseResult result);

    const QString m_projectName;
    const Utils::FilePath m_projectFilePath;
    const Utils::FilePath m_rootPath;
    MimeBinaryCache &m_mimeBinaryCache;
    QList<ProjectExplorer::FileNode *> m_scannedFiles;
    DbContents m_dbContents;
    QByteArray m_projectFileContents;
    QByteArray m_projectFileHash;

    ProjectExplorer::BuildSystem::ParseGuard m_guard;
    Tasking::SingleTaskTreeRunner m_taskTreeRunner;
};

} // namespace CompilationDatabaseProjectManager::Internal
