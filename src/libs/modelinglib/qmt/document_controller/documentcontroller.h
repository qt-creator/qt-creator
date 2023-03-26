// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include "qmt/infrastructure/qmt_global.h"
#include "qmt/model_controller/modelcontroller.h"

namespace qmt {

class ProjectController;
class UndoController;
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
class DReferences;
class MSelection;
class MObject;

class QMT_EXPORT DocumentController : public QObject
{
    Q_OBJECT
public:
    explicit DocumentController(QObject *parent = nullptr);
    ~DocumentController() override;

signals:
    void changed();

public:
    ProjectController *projectController() const { return m_projectController; }
    UndoController *undoController() const { return m_undoController; }
    ModelController *modelController() const { return m_modelController; }
    DiagramController *diagramController() const { return m_diagramController; }
    DiagramSceneController *diagramSceneController() const { return m_diagramSceneController; }
    StyleController *styleController() const { return m_styleController; }
    StereotypeController *stereotypeController() const { return m_stereotypeController; }
    ConfigController *configController() const { return m_configController; }
    TreeModel *treeModel() const { return m_treeModel; }
    SortedTreeModel *sortedTreeModel() const { return m_sortedTreeModel; }
    DiagramsManager *diagramsManager() const { return m_diagramsManager; }
    SceneInspector *sceneInspector() const { return m_sceneInspector; }

    bool hasDiagramSelection(const qmt::MDiagram *diagram) const;

    MContainer cutFromModel(const MSelection &selection);
    DContainer cutFromDiagram(MDiagram *diagram);
    MContainer copyFromModel(const MSelection &selection);
    DContainer copyFromDiagram(const MDiagram *diagram);
    void copyDiagram(const MDiagram *diagram);
    void pasteIntoModel(MObject *modelObject, const MReferences &container, ModelController::PasteOption option);
    void pasteIntoDiagram(MDiagram *diagram, const DReferences &container);
    void deleteFromModel(const MSelection &selection);
    void deleteFromDiagram(MDiagram *diagram);
    void removeFromDiagram(MDiagram *diagram);
    void selectAllOnDiagram(MDiagram *diagram);

    MPackage *createNewPackage(MPackage *);
    MClass *createNewClass(MPackage *);
    MComponent *createNewComponent(MPackage *);
    MCanvasDiagram *createNewCanvasDiagram(MPackage *);

    MDiagram *findRootDiagram();
    MDiagram *findOrCreateRootDiagram();

    void createNewProject(const QString &fileName);
    void loadProject(const QString &fileName);

private:
    ProjectController *m_projectController = nullptr;
    UndoController *m_undoController = nullptr;
    ModelController *m_modelController = nullptr;
    DiagramController *m_diagramController = nullptr;
    DiagramSceneController *m_diagramSceneController = nullptr;
    StyleController *m_styleController = nullptr;
    StereotypeController *m_stereotypeController = nullptr;
    ConfigController *m_configController = nullptr;
    TreeModel *m_treeModel = nullptr;
    SortedTreeModel *m_sortedTreeModel = nullptr;
    DiagramsManager *m_diagramsManager = nullptr;
    SceneInspector *m_sceneInspector = nullptr;
};

} // namespace qmt
