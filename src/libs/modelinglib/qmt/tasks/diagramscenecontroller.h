// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include "qmt/infrastructure/qmt_global.h"

#include <functional>

QT_BEGIN_NAMESPACE
class QPointF;
QT_END_NAMESPACE

namespace qmt {

class Uid;
class ModelController;
class DiagramController;
class StereotypeController;
class MObject;
class MPackage;
class MDiagram;
class MRelation;
class MAssociation;
class MConnection;
class DElement;
class DObject;
class DClass;
class DRelation;
class DAssociation;
class DConnection;
class DSelection;
class IElementTasks;
class ISceneInspector;

class QMT_EXPORT DiagramSceneController : public QObject
{
    Q_OBJECT

    class AcceptRelationVisitor;

    enum RelationEnd {
        EndA,
        EndB
    };

public:
    explicit DiagramSceneController(QObject *parent = nullptr);
    ~DiagramSceneController() override;

signals:
    void newElementCreated(DElement *element, qmt::MDiagram *diagram);
    void elementAdded(DElement *element, qmt::MDiagram *diagram);

public:
    ModelController *modelController() const { return m_modelController; }
    void setModelController(ModelController *modelController);
    DiagramController *diagramController() const { return m_diagramController; }
    void setDiagramController(DiagramController *diagramController);
    StereotypeController *stereotypeController() const { return m_stereotypeController; }
    void setStereotypeController(StereotypeController *stereotypeController);
    IElementTasks *elementTasks() const { return m_elementTasks; }
    void setElementTasks(IElementTasks *elementTasks);
    ISceneInspector *sceneInspector() const { return m_sceneInspector; }
    void setSceneInspector(ISceneInspector *sceneInspector);

    void deleteFromDiagram(const DSelection &dselection, MDiagram *diagram);

    void createDependency(DObject *endAObject, DObject *endBObject,
                          const QList<QPointF> &intermediatePoints, MDiagram *diagram);
    void createInheritance(DClass *derivedClass, DClass *baseClass,
                           const QList<QPointF> &intermediatePoints, MDiagram *diagram);
    void createAssociation(DClass *endAClass, DClass *endBClass,
                           const QList<QPointF> &intermediatePoints, MDiagram *diagram,
                           std::function<void (MAssociation*, DAssociation*)> custom = nullptr);
    void createConnection(const QString &customRelationId, DObject *endAObject, DObject *endBObject,
                          const QList<QPointF> &intermediatePoints, MDiagram *diagram,
                          std::function<void (MConnection*, DConnection*)> custom = nullptr);
    bool relocateRelationEndA(DRelation *relation, DObject *targetObject);
    bool relocateRelationEndB(DRelation *relation, DObject *targetObject);

    bool isAddingAllowed(const Uid &modelElementKey, MDiagram *diagram);
    void addExistingModelElement(const Uid &modelElementKey, const QPointF &pos, MDiagram *diagram);
    void dropNewElement(const QString &newElementId, const QString &name, const QString &stereotype,
                        DElement *topMostElementAtPos, const QPointF &pos, MDiagram *diagram, const QPoint &viewPos, const QSize &viewSize);
    void dropNewModelElement(MObject *modelObject, MPackage *parentPackage, const QPointF &pos,
                             MDiagram *diagram);
    void addRelatedElements(const DSelection &selection, MDiagram *diagram);

    MPackage *findSuitableParentPackage(DElement *topmostDiagramElement, MDiagram *diagram);
    MDiagram *findDiagramBySearchId(MPackage *package, const QString &diagramName);

    void alignLeft(DObject *object, const DSelection &selection, MDiagram *diagram);
    void alignRight(DObject *object, const DSelection &selection, MDiagram *diagram);
    void alignHCenter(DObject *object, const DSelection &selection, MDiagram *diagram);
    void alignTop(DObject *object, const DSelection &selection, MDiagram *diagram);
    void alignBottom(DObject *object, const DSelection &selection, MDiagram *diagram);
    void alignVCenter(DObject *object, const DSelection &selection, MDiagram *diagram);
    void alignWidth(DObject *object, const DSelection &selection, const QSizeF &minimumSize,
                    MDiagram *diagram);
    void alignHeight(DObject *object, const DSelection &selection, const QSizeF &minimumSize,
                     MDiagram *diagram);
    void alignSize(DObject *object, const DSelection &selection, const QSizeF &minimumSize,
                   MDiagram *diagram);
    void alignHCenterDistance(const DSelection &selection, MDiagram *diagram);
    void alignVCenterDistance(const DSelection &selection, MDiagram *diagram);
    void alignHBorderDistance(const DSelection &selection, MDiagram *diagram);
    void alignVBorderDistance(const DSelection &selection, MDiagram *diagram);

private:
    void alignPosition(DObject *object, const DSelection &selection,
                       QPointF (*aligner)(DObject *object, DObject *otherObject),
                       MDiagram *diagram);
    void alignSize(DObject *object, const DSelection &selection, const QSizeF &minimumSize,
                   QRectF (*aligner)(DObject *, const QSizeF &), MDiagram *diagram);
    void alignOnRaster(DElement *element, MDiagram *diagram);

    QList<DObject *> collectObjects(const DSelection &selection, MDiagram *diagram);

    DElement *addModelElement(const Uid &modelElementKey, const QPointF &pos, MDiagram *diagram);
    DObject *addObject(MObject *modelObject, const QPointF &pos, MDiagram *diagram);
    DRelation *addRelation(MRelation *modelRelation, const QList<QPointF> &intermediatePoints,
                           MDiagram *diagram);
    bool relocateRelationEnd(DRelation *relation, DObject *targetObject, RelationEnd relationEnd,
                             Uid (MRelation::*endUid)() const, void (MRelation::*setEndUid)(const Uid &));

    ModelController *m_modelController = nullptr;
    DiagramController *m_diagramController = nullptr;
    StereotypeController *m_stereotypeController = nullptr;
    IElementTasks *m_elementTasks = nullptr;
    ISceneInspector *m_sceneInspector = nullptr;
};

} // namespace qmt
