// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "compilationdatabaseutils.h"

#include <projectexplorer/buildsystem.h>

#include <utils/fileutils.h>

#include <QFutureWatcher>
#include <QObject>
#include <QStringList>

namespace ProjectExplorer {
class FileNode;
class TreeScanner;
}

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

    QList<ProjectExplorer::FileNode *> scannedFiles() const;
    DbContents dbContents() const
    {
        return m_dbContents;
    }

signals:
    void finished(ParseResult result);

private:
    void parserJobFinished();
    void finish(ParseResult result);

    const QString m_projectName;
    const Utils::FilePath m_projectFilePath;
    const Utils::FilePath m_rootPath;
    MimeBinaryCache &m_mimeBinaryCache;
    ProjectExplorer::TreeScanner *m_treeScanner = nullptr;
    QFutureWatcher<DbContents> m_parserWatcher;
    DbContents m_dbContents;
    QByteArray m_projectFileContents;
    QByteArray m_projectFileHash;
    int m_runningParserJobs = 0;

    ProjectExplorer::BuildSystem::ParseGuard m_guard;
};

} // namespace CompilationDatabaseProjectManager::Internal
