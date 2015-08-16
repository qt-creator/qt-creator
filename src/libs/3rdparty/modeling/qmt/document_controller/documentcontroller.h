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

    ProjectController *getProjectController() const { return _project_controller; }

    UndoController *getUndoController() const { return _undo_controller; }

    ModelController *getModelController() const { return _model_controller; }

    DiagramController *getDiagramController() const { return _diagram_controller; }

    DiagramSceneController *getDiagramSceneController() const { return _diagram_scene_controller; }

    StyleController *getStyleController() const { return _style_controller; }

    StereotypeController *getStereotypeController() const { return _stereotype_controller; }

    ConfigController *getConfigController() const { return _config_controller; }

    TreeModel *getTreeModel() const { return _tree_model; }

    SortedTreeModel *getSortedTreeModel() const { return _sorted_tree_model; }

    DiagramsManager *getDiagramsManager() const { return _diagrams_manager; }

    SceneInspector *getSceneInspector() const { return _scene_inspector; }

public:

    bool isModelClipboardEmpty() const;

    bool isDiagramClipboardEmpty() const;

    bool hasCurrentDiagramSelection() const;

public:

    void cutFromModel(const MSelection &selection);

    Q_SLOT void cutFromCurrentDiagram();

    void copyFromModel(const MSelection &selection);

    Q_SLOT void copyFromCurrentDiagram();

    Q_SLOT void copyCurrentDiagram();

    void pasteIntoModel(MObject *model_object);

    Q_SLOT void pasteIntoCurrentDiagram();

    void deleteFromModel(const MSelection &selection);

    Q_SLOT void deleteFromCurrentDiagram();

    Q_SLOT void removeFromCurrentDiagram();

    Q_SLOT void selectAllOnCurrentDiagram();

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

    ProjectController *_project_controller;

    UndoController *_undo_controller;

    ModelController *_model_controller;

    DiagramController *_diagram_controller;

    DiagramSceneController *_diagram_scene_controller;

    StyleController *_style_controller;

    StereotypeController *_stereotype_controller;

    ConfigController *_config_controller;

    TreeModel *_tree_model;

    SortedTreeModel *_sorted_tree_model;

    DiagramsManager *_diagrams_manager;

    SceneInspector *_scene_inspector;

    QScopedPointer<MContainer> _model_clipboard;

    QScopedPointer<DContainer> _diagram_clipboard;

};

}

#endif // QMT_DOCUMENTCONTROLLER_H
