// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "utils/filepath.h"

#include <QObject>

namespace ProjectExplorer {
class FolderNode;
}

namespace QmlProjectManager {

class QmlProject;
class QmlBuildSystem;

namespace GenerateCmake {

// TODO:
// - Create "module" for src dir
// - Replace AppName in templates with ${CMAKE_PROJECT_NAME}
// - Introduce Blacklist (designer)

class CMakeGenerator : public QObject
{
    Q_OBJECT

public:
    CMakeGenerator(QmlBuildSystem *bs, QObject *parent = nullptr);

    void setEnabled(bool enabled);

    void initialize(QmlProject *project);

    void update(const QSet<QString> &added, const QSet<QString> &removed);

private:
    struct Node
    {
        std::shared_ptr<Node> parent = nullptr;
        bool module = false;

        QString uri;
        QString name;
        Utils::FilePath dir;

        std::vector<std::shared_ptr<Node>> subdirs;
        std::vector<Utils::FilePath> files;
        std::vector<Utils::FilePath> singletons;
        std::vector<Utils::FilePath> resources;
        std::vector<Utils::FilePath> sources;
    };

    using NodePtr = std::shared_ptr<Node>;

    std::vector<Utils::FilePath> qmlFiles(const NodePtr &node) const;
    std::vector<Utils::FilePath> singletons(const NodePtr &node) const;
    std::vector<Utils::FilePath> resources(const NodePtr &node) const;
    std::vector<Utils::FilePath> sources(const NodePtr &node) const;

    void createCMakeFiles(const NodePtr &node) const;

    void createQmlModuleFile(const NodePtr &node) const;
    void createMainCMakeFile(const NodePtr &node) const;
    void createModuleCMakeFile(const NodePtr &node) const;

    void createEntryPoints(const NodePtr &node) const;
    void createMainCppFile(const NodePtr &node) const;
    void writeFile(const Utils::FilePath &path, const QString &content) const;

    QString makeEnvironmentVariable(const QString &key) const;

    QString readTemplate(const QString &templatePath) const;
    void readQmlDir(const Utils::FilePath &filePath, NodePtr &node) const;

    NodePtr findModuleFor(const NodePtr &node) const;
    NodePtr findNode(NodePtr &node, const Utils::FilePath &path) const;
    NodePtr findOrCreateNode(NodePtr &node, const Utils::FilePath &path) const;

    void insertFile(NodePtr &node, const Utils::FilePath &path) const;
    void removeFile(NodePtr &node, const Utils::FilePath &path) const;

    bool hasChildModule(const NodePtr &node) const;
    bool isResource(const Utils::FilePath &path) const;

    void printNodeTree(const NodePtr &generatorNode, size_t indent = 0) const;

    void parseNodeTree(NodePtr &generatorNode, const ProjectExplorer::FolderNode *folderNode);
    void parseSourceTree();

    bool m_enabled = false;
    QString m_projectName = {};
    NodePtr m_root = {};
    QStringList m_srcs = {};
    std::vector<QString> m_moduleNames = {};
    QmlBuildSystem *m_buildSystem = nullptr;
};

} // namespace GenerateCmake
} // namespace QmlProjectManager
