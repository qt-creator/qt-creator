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

#include "sourcetool.h"

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

#include <utils/icon.h>

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QDebug>
#include <QPair>
#include <QUrl>

namespace {

bool modelNodeHasUrlSource(const QmlDesigner::ModelNode &modelNode)
{
    QmlDesigner::NodeMetaInfo metaInfo = modelNode.metaInfo();
    if (metaInfo.isValid()) {
        if (metaInfo.hasProperty("source")) {
            if (metaInfo.propertyTypeName("source") == "QUrl")
                return true;
            if (metaInfo.propertyTypeName("source") == "url")
                return true;
        }
    }
    return false;
}

} //namespace

namespace QmlDesigner {

class SourceToolAction : public AbstractAction
{
public:
    SourceToolAction() : AbstractAction(QCoreApplication::translate("SourceToolAction","Change Source URL..."))
    {
        const Utils::Icon prevIcon({
                {QLatin1String(":/utils/images/fileopen.png"), Utils::Theme::OutputPanes_NormalMessageTextColor}}, Utils::Icon::MenuTintedStyle);

        action()->setIcon(prevIcon.icon());
    }

    QByteArray category() const
    {
        return QByteArray();
    }

    QByteArray menuId() const
    {
        return "SourceTool";
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
            return modelNodeHasUrlSource(selectionContext.currentSingleSelectedNode());
        return false;
    }

    bool isEnabled(const SelectionContext &selectionContext) const
    {
        return isVisible(selectionContext);
    }
};


SourceTool::SourceTool()
    : QObject(), AbstractCustomTool()
{
    SourceToolAction *sourceToolAction = new SourceToolAction;
    QmlDesignerPlugin::instance()->designerActionManager().addDesignerAction(sourceToolAction);
    connect(sourceToolAction->action(), &QAction::triggered, [=]() {
        view()->changeCurrentToolTo(this);
    });
}

SourceTool::~SourceTool()
{
}

void SourceTool::clear()
{
    AbstractFormEditorTool::clear();
}

void SourceTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                                 QGraphicsSceneMouseEvent *event)
{
    AbstractFormEditorTool::mousePressEvent(itemList, event);
}

void SourceTool::mouseMoveEvent(const QList<QGraphicsItem*> & /*itemList*/,
                                QGraphicsSceneMouseEvent * /*event*/)
{
}

void SourceTool::hoverMoveEvent(const QList<QGraphicsItem*> & /*itemList*/,
                                QGraphicsSceneMouseEvent * /*event*/)
{
}

void SourceTool::keyPressEvent(QKeyEvent * /*keyEvent*/)
{
}

void SourceTool::keyReleaseEvent(QKeyEvent * /*keyEvent*/)
{
}

void  SourceTool::dragLeaveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{
}

void  SourceTool::dragMoveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{
}

void SourceTool::mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                                   QGraphicsSceneMouseEvent *event)
{
    AbstractFormEditorTool::mouseReleaseEvent(itemList, event);
}


void SourceTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event)
{
    AbstractFormEditorTool::mouseDoubleClickEvent(itemList, event);
}

void SourceTool::itemsAboutToRemoved(const QList<FormEditorItem*> &removedItemList)
{
    if (removedItemList.contains(m_formEditorItem))
        view()->changeToSelectionTool();
}

static QString baseDirectory(const QUrl &url)
{
    QString filePath = url.toLocalFile();
    return QFileInfo(filePath).absoluteDir().path();
}

void SourceTool::selectedItemsChanged(const QList<FormEditorItem*> &itemList)
{
    if (!itemList.isEmpty()) {
        m_formEditorItem = itemList.first();
        m_oldFileName =  m_formEditorItem->qmlItemNode().modelValue("source").toString();

        QString openDirectory = baseDirectory(view()->model()->fileUrl());
        if (openDirectory.isEmpty())
            openDirectory = baseDirectory(view()->model()->fileUrl());

        QString fileName = QFileDialog::getOpenFileName(0,
                                                       tr("Open File"),
                                                       openDirectory);
        fileSelected(fileName);

    } else {
        view()->changeToSelectionTool();
    }
}

void SourceTool::instancesCompleted(const QList<FormEditorItem*> & /*itemList*/)
{
}

void  SourceTool::instancesParentChanged(const QList<FormEditorItem *> & /*itemList*/)
{
}

void SourceTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > & /*propertyList*/)
{
}

void SourceTool::formEditorItemsChanged(const QList<FormEditorItem*> & /*itemList*/)
{
}

int SourceTool::wantHandleItem(const ModelNode &modelNode) const
{
    if (modelNodeHasUrlSource(modelNode))
        return 15;

    return 0;
}

QString SourceTool::name() const
{
    return tr("Source Tool");
}

void SourceTool::fileSelected(const QString &fileName)
{
    if (m_formEditorItem
            && QFileInfo(fileName).isFile()) {
        QString modelFilePath = view()->model()->fileUrl().toLocalFile();
        QDir modelFileDirectory = QFileInfo(modelFilePath).absoluteDir();
        QString relativeFilePath = modelFileDirectory.relativeFilePath(fileName);
        if (m_oldFileName != relativeFilePath) {
            m_formEditorItem->qmlItemNode().setVariantProperty("source", relativeFilePath);
        }
    }

    view()->changeToSelectionTool();
}

}
