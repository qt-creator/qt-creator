// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "anchorindicator.h"

#include "qmlanchors.h"
#include "anchorindicatorgraphicsitem.h"
#include <QGraphicsScene>
#include <QGraphicsPathItem>

namespace QmlDesigner {

AnchorIndicator::AnchorIndicator(LayerItem *layerItem)
    : m_layerItem(layerItem)
{
}

AnchorIndicator::AnchorIndicator() = default;

AnchorIndicator::~AnchorIndicator()
{
    clear();
}

void AnchorIndicator::show()
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

void AnchorIndicator::hide()
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

void AnchorIndicator::clear()
{
  delete m_indicatorTopShape;
  delete m_indicatorBottomShape;
  delete m_indicatorLeftShape;
  delete m_indicatorRightShape;
}

void AnchorIndicator::setItems(const QList<FormEditorItem *> &itemList)
{
    clear();

    if (itemList.size() == 1) {
        m_formEditorItem = itemList.constFirst();
        QmlItemNode sourceQmlItemNode = m_formEditorItem->qmlItemNode();
        if (!sourceQmlItemNode.modelNode().isRootNode()) {
            QmlAnchors qmlAnchors = sourceQmlItemNode.anchors();

            if (qmlAnchors.modelHasAnchor(AnchorLineTop)) {
                m_indicatorTopShape = new AnchorIndicatorGraphicsItem(m_layerItem.data());
                m_indicatorTopShape->updateAnchorIndicator(AnchorLine(sourceQmlItemNode, AnchorLineTop),
                                                           qmlAnchors.modelAnchor(AnchorLineTop));
            }

            if (qmlAnchors.modelHasAnchor(AnchorLineBottom)) {
                m_indicatorBottomShape = new AnchorIndicatorGraphicsItem(m_layerItem.data());
                m_indicatorBottomShape->updateAnchorIndicator(AnchorLine(sourceQmlItemNode, AnchorLineBottom),
                                                              qmlAnchors.modelAnchor(AnchorLineBottom));
            }

            if (qmlAnchors.modelHasAnchor(AnchorLineLeft)) {
                m_indicatorLeftShape = new AnchorIndicatorGraphicsItem(m_layerItem.data());
                m_indicatorLeftShape->updateAnchorIndicator(AnchorLine(sourceQmlItemNode, AnchorLineLeft),
                                                            qmlAnchors.modelAnchor(AnchorLineLeft));
            }

            if (qmlAnchors.modelHasAnchor(AnchorLineRight)) {
                m_indicatorRightShape = new AnchorIndicatorGraphicsItem(m_layerItem.data());
                m_indicatorRightShape->updateAnchorIndicator(AnchorLine(sourceQmlItemNode, AnchorLineRight),
                                                             qmlAnchors.modelAnchor(AnchorLineRight));
            }
        }
    }
}

void AnchorIndicator::updateItems(const QList<FormEditorItem *> &itemList)
{
    for (FormEditorItem *formEditorItem : itemList) {
        if (formEditorItem == m_formEditorItem) {
            QmlItemNode sourceQmlItemNode = m_formEditorItem->qmlItemNode();
            if (!sourceQmlItemNode.modelNode().isRootNode()) {
                QmlAnchors qmlAnchors = formEditorItem->qmlItemNode().anchors();

                if (qmlAnchors.modelHasAnchor(AnchorLineTop)) {
                    if (m_indicatorTopShape.isNull())
                        m_indicatorTopShape = new AnchorIndicatorGraphicsItem(m_layerItem.data());
                    m_indicatorTopShape->updateAnchorIndicator(AnchorLine(sourceQmlItemNode, AnchorLineTop),
                                                               qmlAnchors.modelAnchor(AnchorLineTop));
                } else {
                    delete m_indicatorTopShape;
                }

                if (qmlAnchors.modelHasAnchor(AnchorLineBottom)) {
                    if (m_indicatorBottomShape.isNull())
                        m_indicatorBottomShape = new AnchorIndicatorGraphicsItem(m_layerItem.data());
                    m_indicatorBottomShape->updateAnchorIndicator(AnchorLine(sourceQmlItemNode, AnchorLineBottom),
                                                                  qmlAnchors.modelAnchor(AnchorLineBottom));
                } else {
                    delete m_indicatorBottomShape;
                }

                if (qmlAnchors.modelHasAnchor(AnchorLineLeft)) {
                    if (m_indicatorLeftShape.isNull())
                        m_indicatorLeftShape = new AnchorIndicatorGraphicsItem(m_layerItem.data());
                    m_indicatorLeftShape->updateAnchorIndicator(AnchorLine(sourceQmlItemNode, AnchorLineLeft),
                                                                qmlAnchors.modelAnchor(AnchorLineLeft));
                } else {
                    delete m_indicatorLeftShape;
                }

                if (qmlAnchors.modelHasAnchor(AnchorLineRight)) {
                    if (m_indicatorRightShape.isNull())
                        m_indicatorRightShape = new AnchorIndicatorGraphicsItem(m_layerItem.data());
                    m_indicatorRightShape->updateAnchorIndicator(AnchorLine(sourceQmlItemNode, AnchorLineRight),
                                                                 qmlAnchors.modelAnchor(AnchorLineRight));
                } else {
                    delete m_indicatorRightShape;
                }
            }

            return;
        }
    }
}

} // namespace QmlDesigner
