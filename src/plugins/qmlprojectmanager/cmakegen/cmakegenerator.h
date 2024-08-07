// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "cmakewriter.h"

#include "utils/filepath.h"
#include "projectexplorer/task.h"

#include <QObject>

namespace ProjectExplorer {
class FolderNode;
}

namespace QmlProjectManager {

class QmlProject;
class QmlBuildSystem;

namespace GenerateCmake {

class CMakeGenerator : public QObject
{
    Q_OBJECT

public:
    static void createMenuAction(QObject *parent);
    static void logIssue(ProjectExplorer::Task::TaskType type, const QString &text, const Utils::FilePath &file);

    CMakeGenerator(QmlBuildSystem *bs, QObject *parent = nullptr);

    QString projectName() const;

    const QmlProject *qmlProject() const;
    const QmlBuildSystem *buildSystem() const;

    bool findFile(const Utils::FilePath &file) const;
    bool isRootNode(const NodePtr &node) const;
    bool hasChildModule(const NodePtr &node) const;

    void setEnabled(bool enabled);
    void initialize(QmlProject *project);
    void update(const QSet<QString> &added, const QSet<QString> &removed);
    void updateMenuAction();

private:
    bool ignore(const Utils::FilePath &path) const;
    bool checkUri(const QString& uri, const Utils::FilePath &path) const;

    void createCMakeFiles(const NodePtr &node) const;
    void createSourceFiles() const;

    void readQmlDir(const Utils::FilePath &filePath, NodePtr &node) const;
    NodePtr findModuleFor(const NodePtr &node) const;
    NodePtr findNode(NodePtr &node, const Utils::FilePath &path) const;
    NodePtr findOrCreateNode(NodePtr &node, const Utils::FilePath &path) const;

    bool findFile(const NodePtr &node, const Utils::FilePath &file) const;
    void insertFile(NodePtr &node, const Utils::FilePath &path) const;
    void removeFile(NodePtr &node, const Utils::FilePath &path) const;

    void printModules(const NodePtr &generatorNode) const;
    void printNodeTree(const NodePtr &generatorNode, size_t indent = 0) const;

    void parseNodeTree(NodePtr &generatorNode, const ProjectExplorer::FolderNode *folderNode);
    void parseSourceTree();

    void compareWithFileSystem(const NodePtr &node) const;

    bool m_enabled = false;
    QmlBuildSystem *m_buildSystem = nullptr;
    CMakeWriter::Ptr m_writer = {};

    QString m_projectName = {};
    NodePtr m_root = {};
    QStringList m_moduleNames = {};
};

} // namespace GenerateCmake
} // namespace QmlProjectManager
