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
#include "qmt/infrastructure/qmt_global.h"

QT_BEGIN_NAMESPACE
class QPointF;
QT_END_NAMESPACE

namespace qmt {

class Uid;
class ModelController;
class DiagramController;
class MObject;
class MPackage;
class MDiagram;
class MRelation;
class DElement;
class DObject;
class DClass;
class DRelation;
class DSelection;
class IElementTasks;
class ISceneInspector;

class QMT_EXPORT DiagramSceneController : public QObject
{
    Q_OBJECT

    class AcceptRelationVisitor;

public:
    explicit DiagramSceneController(QObject *parent = 0);
    ~DiagramSceneController() override;

signals:
    void newElementCreated(DElement *element, MDiagram *diagram);
    void elementAdded(DElement *element, MDiagram *diagram);

public:
    ModelController *modelController() const { return m_modelController; }
    void setModelController(ModelController *modelController);
    DiagramController *diagramController() const { return m_diagramController; }
    void setDiagramController(DiagramController *diagramController);
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
                           const QList<QPointF> &intermediatePoints, MDiagram *diagram);
    bool relocateRelationEndA(DRelation *relation, DObject *targetObject);
    bool relocateRelationEndB(DRelation *relation, DObject *targetObject);

    bool isAddingAllowed(const Uid &modelElementKey, MDiagram *diagram);
    void addExistingModelElement(const Uid &modelElementKey, const QPointF &pos, MDiagram *diagram);
    void dropNewElement(const QString &newElementId, const QString &name, const QString &stereotype,
                        DElement *topMostElementAtPos, const QPointF &pos, MDiagram *diagram);
    void dropNewModelElement(MObject *modelObject, MPackage *parentPackage, const QPointF &pos,
                             MDiagram *diagram);

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
    void distributeHorizontal(DObject *object, const DSelection &selection, MDiagram *diagram);
    void distributeVertical(DObject *object, const DSelection &selection, MDiagram *diagram);
    void distributeField(DObject *object, const DSelection &selection, MDiagram *diagram);

private:
    void alignPosition(DObject *object, const DSelection &selection,
                       QPointF (*aligner)(DObject *object, DObject *otherObject),
                       MDiagram *diagram);
    void alignSize(DObject *object, const DSelection &selection, const QSizeF &minimumSize,
                   QRectF (*aligner)(DObject *, const QSizeF &), MDiagram *diagram);
    void alignOnRaster(DElement *element, MDiagram *diagram);

    DElement *addModelElement(const Uid &modelElementKey, const QPointF &pos, MDiagram *diagram);
    DObject *addObject(MObject *modelObject, const QPointF &pos, MDiagram *diagram);
    DRelation *addRelation(MRelation *modelRelation, const QList<QPointF> &intermediatePoints,
                           MDiagram *diagram);
    bool relocateRelationEnd(DRelation *relation, DObject *targetObject, Uid (MRelation::*endUid)() const,
                             void (MRelation::*setEndUid)(const Uid &));

    ModelController *m_modelController;
    DiagramController *m_diagramController;
    IElementTasks *m_elementTasks;
    ISceneInspector *m_sceneInspector;
};

} // namespace qmt
