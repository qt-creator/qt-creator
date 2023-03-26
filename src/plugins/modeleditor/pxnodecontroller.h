// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace ProjectExplorer { class Node; }

namespace qmt {
class MClass;
class MDiagram;
class DElement;
class DiagramSceneController;
}

namespace ModelEditor {
namespace Internal {

class ComponentViewController;

class PxNodeController :
        public QObject
{
    Q_OBJECT
    class PxNodeControllerPrivate;
    class MenuAction;

public:
    explicit PxNodeController(QObject *parent = nullptr);
    ~PxNodeController();

    ComponentViewController *componentViewController() const;

    void setDiagramSceneController(qmt::DiagramSceneController *diagramSceneController);
    void setAnchorFolder(const QString &anchorFolder);

    void addFileSystemEntry(const QString &filePath, int line, int column, qmt::DElement *topMostElementAtPos,
                            const QPointF &pos, qmt::MDiagram *diagram);
    bool hasDiagramForExplorerNode(const ProjectExplorer::Node *node);
    qmt::MDiagram *findDiagramForExplorerNode(const ProjectExplorer::Node *node);

private:
    void onMenuActionTriggered(MenuAction *action, const QString &filePath,
                               qmt::DElement *topMostElementAtPos, const QPointF &pos,
                               qmt::MDiagram *diagram);

    void parseFullClassName(qmt::MClass *klass, const QString &fullClassName);

    PxNodeControllerPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
