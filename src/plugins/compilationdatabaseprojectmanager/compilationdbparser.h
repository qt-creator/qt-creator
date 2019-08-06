/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "compilationdatabaseutils.h"

#include <projectexplorer/project.h>

#include <utils/fileutils.h>

#include <QFutureWatcher>
#include <QList>
#include <QObject>
#include <QStringList>

#include <vector>

namespace ProjectExplorer {
class FileNode;
class TreeScanner;
}

namespace CompilationDatabaseProjectManager {
namespace Internal {

class CompilationDbParser : public QObject
{
    Q_OBJECT
public:
    explicit CompilationDbParser(const QString &projectName,
                                 const Utils::FilePath &projectPath,
                                 const Utils::FilePath &rootPath,
                                 MimeBinaryCache &mimeBinaryCache,
                                 ProjectExplorer::Project::ParseGuard &&guard,
                                 QObject *parent = nullptr);

    void start();
    void stop();

    QList<ProjectExplorer::FileNode *> scannedFiles() const;
    DbContents dbContents() const
    {
        m_guard.markAsSuccess();
        return m_dbContents;
    }

signals:
    void finished(bool success);

private:
    void finish();
    DbContents parseProject();

    const QString m_projectName;
    const Utils::FilePath m_projectFilePath;
    const Utils::FilePath m_rootPath;
    MimeBinaryCache &m_mimeBinaryCache;
    ProjectExplorer::TreeScanner *m_treeScanner = nullptr;
    QFutureWatcher<DbContents> m_parserWatcher;
    DbContents m_dbContents;

    ProjectExplorer::Project::ParseGuard m_guard;
};

} // namespace Internal
} // namespace CompilationDatabaseProjectManager
