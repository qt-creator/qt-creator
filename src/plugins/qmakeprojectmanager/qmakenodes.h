// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmakeprojectmanager_global.h"
#include "qmakeparsernodes.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectnodes.h>

namespace QmakeProjectManager {

class QmakeProFileNode;

// Implements ProjectNode for qmake .pri files
class QMAKEPROJECTMANAGER_EXPORT QmakePriFileNode : public ProjectExplorer::ProjectNode
{
public:
    QmakePriFileNode(QmakeBuildSystem *buildSystem, QmakeProFileNode *qmakeProFileNode,
                     const Utils::FilePath &filePath, QmakePriFile *pf);

    QmakePriFile *priFile() const;

    bool showInSimpleTree() const override { return false; }

    bool canAddSubProject(const Utils::FilePath &proFilePath) const override;
    bool addSubProject(const Utils::FilePath &proFilePath) override;
    bool removeSubProject(const Utils::FilePath &proFilePath) override;
    QStringList subProjectFileNamePatterns() const override;

    AddNewInformation addNewInformation(const Utils::FilePaths &files, Node *context) const override;

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
    AddNewInformation addNewInformation(const Utils::FilePaths &files, Node *context) const override;
    QVariant data(Utils::Id role) const override;
    bool setData(Utils::Id role, const QVariant &value) const override;

    QmakeProjectManager::ProjectType projectType() const;

    QStringList variableValue(const Variable var) const;
    QString singleVariableValue(const Variable var) const;

    TargetInformation targetInformation() const;

    bool showInSimpleTree(ProjectType projectType) const;
};

} // namespace QmakeProjectManager
