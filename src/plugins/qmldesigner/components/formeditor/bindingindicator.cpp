// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bindingindicator.h"

#include "bindingindicatorgraphicsitem.h"

#include <QLineF>

namespace QmlDesigner {

BindingIndicator::BindingIndicator(LayerItem *layerItem)
    : m_layerItem(layerItem)
{
}

BindingIndicator::BindingIndicator() = default;

BindingIndicator::~BindingIndicator()
{
    clear();
}

void BindingIndicator::show()
{
    if (m_indicatorTopShape)
        m_indicatorTopShape->show();

    if (m_indicatorBottomShape)
        m_indicatorBottomShape->show();

    if (m_indicatorLeftShape)
        m_indicatorLeftShape->show();

    if (m_indicatorRightShape)
        m_indicatorRightShape->show();
}

void BindingIndicator::hide()
{
    if (m_indicatorTopShape)
        m_indicatorTopShape->hide();

    if (m_indicatorBottomShape)
        m_indicatorBottomShape->hide();

    if (m_indicatorLeftShape)
        m_indicatorLeftShape->hide();

    if (m_indicatorRightShape)
        m_indicatorRightShape->hide();
}

void BindingIndicator::clear()
{
  delete m_indicatorTopShape;
  delete m_indicatorBottomShape;
  delete m_indicatorLeftShape;
  delete m_indicatorRightShape;
}

QLineF topLine(const QmlItemNode &qmlItemNode)
{
    QRectF rectangle = qmlItemNode.instanceSceneTransform().mapRect(qmlItemNode.instanceBoundingRect()).adjusted(1, 1, 0, 0);

    return QLineF(rectangle.topLeft(), rectangle.topRight());
}

QLineF bottomLine(const QmlItemNode &qmlItemNode)
{
    QRectF rectangle = qmlItemNode.instanceSceneTransform().mapRect(qmlItemNode.instanceBoundingRect()).adjusted(1, 0, 0, 0);

    return QLineF(rectangle.bottomLeft(), rectangle.bottomRight());
}

QLineF leftLine(const QmlItemNode &qmlItemNode)
{
    QRectF rectangle = qmlItemNode.instanceSceneTransform().mapRect(qmlItemNode.instanceBoundingRect()).adjusted(1, 1, 0, 0);

    return QLineF(rectangle.topLeft(), rectangle.bottomLeft());
}

QLineF rightLine(const QmlItemNode &qmlItemNode)
{
    QRectF rectangle = qmlItemNode.instanceSceneTransform().mapRect(qmlItemNode.instanceBoundingRect()).adjusted(0, 1, 0, 0);

    return QLineF(rectangle.topRight(), rectangle.bottomRight());
}

void BindingIndicator::setItems(const QList<FormEditorItem *> &itemList)
{
    clear();

    if (itemList.size() == 1) {
        m_formEditorItem = itemList.constFirst();
        const QmlItemNode qmlItemNode = m_formEditorItem->qmlItemNode();

        if (!qmlItemNode.isValid())
            return;

        if (qmlItemNode.hasBindingProperty("x")) {
            m_indicatorTopShape = new BindingIndicatorGraphicsItem(m_layerItem.data());
            m_indicatorTopShape->updateBindingIndicator(leftLine(qmlItemNode));
        }

        if (qmlItemNode.hasBindingProperty("y")) {
            m_indicatorLeftShape = new BindingIndicatorGraphicsItem(m_layerItem.data());
            m_indicatorLeftShape->updateBindingIndicator(topLine(qmlItemNode));
        }

        if (qmlItemNode.hasBindingProperty("width")) {
            m_indicatorRightShape = new BindingIndicatorGraphicsItem(m_layerItem.data());
            m_indicatorRightShape->updateBindingIndicator(rightLine(qmlItemNode));
        }

        if (qmlItemNode.hasBindingProperty("height")) {
            m_indicatorBottomShape = new BindingIndicatorGraphicsItem(m_layerItem.data());
            m_indicatorBottomShape->updateBindingIndicator(bottomLine(qmlItemNode));
        }
    }
}

void BindingIndicator::updateItems(const QList<FormEditorItem *> &itemList)
{
    for (FormEditorItem *formEditorItem : itemList) {
        if (formEditorItem == m_formEditorItem) {
            const QmlItemNode qmlItemNode = m_formEditorItem->qmlItemNode();

            if (!qmlItemNode.isValid())
                continue;

            if (qmlItemNode.hasBindingProperty("x")) {
                if (m_indicatorTopShape.isNull())
                    m_indicatorTopShape = new BindingIndicatorGraphicsItem(m_layerItem.data());
                m_indicatorTopShape->updateBindingIndicator(leftLine(qmlItemNode));
            } else {
                delete m_indicatorTopShape;
            }

            if (qmlItemNode.hasBindingProperty("y")) {
                if (m_indicatorLeftShape.isNull())
                    m_indicatorLeftShape = new BindingIndicatorGraphicsItem(m_layerItem.data());
                m_indicatorLeftShape->updateBindingIndicator(topLine(qmlItemNode));
            } else {
                delete m_indicatorLeftShape;
            }

            if (qmlItemNode.hasBindingProperty("width")) {
                if (m_indicatorRightShape.isNull())
                    m_indicatorRightShape = new BindingIndicatorGraphicsItem(m_layerItem.data());
                m_indicatorRightShape->updateBindingIndicator(rightLine(qmlItemNode));
            } else {
                delete m_indicatorRightShape;
            }

            if (qmlItemNode.hasBindingProperty("height")) {
                if (m_indicatorBottomShape.isNull())
                    m_indicatorBottomShape = new BindingIndicatorGraphicsItem(m_layerItem.data());
                m_indicatorBottomShape->updateBindingIndicator(bottomLine(qmlItemNode));
            } else {
                delete m_indicatorBottomShape;
            }

            return;
        }
    }
}

} // namespace QmlDesigner
