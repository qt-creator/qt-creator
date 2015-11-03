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

#ifndef QMT_DOCUMENTCONTROLLER_H
#define QMT_DOCUMENTCONTROLLER_H

#include <QObject>
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class ProjectController;
class UndoController;
class ModelController;
class DiagramController;
class DiagramSceneController;
class StyleController;
class StereotypeController;
class ConfigController;
class TreeModel;
class SortedTreeModel;
class DiagramsManager;
class SceneInspector;
class MPackage;
class MClass;
class MComponent;
class MDiagram;
class MCanvasDiagram;
class MContainer;
class DContainer;
class MSelection;
class MObject;


class QMT_EXPORT DocumentController :
        public QObject
{
    Q_OBJECT
public:
    explicit DocumentController(QObject *parent = 0);

    ~DocumentController();

signals:

    void changed();

    void modelClipboardChanged(bool is_empty);

    void diagramClipboardChanged(bool is_empty);

public:

    ProjectController *getProjectController() const { return m_projectController; }

    UndoController *getUndoController() const { return m_undoController; }

    ModelController *getModelController() const { return m_modelController; }

    DiagramController *getDiagramController() const { return m_diagramController; }

    DiagramSceneController *getDiagramSceneController() const { return m_diagramSceneController; }

    StyleController *getStyleController() const { return m_styleController; }

    StereotypeController *getStereotypeController() const { return m_stereotypeController; }

    ConfigController *getConfigController() const { return m_configController; }

    TreeModel *getTreeModel() const { return m_treeModel; }

    SortedTreeModel *getSortedTreeModel() const { return m_sortedTreeModel; }

    DiagramsManager *getDiagramsManager() const { return m_diagramsManager; }

    SceneInspector *getSceneInspector() const { return m_sceneInspector; }

public:

    bool isModelClipboardEmpty() const;

    bool isDiagramClipboardEmpty() const;

    bool hasDiagramSelection(const qmt::MDiagram *diagram) const;

public:

    void cutFromModel(const MSelection &selection);

    void cutFromDiagram(MDiagram *diagram);

    void copyFromModel(const MSelection &selection);

    void copyFromDiagram(const MDiagram *diagram);

    void copyDiagram(const MDiagram *diagram);

    void pasteIntoModel(MObject *model_object);

    void pasteIntoDiagram(MDiagram *diagram);

    void deleteFromModel(const MSelection &selection);

    void deleteFromDiagram(MDiagram *diagram);

    void removeFromDiagram(MDiagram *diagram);

    void selectAllOnDiagram(MDiagram *diagram);

public:

    MPackage *createNewPackage(MPackage *);

    MClass *createNewClass(MPackage *);

    MComponent *createNewComponent(MPackage *);

    MCanvasDiagram *createNewCanvasDiagram(MPackage *);

public:

    MDiagram *findRootDiagram();

    MDiagram *findOrCreateRootDiagram();

public:

    void createNewProject(const QString &file_name);

    void loadProject(const QString &file_name);

private:

    ProjectController *m_projectController;

    UndoController *m_undoController;

    ModelController *m_modelController;

    DiagramController *m_diagramController;

    DiagramSceneController *m_diagramSceneController;

    StyleController *m_styleController;

    StereotypeController *m_stereotypeController;

    ConfigController *m_configController;

    TreeModel *m_treeModel;

    SortedTreeModel *m_sortedTreeModel;

    DiagramsManager *m_diagramsManager;

    SceneInspector *m_sceneInspector;

    QScopedPointer<MContainer> m_modelClipboard;

    QScopedPointer<DContainer> m_diagramClipboard;

};

}

#endif // QMT_DOCUMENTCONTROLLER_H
