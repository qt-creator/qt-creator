// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <coreplugin/icontext.h>

#include <functional>

namespace Core { class IDocument; }
namespace Utils { class FilePath; }

namespace ProjectExplorer {
class BuildSystem;
class FileNode;
class FolderNode;
class Node;
class Project;
class ProjectNode;
class SessionNode;
class Target;

namespace Internal { class ProjectTreeWidget; }

class PROJECTEXPLORER_EXPORT ProjectTree : public QObject
{
    Q_OBJECT
public:
    explicit ProjectTree(QObject *parent = nullptr);
    ~ProjectTree() override;

    static ProjectTree *instance();

    static Project *currentProject();
    static Target *currentTarget();
    static BuildSystem *currentBuildSystem();
    static Node *currentNode();
    static Utils::FilePath currentFilePath();

    class CurrentNodeKeeper {
    public:
        CurrentNodeKeeper();
        ~CurrentNodeKeeper();
    private:
        const bool m_active = false;
    };

    enum ConstructionPhase {
        AsyncPhase,
        FinalPhase
    };

    // Integration with ProjectTreeWidget
    static void registerWidget(Internal::ProjectTreeWidget *widget);
    static void unregisterWidget(Internal::ProjectTreeWidget *widget);
    static void nodeChanged(Internal::ProjectTreeWidget *widget);

    static void aboutToShutDown();

    static void showContextMenu(Internal::ProjectTreeWidget *focus, const QPoint &globalPos, Node *node);

    static void highlightProject(Project *project, const QString &message);

    using TreeManagerFunction = std::function<void(FolderNode *, ConstructionPhase)>;
    static void registerTreeManager(const TreeManagerFunction &treeChange);
    static void applyTreeManager(FolderNode *folder, ConstructionPhase phase);

    // Nodes:
    static bool hasNode(const Node *node);
    static void forEachNode(const std::function<void(Node *)> &task);

    static Project *projectForNode(const Node *node);
    static Node *nodeForFile(const Utils::FilePath &fileName);

    static const QList<Node *> siblingsWithSameBaseName(const Node *fileNode);

    void expandCurrentNodeRecursively();

    void collapseAll();
    void expandAll();

    void changeProjectRootDirectory();

    // for nodes to emit signals, do not call unless you are a node
    static void emitSubtreeChanged(FolderNode *node);

signals:
    void currentProjectChanged(ProjectExplorer::Project *project);
    void currentNodeChanged(Node *node);
    void nodeActionsChanged();

    // Emitted whenever the model needs to send a update signal.
    void subtreeChanged(ProjectExplorer::FolderNode *node);

    void aboutToShowContextMenu(ProjectExplorer::Node *node);

    // Emitted on any change to the tree
    void treeChanged();

private:
    void sessionAndTreeChanged();
    void sessionChanged();
    void update();
    void updateFromProjectTreeWidget(Internal::ProjectTreeWidget *widget);
    void updateFromDocumentManager();
    void updateFromNode(Node *node);
    void setCurrent(Node *node, Project *project);
    void updateContext();

    void updateFromFocus();

    void updateFileWarning(Core::IDocument *document, const QString &text);
    static bool hasFocus(Internal::ProjectTreeWidget *widget);
    Internal::ProjectTreeWidget *currentWidget() const;
    void hideContextMenu();

private:
    static ProjectTree *s_instance;
    QList<QPointer<Internal::ProjectTreeWidget>> m_projectTreeWidgets;
    QVector<TreeManagerFunction> m_treeManagers;
    Node *m_currentNode = nullptr;
    Project *m_currentProject = nullptr;
    Internal::ProjectTreeWidget *m_focusForContextMenu = nullptr;
    int m_keepCurrentNodeRequests = 0;
    Core::Context m_lastProjectContext;
};

} // namespace ProjectExplorer
