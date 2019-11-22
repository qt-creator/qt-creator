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

#include <nim/project/nimbuildsystem.h>

namespace Nim {

struct NimbleTask
{
    QString name;
    QString description;

    bool operator==(const NimbleTask &o) const {
        return name == o.name && description == o.description;
    }
};

struct NimbleMetadata
{
    QStringList bin;
    QString binDir;
    QString srcDir;

    bool operator==(const NimbleMetadata &o) const {
        return bin == o.bin && binDir == o.binDir && srcDir == o.srcDir;
    }
    bool operator!=(const NimbleMetadata &o) const {
        return  !operator==(o);
    }
};

class NimbleBuildSystem : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    NimbleBuildSystem(ProjectExplorer::Target *target);

    std::vector<NimbleTask> tasks() const;
    NimbleMetadata metadata() const;

signals:
    void tasksChanged();

private:
    void loadSettings();
    void saveSettings();

    void updateProject();

    bool supportsAction(ProjectExplorer::Node *,
                        ProjectExplorer::ProjectAction action,
                        const ProjectExplorer::Node *node) const override;
    bool addFiles(ProjectExplorer::Node *node,
                  const QStringList &filePaths, QStringList *) override;
    ProjectExplorer::RemovedFilesFromProject removeFiles(ProjectExplorer::Node *node,
                                                         const QStringList &filePaths,
                                                         QStringList *) override;
    bool deleteFiles(ProjectExplorer::Node *, const QStringList &) override;
    bool renameFile(ProjectExplorer::Node *,
                    const QString &filePath, const QString &newFilePath) override;

    void triggerParsing() final;

    NimbleMetadata m_metadata;
    std::vector<NimbleTask> m_tasks;

    NimProjectScanner m_projectScanner;
    ParseGuard m_guard;
};

} // Nim
