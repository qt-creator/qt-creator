/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

    void addExplorerNode(const ProjectExplorer::Node *node, qmt::DElement *topMostElementAtPos,
                         const QPointF &pos, qmt::MDiagram *diagram);
    bool hasDiagramForExplorerNode(const ProjectExplorer::Node *node);
    qmt::MDiagram *findDiagramForExplorerNode(const ProjectExplorer::Node *node);

private:
    void onMenuActionTriggered(MenuAction *action, const ProjectExplorer::Node *node,
                               qmt::DElement *topMostElementAtPos, const QPointF &pos,
                               qmt::MDiagram *diagram);

    void parseFullClassName(qmt::MClass *klass, const QString &fullClassName);

    PxNodeControllerPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
