/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "annotationtool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "itemutilfunctions.h"
#include "formeditoritem.h"

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
#include <QMetaType>

namespace QmlDesigner {

class AnnotationToolAction : public AbstractAction
{
public:
    AnnotationToolAction() : AbstractAction(QCoreApplication::translate("AnnotationToolAction","Edit Annotation"))
    {
    }

    QByteArray category() const override
    {
        return QByteArray();
    }

    QByteArray menuId() const override
    {
        return "AnnotationTool";
    }

    int priority() const override
    {
        return CustomActionsPriority + 5;
    }

    Type type() const override
    {
        return FormEditorAction;
    }

protected:
    bool isVisible(const SelectionContext &selectionContext) const override
    {
        return selectionContext.singleNodeIsSelected();
    }

    bool isEnabled(const SelectionContext &selectionContext) const override
    {
        return isVisible(selectionContext);
    }
};

AnnotationTool::AnnotationTool()
{
    auto annotationToolAction = new AnnotationToolAction;
    QmlDesignerPlugin::instance()->designerActionManager().addDesignerAction(annotationToolAction);
        connect(annotationToolAction->action(), &QAction::triggered, [=]() {
        view()->changeCurrentToolTo(this);
    });
}

AnnotationTool::~AnnotationTool() = default;

void AnnotationTool::clear()
{
    if (m_annotationEditor)
        m_annotationEditor->deleteLater();

    AbstractFormEditorTool::clear();
}

void AnnotationTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                                            QGraphicsSceneMouseEvent *event)
{
    AbstractFormEditorTool::mousePressEvent(itemList, event);
}

void AnnotationTool::mouseMoveEvent(const QList<QGraphicsItem*> & /*itemList*/,
                              QGraphicsSceneMouseEvent * /*event*/)
{
}

void AnnotationTool::hoverMoveEvent(const QList<QGraphicsItem*> & /*itemList*/,
                        QGraphicsSceneMouseEvent * /*event*/)
{
}

void AnnotationTool::keyPressEvent(QKeyEvent * /*keyEvent*/)
{
}

void AnnotationTool::keyReleaseEvent(QKeyEvent * /*keyEvent*/)
{
}

void  AnnotationTool::dragLeaveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{
}

void  AnnotationTool::dragMoveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{
}

void AnnotationTool::mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                                 QGraphicsSceneMouseEvent *event)
{
    AbstractFormEditorTool::mouseReleaseEvent(itemList, event);
}


void AnnotationTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event)
{
    AbstractFormEditorTool::mouseDoubleClickEvent(itemList, event);
}

void AnnotationTool::itemsAboutToRemoved(const QList<FormEditorItem*> &removedItemList)
{
    if (m_annotationEditor.isNull())
        return;

    if (removedItemList.contains(m_formEditorItem))
        view()->changeToSelectionTool();
}

void AnnotationTool::selectedItemsChanged(const QList<FormEditorItem*> &itemList)
{
    if (!itemList.isEmpty()) {
        m_formEditorItem = itemList.constFirst();

        ModelNode itemModelNode = m_formEditorItem->qmlItemNode().modelNode();
        m_oldCustomId = itemModelNode.customId();
        m_oldAnnotation = itemModelNode.annotation();

        if (m_annotationEditor.isNull()) {
            m_annotationEditor = new AnnotationEditorDialog(view()->formEditorWidget()->parentWidget(),
                                                            itemModelNode.displayName(),
                                                            m_oldCustomId, m_oldAnnotation);

            connect(m_annotationEditor, &AnnotationEditorDialog::accepted, this, &AnnotationTool::annotationDialogAccepted);
            connect(m_annotationEditor, &QDialog::rejected, this, &AnnotationTool::annotationDialogRejected);

            m_annotationEditor->show();
            m_annotationEditor->raise();
        }
    } else {
        view()->changeToSelectionTool();
    }
}

void AnnotationTool::instancesCompleted(const QList<FormEditorItem*> & /*itemList*/)
{
}

void  AnnotationTool::instancesParentChanged(const QList<FormEditorItem *> & /*itemList*/)
{
}

void AnnotationTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > & /*propertyList*/)
{
}

void AnnotationTool::formEditorItemsChanged(const QList<FormEditorItem*> & /*itemList*/)
{
}

int AnnotationTool::wantHandleItem(const ModelNode & /*modelNode*/) const
{
    return 5;
}

QString AnnotationTool::name() const
{
    return tr("Annotation Tool");
}

void AnnotationTool::annotationDialogAccepted()
{
    if (m_annotationEditor) {
        saveNewCustomId(m_annotationEditor->customId());
        saveNewAnnotation(m_annotationEditor->annotation());

        m_annotationEditor->close();
        m_annotationEditor->deleteLater();
    }

    m_annotationEditor = nullptr;

    view()->changeToSelectionTool();
}

void AnnotationTool::saveNewCustomId(const QString &customId)
{
    if (m_formEditorItem) {
        m_oldCustomId = customId;
        m_formEditorItem->qmlItemNode().modelNode().setCustomId(customId);
    }
}

void AnnotationTool::saveNewAnnotation(const Annotation &annotation)
{
    if (m_formEditorItem) {
        if (annotation.comments().isEmpty())
            m_formEditorItem->qmlItemNode().modelNode().removeAnnotation();
        else
            m_formEditorItem->qmlItemNode().modelNode().setAnnotation(annotation);

        m_oldAnnotation = annotation;
    }
}

void AnnotationTool::annotationDialogRejected()
{
    if (m_annotationEditor) {
        m_annotationEditor->close();
        m_annotationEditor->deleteLater();
    }

    m_annotationEditor = nullptr;

    view()->changeToSelectionTool();
}

}
