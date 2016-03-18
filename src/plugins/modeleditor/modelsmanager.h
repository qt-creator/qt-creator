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

namespace ProjectExplorer {
class Project;
class Node;
}

namespace qmt {
class Uid;
class MDiagram;
class DiagramsViewInterface;
}

namespace ModelEditor {
namespace Internal {

class ExtDocumentController;
class DiagramsViewManager;
class ModelDocument;

class ModelsManager :
        public QObject
{
    Q_OBJECT
    class ManagedModel;
    class ModelsManagerPrivate;

public:
    explicit ModelsManager(QObject *parent = 0);
    ~ModelsManager();

    ExtDocumentController *createModel(ModelDocument *modelDocument);
    void releaseModel(ExtDocumentController *documentController);
    void openDiagram(const qmt::Uid &modelUid, const qmt::Uid &diagramUid);

private slots:
    void onAboutToShowContextMenu(ProjectExplorer::Project *project, ProjectExplorer::Node *node);
    void onOpenDiagramFromProjectExplorer();
    void onOpenDefaultModel(const qmt::Uid &modelUid);

private:
    void openDiagram(ExtDocumentController *documentController, qmt::MDiagram *diagram);

private:
    ModelsManagerPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
