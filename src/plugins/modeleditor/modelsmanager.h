/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MODELSMANAGER_H
#define MODELSMANAGER_H

#include <QObject>

namespace ProjectExplorer {
class Project;
class Node;
}

namespace qmt {
class Uid;
class MDiagram;
}

namespace ModelEditor {
namespace Internal {

class ExtDocumentController;
class DiagramsViewManager;
class ModelDocument;
class DiagramDocument;

class ModelsManager :
        public QObject
{
    Q_OBJECT
    class ManagedModel;
    class ModelsManagerPrivate;

public:
    explicit ModelsManager(QObject *parent = 0);
    ~ModelsManager();

    bool isDiagramOpen(const qmt::Uid &modelUid, const qmt::Uid &diagramUid) const;

    ExtDocumentController *createModel(ModelDocument *findModelDocument);
    ExtDocumentController *findModelByFileName(const QString &fileName,
                                               ModelDocument *findModelDocument);
    ExtDocumentController *findOrLoadModel(const qmt::Uid &modelUid,
                                           DiagramDocument *diagramDocument);
    void release(ExtDocumentController *documentController, ModelDocument *findModelDocument);
    void release(ExtDocumentController *documentController, DiagramDocument *diagramDocument);
    DiagramsViewManager *findDiagramsViewManager(ExtDocumentController *documentController) const;
    ModelDocument *findModelDocument(ExtDocumentController *documentController) const;
    QList<ExtDocumentController *> collectAllDocumentControllers() const;
    void openDiagram(const qmt::Uid &modelUid, const qmt::Uid &diagramUid);

private slots:
    void onOpenEditor(ExtDocumentController *documentController, const qmt::MDiagram *diagram);
    void onCloseEditor(ExtDocumentController *documentController, const qmt::MDiagram *diagram);
    void onCloseDocumentsLater();
    void onDiagramRenamed(ExtDocumentController *documentController, const qmt::MDiagram *diagram);
    void onAboutToShowContextMenu(ProjectExplorer::Project *project, ProjectExplorer::Node *node);
    void onOpenDiagram();
    void onOpenDefaultModel(const qmt::Uid &modelUid);

private:
    ModelsManagerPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor

#endif // MODELSMANAGER_H
