// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rotationmanipulator.h"

#include "formeditoritem.h"
#include "formeditorscene.h"
#include "qmlanchors.h"
#include <QDebug>
#include <QtMath>
#include "enumeration.h"
#include "mathutils.h"

#include <limits>

namespace QmlDesigner {

RotationManipulator::RotationManipulator(LayerItem *layerItem, FormEditorView *view)
    : m_view(view)
    , m_beginTopMargin(0.0)
    , m_beginLeftMargin(0.0)
    , m_beginRightMargin(0.0)
    , m_beginBottomMargin(0.0)
    , m_layerItem(layerItem)
{
}

RotationManipulator::~RotationManipulator()
{
    deleteSnapLines();
}

void RotationManipulator::setHandle(RotationHandleItem *rotationHandle)
{
    Q_ASSERT(rotationHandle);
    m_rotationHandle = rotationHandle;
    m_rotationController = rotationHandle->rotationController();
    Q_ASSERT(m_rotationController.isValid());
}

void RotationManipulator::removeHandle()
{
    m_rotationController = RotationController();
    m_rotationHandle = nullptr;
}

void RotationManipulator::begin(const QPointF &/*beginPoint*/)
{
    if (m_rotationController.isValid()) {
        m_isActive = true;
        m_beginBoundingRect = m_rotationController.formEditorItem()->qmlItemNode().instanceBoundingRect();
        m_beginFromContentItemToSceneTransform = m_rotationController.formEditorItem()->instanceSceneContentItemTransform();
        m_beginFromSceneToContentItemTransform = m_beginFromContentItemToSceneTransform.inverted();
        m_beginFromItemToSceneTransform = m_rotationController.formEditorItem()->instanceSceneTransform();
        m_beginToParentTransform = m_rotationController.formEditorItem()->qmlItemNode().instanceTransform();
        m_rewriterTransaction = m_view->beginRewriterTransaction(QByteArrayLiteral("RotationManipulator::begin"));
        m_rewriterTransaction.ignoreSemanticChecks();
        m_beginBottomRightPoint = m_beginToParentTransform.map(m_rotationController.formEditorItem()->qmlItemNode().instanceBoundingRect().bottomRight());

        QmlAnchors anchors(m_rotationController.formEditorItem()->qmlItemNode().anchors());
        m_beginTopMargin = anchors.instanceMargin(AnchorLineTop);
        m_beginLeftMargin = anchors.instanceMargin(AnchorLineLeft);
        m_beginRightMargin = anchors.instanceMargin(AnchorLineRight);
        m_beginBottomMargin = anchors.instanceMargin(AnchorLineBottom);

        m_beginRotation = m_rotationController.formEditorItem()->qmlItemNode().rotation();
        deleteSnapLines();
    }
}

void RotationManipulator::update(const QPointF& updatePoint, Qt::KeyboardModifiers keyMods)
{
    if (m_rotationController.isValid()) {
        FormEditorItem *formEditorItem = m_rotationController.formEditorItem();

        const bool degreeStep5 = keyMods.testFlag(Qt::ShiftModifier);
        const bool degreeStep45 = keyMods.testFlag(Qt::AltModifier);

        QPointF updatePointInLocalSpace = m_beginFromSceneToContentItemTransform.map(updatePoint);

        QRectF boundingRect(m_beginBoundingRect);

        QPointF transformOrigin;
        QVariant transformOriginVar = formEditorItem->qmlItemNode().transformOrigin();
        if (transformOriginVar.isValid()) {
            QmlDesigner::Enumeration a = transformOriginVar.value<QmlDesigner::Enumeration>();
            const QString originStr = a.nameToString();
            if (originStr == "TopLeft") {
                transformOrigin = boundingRect.topLeft();
            }
            else if (originStr == "Top") {
                transformOrigin.setX(boundingRect.center().x());
                transformOrigin.setY(boundingRect.top());
            }
            else if (originStr == "TopRight") {
                transformOrigin = boundingRect.topRight();
            }
            else if (originStr == "Right") {
                transformOrigin.setX(boundingRect.right());
                transformOrigin.setY(boundingRect.center().y());
            }
            else if (originStr == "BottomRight") {
                transformOrigin = boundingRect.bottomRight();
            }
            else if (originStr == "Bottom") {
                transformOrigin.setX(boundingRect.center().x());
                transformOrigin.setY(boundingRect.bottom());
            }
            else if (originStr == "BottomLeft") {
                transformOrigin = boundingRect.bottomLeft();
            }
            else if (originStr == "Left") {
                transformOrigin.setX(boundingRect.left());
                transformOrigin.setY(boundingRect.center().y());
            }
            else {
                //center and anything else
                transformOrigin = boundingRect.center();
            }
        }
        else {
            transformOrigin = boundingRect.center();
        }

        auto angleCalc = [](const QPointF &origin, const QPointF &position){
            const qreal deltaX = origin.x() - position.x();
            const qreal deltaY = origin.y() - position.y();

            return qRadiansToDegrees(qAtan2(deltaY, deltaX));
        };

        auto snapCalc = [](const qreal angle, const qreal snap){
            return qRound(angle/snap)*snap;
        };

        auto resultCalc = [&](qreal cursorAngle, qreal handleAngle) {
            qreal result = 0.0;
            const qreal rotateBy = cursorAngle - handleAngle;

            if (degreeStep45)
                result = snapCalc(rotateBy, 45.0);
            else if (degreeStep5)
                result = snapCalc(rotateBy, 5.0);
            else
                result = rotateBy;

            while (result > 360.)
                result -= 360.;
            while (result < -360.)
                result += 360.;

            return result;
        };

        const qreal cursorAngle = angleCalc(transformOrigin, updatePointInLocalSpace);
        const QPointF topLeftHandle = boundingRect.topLeft();
        const QPointF topRightHandle = boundingRect.topRight();
        const QPointF bottomRightHandle = boundingRect.bottomRight();
        const QPointF bottomLeftHandle = boundingRect.bottomLeft();

        if (m_rotationHandle->isTopLeftHandle()) {
            //we need to change origin coords, otherwise item will rotate unpredictably:
            if (transformOrigin == topLeftHandle)
                transformOrigin = boundingRect.center();

            const qreal handleAngle = angleCalc(transformOrigin, topLeftHandle);
            formEditorItem->qmlItemNode().setRotation(resultCalc(cursorAngle + m_beginRotation, handleAngle));
        }
        else if (m_rotationHandle->isTopRightHandle()) {
            if (transformOrigin == topRightHandle)
                transformOrigin = boundingRect.center();
            const qreal handleAngle = angleCalc(transformOrigin, topRightHandle);
            formEditorItem->qmlItemNode().setRotation(resultCalc(cursorAngle + m_beginRotation, handleAngle));
        }
        else if (m_rotationHandle->isBottomRightHandle()) {
            if (transformOrigin == bottomRightHandle)
                transformOrigin = boundingRect.center();
            const qreal handleAngle = angleCalc(transformOrigin, bottomRightHandle);
            formEditorItem->qmlItemNode().setRotation(resultCalc(cursorAngle + m_beginRotation, handleAngle));
        }
        else if (m_rotationHandle->isBottomLeftHandle()) {
            if (transformOrigin == bottomLeftHandle)
                transformOrigin = boundingRect.center();
            const qreal handleAngle = angleCalc(transformOrigin, bottomLeftHandle);
            formEditorItem->qmlItemNode().setRotation(resultCalc(cursorAngle + m_beginRotation, handleAngle));
        }
    }
}

void RotationManipulator::end()
{
    m_isActive = false;
    m_rewriterTransaction.commit();
    clear();
    removeHandle();
}

void RotationManipulator::deleteSnapLines()
{
    if (m_layerItem) {
        for (QGraphicsItem *item : std::as_const(m_graphicsLineList)) {
            m_layerItem->scene()->removeItem(item);
            delete item;
        }
    }

    m_graphicsLineList.clear();
    m_view->scene()->update();
}

RotationHandleItem *RotationManipulator::rotationHandle()
{
    return m_rotationHandle;
}

void RotationManipulator::clear()
{
    m_rewriterTransaction.commit();

    deleteSnapLines();
    m_beginBoundingRect = QRectF();
    m_beginFromSceneToContentItemTransform = QTransform();
    m_beginFromContentItemToSceneTransform = QTransform();
    m_beginFromItemToSceneTransform = QTransform();
    m_beginToParentTransform = QTransform();
    m_beginTopMargin = 0.0;
    m_beginLeftMargin = 0.0;
    m_beginRightMargin = 0.0;
    m_beginBottomMargin = 0.0;
    removeHandle();
}

bool RotationManipulator::isActive() const
{
    return m_isActive;
}

}
