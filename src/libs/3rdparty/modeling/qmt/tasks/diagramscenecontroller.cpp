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

#include "diagramscenecontroller.h"

#include "qmt/controller/namecontroller.h"
#include "qmt/controller/undocontroller.h"
#include "qmt/diagram_controller/dfactory.h"
#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram_controller/dselection.h"
#include "qmt/diagram/dannotation.h"
#include "qmt/diagram/dboundary.h"
#include "qmt/diagram/dclass.h"
#include "qmt/diagram/dpackage.h"
#include "qmt/diagram/ditem.h"
#include "qmt/diagram/drelation.h"
#include "qmt/diagram_ui/diagram_mime_types.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/model_controller/mselection.h"
#include "qmt/model/massociation.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/model/mclass.h"
#include "qmt/model/mcomponent.h"
#include "qmt/model/mdependency.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/minheritance.h"
#include "qmt/model/mitem.h"
#include "qmt/model/mobject.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/msourceexpansion.h"
#include "qmt/tasks/alignonrastervisitor.h"
#include "qmt/tasks/isceneinspector.h"
#include "qmt/tasks/voidelementtasks.h"

#include <QMenu>
#include <QFileInfo>
#include <QDir>
#include <QQueue>
#include <QPair>

namespace qmt {

namespace {
static VoidElementTasks dummyElementTasks;
}

DiagramSceneController::DiagramSceneController(QObject *parent)
    : QObject(parent),
      m_modelController(0),
      m_diagramController(0),
      m_elementTasks(&dummyElementTasks),
      m_sceneInspector(0)
{
}

DiagramSceneController::~DiagramSceneController()
{
}

void DiagramSceneController::setModelController(ModelController *modelController)
{
    if (m_modelController == modelController)
        return;
    if (m_modelController) {
        disconnect(m_modelController, 0, this, 0);
        m_modelController = 0;
    }
    if (modelController)
        m_modelController = modelController;
}

void DiagramSceneController::setDiagramController(DiagramController *diagramController)
{
    if (m_diagramController == diagramController)
        return;
    if (m_diagramController) {
        disconnect(m_diagramController, 0, this, 0);
        m_diagramController = 0;
    }
    if (diagramController)
        m_diagramController = diagramController;
}

void DiagramSceneController::setElementTasks(IElementTasks *elementTasks)
{
    m_elementTasks = elementTasks;
}

void DiagramSceneController::setSceneInspector(ISceneInspector *sceneInspector)
{
    m_sceneInspector = sceneInspector;
}

void DiagramSceneController::deleteFromDiagram(const DSelection &dselection, MDiagram *diagram)
{
    if (!dselection.isEmpty()) {
        MSelection mselection;
        DSelection remainingDselection;
        foreach (const DSelection::Index &index, dselection.indices()) {
            DElement *delement = m_diagramController->findElement(index.elementKey(), diagram);
            QMT_CHECK(delement);
            if (delement->modelUid().isValid()) {
                MElement *melement = m_modelController->findElement(delement->modelUid());
                QMT_CHECK(melement);
                if (melement->owner())
                    mselection.append(melement->uid(), melement->owner()->uid());
            } else {
                remainingDselection.append(index);
            }
        }
        if (!remainingDselection.isEmpty())
            m_diagramController->deleteElements(remainingDselection, diagram);
        if (!mselection.isEmpty())
            m_modelController->deleteElements(mselection);
    }
}

void DiagramSceneController::createDependency(DObject *endAObject, DObject *endBObject,
                                              const QList<QPointF> &intermediatePoints, MDiagram *diagram)
{
    m_diagramController->undoController()->beginMergeSequence(tr("Create Dependency"));

    MObject *endAModelObject = m_modelController->findObject<MObject>(endAObject->modelUid());
    QMT_CHECK(endAModelObject);
    MObject *endBModelObject = m_modelController->findObject<MObject>(endBObject->modelUid());
    QMT_CHECK(endBModelObject);

    if (endAModelObject == endBModelObject)
        return;

    auto modelDependency = new MDependency();
    modelDependency->setEndAUid(endAModelObject->uid());
    modelDependency->setEndBUid(endBModelObject->uid());
    modelDependency->setDirection(MDependency::AToB);
    m_modelController->addRelation(endAModelObject, modelDependency);

    DRelation *relation = addRelation(modelDependency, intermediatePoints, diagram);

    m_diagramController->undoController()->endMergeSequence();

    if (relation)
        emit newElementCreated(relation, diagram);
}

void DiagramSceneController::createInheritance(DClass *derivedClass, DClass *baseClass,
                                               const QList<QPointF> &intermediatePoints, MDiagram *diagram)
{
    m_diagramController->undoController()->beginMergeSequence(tr("Create Inheritance"));

    MClass *derivedModelClass = m_modelController->findObject<MClass>(derivedClass->modelUid());
    QMT_CHECK(derivedModelClass);
    MClass *baseModelClass = m_modelController->findObject<MClass>(baseClass->modelUid());
    QMT_CHECK(baseModelClass);

    if (derivedModelClass == baseModelClass)
        return;

    auto modelInheritance = new MInheritance();
    modelInheritance->setDerived(derivedModelClass->uid());
    modelInheritance->setBase(baseModelClass->uid());
    m_modelController->addRelation(derivedModelClass, modelInheritance);

    DRelation *relation = addRelation(modelInheritance, intermediatePoints, diagram);

    m_diagramController->undoController()->endMergeSequence();

    if (relation)
        emit newElementCreated(relation, diagram);
}

void DiagramSceneController::createAssociation(DClass *endAClass, DClass *endBClass,
                                               const QList<QPointF> &intermediatePoints, MDiagram *diagram)
{
    m_diagramController->undoController()->beginMergeSequence(tr("Create Association"));

    MClass *endAModelObject = m_modelController->findObject<MClass>(endAClass->modelUid());
    QMT_CHECK(endAModelObject);
    MClass *endBModelObject = m_modelController->findObject<MClass>(endBClass->modelUid());
    QMT_CHECK(endBModelObject);

    // TODO allow self assignment with just one intermediate point and a nice round arrow
    if (endAModelObject == endBModelObject && intermediatePoints.count() < 2)
        return;

    auto modelAssociation = new MAssociation();
    modelAssociation->setEndAUid(endAModelObject->uid());
    MAssociationEnd endA = modelAssociation->endA();
    endA.setNavigable(true);
    modelAssociation->setEndA(endA);
    modelAssociation->setEndBUid(endBModelObject->uid());
    m_modelController->addRelation(endAModelObject, modelAssociation);

    DRelation *relation = addRelation(modelAssociation, intermediatePoints, diagram);

    m_diagramController->undoController()->endMergeSequence();

    if (relation)
        emit newElementCreated(relation, diagram);
}

bool DiagramSceneController::isAddingAllowed(const Uid &modelElementKey, MDiagram *diagram)
{
    MElement *modelElement = m_modelController->findElement(modelElementKey);
    QMT_CHECK(modelElement);
    if (m_diagramController->hasDelegate(modelElement, diagram))
        return false;
    if (auto modelRelation = dynamic_cast<MRelation *>(modelElement)) {
        MObject *endAModelObject = m_modelController->findObject(modelRelation->endAUid());
        QMT_CHECK(endAModelObject);
        DObject *endADiagramObject = m_diagramController->findDelegate<DObject>(endAModelObject, diagram);
        if (!endADiagramObject)
            return false;

        MObject *endBModelObject = m_modelController->findObject(modelRelation->endBUid());
        QMT_CHECK(endBModelObject);
        DObject *endBDiagramObject = m_diagramController->findDelegate<DObject>(endBModelObject, diagram);
        if (!endBDiagramObject)
            return false;
    }
    return true;
}

void DiagramSceneController::addExistingModelElement(const Uid &modelElementKey, const QPointF &pos, MDiagram *diagram)
{
    DElement *element = addModelElement(modelElementKey, pos, diagram);
    if (element)
        emit elementAdded(element, diagram);
}

void DiagramSceneController::dropNewElement(const QString &newElementId, const QString &name, const QString &stereotype,
                                            DElement *topMostElementAtPos, const QPointF &pos, MDiagram *diagram)
{
    if (newElementId == QLatin1String(ELEMENT_TYPE_ANNOTATION)) {
        auto annotation = new DAnnotation();
        annotation->setText(QStringLiteral(""));
        annotation->setPos(pos - QPointF(10.0, 10.0));
        m_diagramController->addElement(annotation, diagram);
        alignOnRaster(annotation, diagram);
        emit newElementCreated(annotation, diagram);
    } else if (newElementId == QLatin1String(ELEMENT_TYPE_BOUNDARY)) {
        auto boundary = new DBoundary();
        boundary->setPos(pos);
        m_diagramController->addElement(boundary, diagram);
        alignOnRaster(boundary, diagram);
        emit newElementCreated(boundary, diagram);
    } else {
        MPackage *parentPackage = findSuitableParentPackage(topMostElementAtPos, diagram);
        MObject *newObject = 0;
        QString newName;
        if (newElementId == QLatin1String(ELEMENT_TYPE_PACKAGE)) {
            auto package = new MPackage();
            newName = tr("New Package");
            if (!stereotype.isEmpty())
                package->setStereotypes(QStringList() << stereotype);
            newObject = package;
        } else if (newElementId == QLatin1String(ELEMENT_TYPE_COMPONENT)) {
            auto component = new MComponent();
            newName = tr("New Component");
            if (!stereotype.isEmpty())
                component->setStereotypes(QStringList() << stereotype);
            newObject = component;
        } else if (newElementId == QLatin1String(ELEMENT_TYPE_CLASS)) {
            auto klass = new MClass();
            newName = tr("New Class");
            if (!stereotype.isEmpty())
                klass->setStereotypes(QStringList() << stereotype);
            newObject = klass;
        } else if (newElementId == QLatin1String(ELEMENT_TYPE_ITEM)) {
            auto item = new MItem();
            newName = tr("New Item");
            if (!stereotype.isEmpty()) {
                item->setVariety(stereotype);
                item->setVarietyEditable(false);
            }
            newObject = item;
        }
        if (newObject) {
            if (!name.isEmpty())
                newName = tr("New %1").arg(name);
            newObject->setName(newName);
            dropNewModelElement(newObject, parentPackage, pos, diagram);
        }
    }
}

void DiagramSceneController::dropNewModelElement(MObject *modelObject, MPackage *parentPackage, const QPointF &pos,
                                                 MDiagram *diagram)
{
    m_modelController->undoController()->beginMergeSequence(tr("Drop Element"));
    m_modelController->addObject(parentPackage, modelObject);
    DElement *element = addObject(modelObject, pos, diagram);
    m_modelController->undoController()->endMergeSequence();
    if (element)
        emit newElementCreated(element, diagram);
}

MPackage *DiagramSceneController::findSuitableParentPackage(DElement *topmostDiagramElement, MDiagram *diagram)
{
    MPackage *parentPackage = 0;
    if (auto diagramPackage = dynamic_cast<DPackage *>(topmostDiagramElement)) {
        parentPackage = m_modelController->findObject<MPackage>(diagramPackage->modelUid());
    } else if (auto diagramObject = dynamic_cast<DObject *>(topmostDiagramElement)) {
        MObject *modelObject = m_modelController->findObject(diagramObject->modelUid());
        if (modelObject)
            parentPackage = dynamic_cast<MPackage *>(modelObject->owner());
    }
    if (parentPackage == 0 && diagram != 0)
        parentPackage = dynamic_cast<MPackage *>(diagram->owner());
    if (parentPackage == 0)
        parentPackage = m_modelController->rootPackage();
    return parentPackage;
}

MDiagram *DiagramSceneController::findDiagramBySearchId(MPackage *package, const QString &diagramName)
{
    QString diagramSearchId = NameController::calcElementNameSearchId(diagramName);
    foreach (const Handle<MObject> &handle, package->children()) {
        if (handle.hasTarget()) {
            if (auto diagram = dynamic_cast<MDiagram *>(handle.target())) {
                if (NameController::calcElementNameSearchId(diagram->name()) == diagramSearchId)
                    return diagram;
            }
        }
    }
    return 0;
}

namespace {

static QPointF alignObjectLeft(DObject *object, DObject *otherObject)
{
    qreal left = object->pos().x() + object->rect().left();
    QPointF pos = otherObject->pos();
    qreal otherLeft = pos.x() + otherObject->rect().left();
    qreal delta = otherLeft - left;
    pos.setX(pos.x() - delta);
    return pos;
}

static QPointF alignObjectRight(DObject *object, DObject *otherObject)
{
    qreal right = object->pos().x() + object->rect().right();
    QPointF pos = otherObject->pos();
    qreal otherRight = pos.x() + otherObject->rect().right();
    qreal delta = otherRight - right;
    pos.setX(pos.x() - delta);
    return pos;
}

static QPointF alignObjectHCenter(DObject *object, DObject *otherObject)
{
    qreal center = object->pos().x();
    QPointF pos = otherObject->pos();
    qreal otherCenter = pos.x();
    qreal delta = otherCenter - center;
    pos.setX(pos.x() - delta);
    return pos;
}

static QPointF alignObjectTop(DObject *object, DObject *otherObject)
{
    qreal top = object->pos().y() + object->rect().top();
    QPointF pos = otherObject->pos();
    qreal otherTop = pos.y() + otherObject->rect().top();
    qreal delta = otherTop - top;
    pos.setY(pos.y() - delta);
    return pos;
}

static QPointF alignObjectBottom(DObject *object, DObject *otherObject)
{
    qreal bottom = object->pos().y() + object->rect().bottom();
    QPointF pos = otherObject->pos();
    qreal otherBottom = pos.y() + otherObject->rect().bottom();
    qreal delta = otherBottom - bottom;
    pos.setY(pos.y() - delta);
    return pos;
}

static QPointF alignObjectVCenter(DObject *object, DObject *otherObject)
{
    qreal center = object->pos().y();
    QPointF pos = otherObject->pos();
    qreal otherCenter = pos.y();
    qreal delta = otherCenter - center;
    pos.setY(pos.y() - delta);
    return pos;
}

static QRectF alignObjectWidth(DObject *object, const QSizeF &size)
{
    QRectF rect = object->rect();
    rect.setX(-size.width() / 2.0);
    rect.setWidth(size.width());
    return rect;
}

static QRectF alignObjectHeight(DObject *object, const QSizeF &size)
{
    QRectF rect = object->rect();
    rect.setY(-size.height() / 2.0);
    rect.setHeight(size.height());
    return rect;
}

static QRectF alignObjectSize(DObject *object, const QSizeF &size)
{
    Q_UNUSED(object);

    QRectF rect;
    rect.setX(-size.width() / 2.0);
    rect.setY(-size.height() / 2.0);
    rect.setWidth(size.width());
    rect.setHeight(size.height());
    return rect;
}

}

void DiagramSceneController::alignLeft(DObject *object, const DSelection &selection, MDiagram *diagram)
{
    alignPosition(object, selection, alignObjectLeft, diagram);
}

void DiagramSceneController::alignRight(DObject *object, const DSelection &selection, MDiagram *diagram)
{
    alignPosition(object, selection, alignObjectRight, diagram);
}

void DiagramSceneController::alignHCenter(DObject *object, const DSelection &selection, MDiagram *diagram)
{
    alignPosition(object, selection, alignObjectHCenter, diagram);
}

void DiagramSceneController::alignTop(DObject *object, const DSelection &selection, MDiagram *diagram)
{
    alignPosition(object, selection, alignObjectTop, diagram);
}

void DiagramSceneController::alignBottom(DObject *object, const DSelection &selection, MDiagram *diagram)
{
    alignPosition(object, selection, alignObjectBottom, diagram);
}

void DiagramSceneController::alignVCenter(DObject *object, const DSelection &selection, MDiagram *diagram)
{
    alignPosition(object, selection, alignObjectVCenter, diagram);
}

void DiagramSceneController::alignWidth(DObject *object, const DSelection &selection, const QSizeF &minimumSize,
                                        MDiagram *diagram)
{
    alignSize(object, selection, minimumSize, alignObjectWidth, diagram);
}

void DiagramSceneController::alignHeight(DObject *object, const DSelection &selection, const QSizeF &minimumSize,
                                         MDiagram *diagram)
{
    alignSize(object, selection, minimumSize, alignObjectHeight, diagram);
}

void DiagramSceneController::alignSize(DObject *object, const DSelection &selection, const QSizeF &minimumSize,
                                       MDiagram *diagram)
{
    alignSize(object, selection, minimumSize, alignObjectSize, diagram);
}

void DiagramSceneController::alignPosition(DObject *object, const DSelection &selection,
                                           QPointF (*aligner)(DObject *, DObject *), MDiagram *diagram)
{
    foreach (const DSelection::Index &index, selection.indices()) {
        DElement *element = m_diagramController->findElement(index.elementKey(), diagram);
        if (auto selectedObject = dynamic_cast<DObject *>(element)) {
            if (selectedObject != object) {
                QPointF newPos = aligner(object, selectedObject);
                if (newPos != selectedObject->pos()) {
                    m_diagramController->startUpdateElement(selectedObject, diagram, DiagramController::UpdateGeometry);
                    selectedObject->setPos(newPos);
                    m_diagramController->finishUpdateElement(selectedObject, diagram, false);
                }
            }
        }
    }
}

void DiagramSceneController::alignSize(DObject *object, const DSelection &selection, const QSizeF &minimumSize,
                                       QRectF (*aligner)(DObject *, const QSizeF &), MDiagram *diagram)
{
    QSizeF size;
    if (object->rect().width() < minimumSize.width())
        size.setWidth(minimumSize.width());
    else
        size.setWidth(object->rect().width());
    if (object->rect().height() < minimumSize.height())
        size.setHeight(minimumSize.height());
    else
        size.setHeight(object->rect().height());
    foreach (const DSelection::Index &index, selection.indices()) {
        DElement *element = m_diagramController->findElement(index.elementKey(), diagram);
        if (auto selectedObject = dynamic_cast<DObject *>(element)) {
            QRectF newRect = aligner(selectedObject, size);
            if (newRect != selectedObject->rect()) {
                m_diagramController->startUpdateElement(selectedObject, diagram, DiagramController::UpdateGeometry);
                selectedObject->setAutoSized(false);
                selectedObject->setRect(newRect);
                m_diagramController->finishUpdateElement(selectedObject, diagram, false);
            }
        }
    }
}

void DiagramSceneController::alignOnRaster(DElement *element, MDiagram *diagram)
{
    AlignOnRasterVisitor visitor;
    visitor.setDiagramController(m_diagramController);
    visitor.setSceneInspector(m_sceneInspector);
    visitor.setDiagram(diagram);
    element->accept(&visitor);
}

DElement *DiagramSceneController::addModelElement(const Uid &modelElementKey, const QPointF &pos, MDiagram *diagram)
{
    DElement *element = 0;
    if (MObject *modelObject = m_modelController->findObject(modelElementKey)) {
        element = addObject(modelObject, pos, diagram);
    } else if (MRelation *modelRelation = m_modelController->findRelation(modelElementKey)) {
        element = addRelation(modelRelation, QList<QPointF>(), diagram);
    } else {
        QMT_CHECK(false);
    }
    return element;
}

DObject *DiagramSceneController::addObject(MObject *modelObject, const QPointF &pos, MDiagram *diagram)
{
    QMT_CHECK(modelObject);

    if (m_diagramController->hasDelegate(modelObject, diagram))
        return 0;

    m_diagramController->undoController()->beginMergeSequence(tr("Add Element"));

    DFactory factory;
    modelObject->accept(&factory);
    auto diagramObject = dynamic_cast<DObject *>(factory.product());
    QMT_CHECK(diagramObject);
    diagramObject->setPos(pos);
    m_diagramController->addElement(diagramObject, diagram);
    alignOnRaster(diagramObject, diagram);

    // add all relations between any other element on diagram and new element
    foreach (DElement *delement, diagram->diagramElements()) {
        if (delement != diagramObject) {
            auto dobject = dynamic_cast<DObject *>(delement);
            if (dobject) {
                MObject *mobject = m_modelController->findObject(dobject->modelUid());
                if (mobject) {
                    foreach (const Handle<MRelation> &handle, mobject->relations()) {
                        if (handle.hasTarget()
                                && ((handle.target()->endAUid() == modelObject->uid()
                                     && handle.target()->endBUid() == mobject->uid())
                                    || (handle.target()->endAUid() == mobject->uid()
                                        && handle.target()->endBUid() == modelObject->uid()))) {
                            addRelation(handle.target(), QList<QPointF>(), diagram);
                        }
                    }
                    foreach (const Handle<MRelation> &handle, modelObject->relations()) {
                        if (handle.hasTarget()
                                && ((handle.target()->endAUid() == modelObject->uid()
                                     && handle.target()->endBUid() == mobject->uid())
                                    || (handle.target()->endAUid() == mobject->uid()
                                        && handle.target()->endBUid() == modelObject->uid()))) {
                            addRelation(handle.target(), QList<QPointF>(), diagram);
                        }
                    }
                }
            }
        }
    }

    // add all self relations
    foreach (const Handle<MRelation> &handle, modelObject->relations()) {
        if (handle.hasTarget ()
                && handle.target()->endAUid() == modelObject->uid()
                && handle.target()->endBUid() == modelObject->uid()) {
            addRelation(handle.target(), QList<QPointF>(), diagram);
        }
    }

    m_diagramController->undoController()->endMergeSequence();

    return diagramObject;
}

DRelation *DiagramSceneController::addRelation(MRelation *modelRelation, const QList<QPointF> &intermediatePoints,
                                               MDiagram *diagram)
{
    QMT_CHECK(modelRelation);

    if (m_diagramController->hasDelegate(modelRelation, diagram))
        return 0;

    DFactory factory;
    modelRelation->accept(&factory);
    auto diagramRelation = dynamic_cast<DRelation *>(factory.product());
    QMT_CHECK(diagramRelation);

    MObject *endAModelObject = m_modelController->findObject(modelRelation->endAUid());
    QMT_CHECK(endAModelObject);
    DObject *endADiagramObject = m_diagramController->findDelegate<DObject>(endAModelObject, diagram);
    QMT_CHECK(endADiagramObject);
    diagramRelation->setEndAUid(endADiagramObject->uid());

    MObject *endBModelObject = m_modelController->findObject(modelRelation->endBUid());
    QMT_CHECK(endBModelObject);
    DObject *endBDiagramObject = m_diagramController->findDelegate<DObject>(endBModelObject, diagram);
    QMT_CHECK(endBDiagramObject);
    diagramRelation->setEndBUid(endBDiagramObject->uid());

    QList<DRelation::IntermediatePoint> relationPoints;
    if (endADiagramObject->uid() == endBDiagramObject->uid() && intermediatePoints.isEmpty()) {
        // create some intermediate points for self-relation
        QRectF rect = endADiagramObject->rect().translated(endADiagramObject->pos());
        static const qreal EDGE_RADIUS = 30.0;
        qreal w = rect.width() * 0.25;
        if (w > EDGE_RADIUS)
            w = EDGE_RADIUS;
        qreal h = rect.height() * 0.25;
        if (h > EDGE_RADIUS)
            h = EDGE_RADIUS;
        QPointF i1(rect.x() - EDGE_RADIUS, rect.bottom() - h);
        QPointF i2(rect.x() - EDGE_RADIUS, rect.bottom() + EDGE_RADIUS);
        QPointF i3(rect.x() + w, rect.bottom() + EDGE_RADIUS);
        relationPoints.append(DRelation::IntermediatePoint(i1));
        relationPoints.append(DRelation::IntermediatePoint(i2));
        relationPoints.append(DRelation::IntermediatePoint(i3));
    } else {
        foreach (const QPointF &intermediatePoint, intermediatePoints)
            relationPoints.append(DRelation::IntermediatePoint(intermediatePoint));
    }
    diagramRelation->setIntermediatePoints(relationPoints);

    m_diagramController->addElement(diagramRelation, diagram);
    alignOnRaster(diagramRelation, diagram);

    return diagramRelation;
}

} // namespace qmt
