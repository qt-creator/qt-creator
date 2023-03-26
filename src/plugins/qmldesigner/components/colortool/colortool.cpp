// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "colortool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "itemutilfunctions.h"
#include "formeditoritem.h"

#include "resizehandleitem.h"

#include "nodemetainfo.h"
#include "qmlitemnode.h"
#include "bindingproperty.h"
#include <qmldesignerplugin.h>
#include <abstractaction.h>
#include <designeractionmanager.h>

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QDebug>
#include <QPair>
#include <QUrl>

namespace QmlDesigner {

ColorTool::ColorTool()
{
}

ColorTool::~ColorTool() = default;

void ColorTool::clear()
{
    if (m_colorDialog)
        m_colorDialog.data()->deleteLater();

    AbstractFormEditorTool::clear();
}

void ColorTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                                            QGraphicsSceneMouseEvent *event)
{
    AbstractFormEditorTool::mousePressEvent(itemList, event);
}

void ColorTool::mouseMoveEvent(const QList<QGraphicsItem*> & /*itemList*/,
                              QGraphicsSceneMouseEvent * /*event*/)
{
}

void ColorTool::hoverMoveEvent(const QList<QGraphicsItem*> & /*itemList*/,
                        QGraphicsSceneMouseEvent * /*event*/)
{
}

void ColorTool::keyPressEvent(QKeyEvent * /*keyEvent*/)
{
}

void ColorTool::keyReleaseEvent(QKeyEvent * /*keyEvent*/)
{
}

void  ColorTool::dragLeaveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{
}

void  ColorTool::dragMoveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{
}

void ColorTool::mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                                 QGraphicsSceneMouseEvent *event)
{
    AbstractFormEditorTool::mouseReleaseEvent(itemList, event);
}


void ColorTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event)
{
    AbstractFormEditorTool::mouseDoubleClickEvent(itemList, event);
}

void ColorTool::itemsAboutToRemoved(const QList<FormEditorItem*> &removedItemList)
{
    if (m_colorDialog.isNull())
        return;

    if (removedItemList.contains(m_formEditorItem))
        view()->changeToSelectionTool();
}

void ColorTool::selectedItemsChanged(const QList<FormEditorItem*> &itemList)
{
    if (m_colorDialog.data() && m_oldColor.isValid())
        m_formEditorItem->qmlItemNode().setVariantProperty("color", m_oldColor);

    if (!itemList.isEmpty()
            && itemList.constFirst()->qmlItemNode().modelNode().metaInfo().hasProperty("color")) {
        m_formEditorItem = itemList.constFirst();

        if (m_formEditorItem->qmlItemNode().hasBindingProperty("color"))
            m_oldExpression = m_formEditorItem->qmlItemNode().modelNode().bindingProperty("color").expression();
        else
            m_oldColor = m_formEditorItem->qmlItemNode().modelValue("color").value<QColor>();

        if (m_colorDialog.isNull()) {
            m_colorDialog = new QColorDialog(view()->formEditorWidget()->parentWidget());
            m_colorDialog.data()->setCurrentColor(m_oldColor);

            connect(m_colorDialog.data(), &QDialog::accepted, this, &ColorTool::colorDialogAccepted);
            connect(m_colorDialog.data(), &QDialog::rejected, this, &ColorTool::colorDialogRejected);
            connect(m_colorDialog.data(), &QColorDialog::currentColorChanged, this, &ColorTool::currentColorChanged);

            m_colorDialog.data()->exec();
        }
    } else {
        view()->changeToSelectionTool();
    }
}

void ColorTool::instancesCompleted(const QList<FormEditorItem*> & /*itemList*/)
{
}

void  ColorTool::instancesParentChanged(const QList<FormEditorItem *> & /*itemList*/)
{
}

void ColorTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > & /*propertyList*/)
{
}

void ColorTool::formEditorItemsChanged(const QList<FormEditorItem*> & /*itemList*/)
{
}

int ColorTool::wantHandleItem(const ModelNode &modelNode) const
{
    if (modelNode.metaInfo().hasProperty("color"))
        return 10;

    return 0;
}

QString ColorTool::name() const
{
    return tr("Color Tool");
}

void ColorTool::colorDialogAccepted()
{
    m_oldExpression.clear();
    view()->changeToSelectionTool();
}

void ColorTool::colorDialogRejected()
{
    if (m_formEditorItem) {
        if (!m_oldExpression.isEmpty())
            m_formEditorItem->qmlItemNode().setBindingProperty("color", m_oldExpression);
        else {
            if (m_oldColor.isValid())
                m_formEditorItem->qmlItemNode().setVariantProperty("color", m_oldColor);
            else
                m_formEditorItem->qmlItemNode().removeProperty("color");
        }
    }

    m_oldExpression.clear();
    view()->changeToSelectionTool();
}

void ColorTool::currentColorChanged(const QColor &color)
{
    if (m_formEditorItem)
        m_formEditorItem->qmlItemNode().setVariantProperty("color", color);
}

}
