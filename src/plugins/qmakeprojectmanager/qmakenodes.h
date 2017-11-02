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

#include "qmakeprojectmanager_global.h"
#include "qmakeparsernodes.h"

#include <projectexplorer/projectnodes.h>

namespace Utils { class FileName; }
namespace ProjectExplorer { class RunConfiguration; }

namespace QmakeProjectManager {
class QmakeProFileNode;
class QmakeProject;

// Implements ProjectNode for qmake .pri files
class QMAKEPROJECTMANAGER_EXPORT QmakePriFileNode : public ProjectExplorer::ProjectNode
{
public:
    QmakePriFileNode(QmakeProject *project, QmakeProFileNode *qmakeProFileNode,
                     const Utils::FileName &filePath);

    QmakePriFile *priFile() const;

    // ProjectNode interface
    bool supportsAction(ProjectExplorer::ProjectAction action, const Node *node) const override;

    bool showInSimpleTree() const override { return false; }

    bool canAddSubProject(const QString &proFilePath) const override;

    bool addSubProject(const QString &proFilePath) override;
    bool removeSubProject(const QString &proFilePath) override;

    bool addFiles(const QStringList &filePaths, QStringList *notAdded = nullptr) override;
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = nullptr) override;
    bool deleteFiles(const QStringList &filePaths) override;
    bool canRenameFile(const QString &filePath, const QString &newFilePath) override;
    bool renameFile(const QString &filePath, const QString &newFilePath) override;
    AddNewInformation addNewInformation(const QStringList &files, Node *context) const override;

    bool deploysFolder(const QString &folder) const override;
    QList<ProjectExplorer::RunConfiguration *> runConfigurations() const override;

    QmakeProFileNode *proFileNode() const;

protected:
    QmakeProject *m_project = nullptr;

private:
    QmakeProFileNode *m_qmakeProFileNode = nullptr;
};

// Implements ProjectNode for qmake .pro files
class QMAKEPROJECTMANAGER_EXPORT QmakeProFileNode : public QmakePriFileNode
{
public:
    QmakeProFileNode(QmakeProject *project, const Utils::FileName &filePath);

    QmakeProFile *proFile() const;

    bool showInSimpleTree() const override;

    AddNewInformation addNewInformation(const QStringList &files, Node *context) const override;

    QmakeProjectManager::ProjectType projectType() const;
    QString buildDir() const;

    QStringList variableValue(const Variable var) const;
    QString singleVariableValue(const Variable var) const;

    QmakeProFileNode *findProFileFor(const Utils::FileName &string) const;

    bool showInSimpleTree(ProjectType projectType) const;
};

} // namespace QmakeProjectManager
