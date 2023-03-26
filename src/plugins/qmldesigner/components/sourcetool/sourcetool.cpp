// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sourcetool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "itemutilfunctions.h"
#include "formeditoritem.h"

#include "resizehandleitem.h"

#include "nodemetainfo.h"
#include "qmlitemnode.h"
#include <designeractionmanager.h>
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
    return metaInfo.isValid() && metaInfo.hasProperty("source")
           && metaInfo.property("source").propertyType().isUrl();
}

} //namespace

namespace QmlDesigner {

SourceTool::SourceTool()
{
}

SourceTool::~SourceTool() = default;

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
        m_formEditorItem = itemList.constFirst();
        m_oldFileName =  m_formEditorItem->qmlItemNode().modelValue("source").toString();

        QString openDirectory = baseDirectory(view()->model()->fileUrl());
        if (openDirectory.isEmpty())
            openDirectory = baseDirectory(view()->model()->fileUrl());

        QString fileName = QFileDialog::getOpenFileName(nullptr,
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
