// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "filegenerator.h"
#include "cmakewriter.h"

#include "utils/filepath.h"

#include <QObject>

namespace ProjectExplorer {
class FolderNode;
}

namespace QmlProjectManager {

class QmlProject;
class QmlBuildSystem;

namespace QmlProjectExporter {

class CMakeGenerator : public FileGenerator
{
    Q_OBJECT

public:
    static void createMenuAction(QObject *parent);

    CMakeGenerator(QmlBuildSystem *bs);

    void updateProject(QmlProject *project) override;
    void updateMenuAction() override;

    QString projectName() const;
    bool findFile(const Utils::FilePath &file) const;
    bool isRootNode(const NodePtr &node) const;
    bool hasChildModule(const NodePtr &node) const;

    void update(const QSet<QString> &added, const QSet<QString> &removed);

private:
    bool ignore(const Utils::FilePath &path) const;
    bool checkUri(const QString& uri, const Utils::FilePath &path) const;
    bool isMockModule(const NodePtr &node) const;

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

    CMakeWriter::Ptr m_writer = {};

    QString m_projectName = {};
    NodePtr m_root = {};
    QStringList m_moduleNames = {};
};

} // namespace QmlProjectExporter
} // namespace QmlProjectManager
