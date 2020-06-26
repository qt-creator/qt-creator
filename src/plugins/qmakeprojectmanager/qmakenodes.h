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

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectnodes.h>

namespace Utils { class FilePath; }

namespace QmakeProjectManager {
class QmakeProFileNode;
class QmakeProject;

// Implements ProjectNode for qmake .pri files
class QMAKEPROJECTMANAGER_EXPORT QmakePriFileNode : public ProjectExplorer::ProjectNode
{
public:
    QmakePriFileNode(QmakeBuildSystem *buildSystem, QmakeProFileNode *qmakeProFileNode,
                     const Utils::FilePath &filePath, QmakePriFile *pf);

    QmakePriFile *priFile() const;

    bool showInSimpleTree() const override { return false; }

    bool canAddSubProject(const QString &proFilePath) const override;
    bool addSubProject(const QString &proFilePath) override;
    bool removeSubProject(const QString &proFilePath) override;
    QStringList subProjectFileNamePatterns() const override;

    AddNewInformation addNewInformation(const QStringList &files, Node *context) const override;

    bool deploysFolder(const QString &folder) const override;

    QmakeProFileNode *proFileNode() const;

protected:
    QPointer<QmakeBuildSystem> m_buildSystem;

private:
    QmakeProFileNode *m_qmakeProFileNode = nullptr;
    QmakePriFile *m_qmakePriFile = nullptr;
};

// Implements ProjectNode for qmake .pro files
class QMAKEPROJECTMANAGER_EXPORT QmakeProFileNode : public QmakePriFileNode
{
public:
    QmakeProFileNode(QmakeBuildSystem *buildSystem, const Utils::FilePath &filePath, QmakeProFile *pf);

    QmakeProFile *proFile() const;

    QString makefile() const;
    QString objectsDirectory() const;
    QString objectExtension() const;

    bool isDebugAndRelease() const;
    bool isObjectParallelToSource() const;
    bool isQtcRunnable() const;
    bool includedInExactParse() const;

    bool showInSimpleTree() const override;

    QString buildKey() const override;
    bool parseInProgress() const override;
    bool validParse() const override;

    void build() override;

    QStringList targetApplications() const override;
    AddNewInformation addNewInformation(const QStringList &files, Node *context) const override;
    QVariant data(Utils::Id role) const override;
    bool setData(Utils::Id role, const QVariant &value) const override;

    QmakeProjectManager::ProjectType projectType() const;

    QStringList variableValue(const Variable var) const;
    QString singleVariableValue(const Variable var) const;

    TargetInformation targetInformation() const;

    bool showInSimpleTree(ProjectType projectType) const;
};

} // namespace QmakeProjectManager
