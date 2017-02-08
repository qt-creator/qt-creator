/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "colortool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "itemutilfunctions.h"
#include "formeditoritem.h"

#include "resizehandleitem.h"

#include "nodemetainfo.h"
#include "qmlitemnode.h"
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

class ColorToolAction : public AbstractAction
{
public:
    ColorToolAction() : AbstractAction(QCoreApplication::translate("ColorToolAction","Edit Color")) {}

    QByteArray category() const
    {
        return QByteArray();
    }

    QByteArray menuId() const
    {
        return "ColorTool";
    }

    int priority() const
    {
        return CustomActionsPriority;
    }

    Type type() const
    {
        return FormEditorAction;
    }

protected:
    bool isVisible(const SelectionContext &selectionContext) const
    {
        if (selectionContext.singleNodeIsSelected())
            return selectionContext.currentSingleSelectedNode().metaInfo().hasProperty("color");

        return false;
    }

    bool isEnabled(const SelectionContext &selectionContext) const
    {
        return isVisible(selectionContext);
    }
};

ColorTool::ColorTool()
    : QObject(), AbstractCustomTool()
{
    ColorToolAction *colorToolAction = new ColorToolAction;
    QmlDesignerPlugin::instance()->designerActionManager().addDesignerAction(colorToolAction);
    connect(colorToolAction->action(), &QAction::triggered, [=]() {
        view()->changeCurrentToolTo(this);
    });
}

ColorTool::~ColorTool()
{
}

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

    if (removedItemList.contains(m_formEditorItem.data()))
        view()->changeToSelectionTool();
}

void ColorTool::selectedItemsChanged(const QList<FormEditorItem*> &itemList)
{
    if (m_colorDialog.data()
            && m_oldColor.isValid())
        m_formEditorItem.data()->qmlItemNode().setVariantProperty("color", m_oldColor);

    if (!itemList.isEmpty()
            && itemList.first()->qmlItemNode().modelNode().metaInfo().hasProperty("color")) {
        m_formEditorItem = itemList.first();
        m_oldColor =  m_formEditorItem.data()->qmlItemNode().modelValue("color").value<QColor>();

        if (m_colorDialog.isNull()) {
            m_colorDialog = new QColorDialog(view()->formEditorWidget()->parentWidget());
            m_colorDialog.data()->setCurrentColor(m_oldColor);

            connect(m_colorDialog.data(), SIGNAL(accepted()), SLOT(colorDialogAccepted()));
            connect(m_colorDialog.data(), SIGNAL(rejected()), SLOT(colorDialogRejected()));
            connect(m_colorDialog.data(), SIGNAL(currentColorChanged(QColor)), SLOT(currentColorChanged(QColor)));

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
    view()->changeToSelectionTool();
}

void ColorTool::colorDialogRejected()
{
    if (m_formEditorItem) {
        if (m_oldColor.isValid())
            m_formEditorItem.data()->qmlItemNode().setVariantProperty("color", m_oldColor);
        else
            m_formEditorItem.data()->qmlItemNode().removeProperty("color");

    }

    view()->changeToSelectionTool();
}

void ColorTool::currentColorChanged(const QColor &color)
{
    if (m_formEditorItem) {
        m_formEditorItem.data()->qmlItemNode().setVariantProperty("color", color);
    }
}

}
