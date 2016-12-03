/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <QElapsedTimer>
#include <QFileSystemWatcher>
#include <QFutureWatcher>
#include <QTimer>

namespace TextEditor { class TextDocument; }

namespace Nim {

class NimProjectManager;
class NimProjectNode;

class NimProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    NimProject(NimProjectManager *projectManager, const QString &fileName);

    QString displayName() const override;
    QStringList files(FilesMode) const override;
    bool needsConfiguration() const override;
    bool supportsKit(ProjectExplorer::Kit *k, QString *errorMessage) const override;
    Utils::FileNameList nimFiles() const;
    QVariantMap toMap() const override;

    bool addFiles(const QStringList &filePaths);
    bool removeFiles(const QStringList &filePaths);
    bool renameFile(const QString &filePath, const QString &newFilePath);

protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;

private:
    void scheduleProjectScan();
    void collectProjectFiles();
    void updateProject();

    QStringList m_files;
    QStringList m_excludedFiles;
    QFutureWatcher<QList<ProjectExplorer::FileNode *>> m_futureWatcher;
    QElapsedTimer m_lastProjectScan;
    QTimer m_projectScanTimer;
};

}
