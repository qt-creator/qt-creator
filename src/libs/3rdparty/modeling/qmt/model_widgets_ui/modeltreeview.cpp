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

#include "modeltreeview.h"

#include "qmt/infrastructure/contextmenuaction.h"
#include "qmt/model/melement.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/mrelation.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/model_controller/mselection.h"
#include "qmt/model_ui/sortedtreemodel.h"
#include "qmt/model_ui/treemodel.h"
#include "qmt/tasks/ielementtasks.h"

#include <QPainter>
#include <QMimeData>
#include <QDrag>
#include <QStack>
#include <QSortFilterProxyModel>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMenu>

namespace qmt {

ModelTreeView::ModelTreeView(QWidget *parent)
    : QTreeView(parent),
      m_sortedTreeModel(0),
      m_elementTasks(0)
{
    setHeaderHidden(true);
    setSortingEnabled(false);
    setDragEnabled(true);
    setDropIndicatorShown(false);
    setDefaultDropAction(Qt::MoveAction);
    setAutoExpandDelay(-1);
    setDragDropMode(QAbstractItemView::DragDrop);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
}

ModelTreeView::~ModelTreeView()
{
}

QModelIndex ModelTreeView::currentSourceModelIndex() const
{
    return m_sortedTreeModel->mapToSource(currentIndex());
}

QList<QModelIndex> ModelTreeView::selectedSourceModelIndexes() const
{
    QList<QModelIndex> indexes;
    if (selectionModel()) {
        foreach (const QModelIndex &index, selectionModel()->selection().indexes())
            indexes.append(m_sortedTreeModel->mapToSource(index));
    }
    return indexes;
}

void ModelTreeView::setTreeModel(SortedTreeModel *model)
{
    m_sortedTreeModel = model;
    QTreeView::setModel(model);
}

void ModelTreeView::setElementTasks(IElementTasks *elementTasks)
{
    m_elementTasks = elementTasks;
}

QModelIndex ModelTreeView::mapToSourceModelIndex(const QModelIndex &index) const
{
    return m_sortedTreeModel->mapToSource(index);
}

void ModelTreeView::selectFromSourceModelIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    QModelIndex sortedIndex = m_sortedTreeModel->mapFromSource(index);
    scrollTo(sortedIndex);
    setCurrentIndex(sortedIndex);
    if (selectionModel())
        selectionModel()->select(sortedIndex, QItemSelectionModel::ClearAndSelect);
}

void ModelTreeView::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions);

    TreeModel *treeModel = m_sortedTreeModel->treeModel();
    QMT_CHECK(treeModel);

    QByteArray dragData;
    QDataStream dataStream(&dragData, QIODevice::WriteOnly);

    QIcon dragIcon;

    QModelIndexList indexes;
    if (selectionModel())
        indexes = selectedSourceModelIndexes();
    else if (currentSourceModelIndex().isValid())
        indexes.append(currentSourceModelIndex());
    if (!indexes.isEmpty()) {
        foreach (const QModelIndex &index, indexes) {
            MElement *element = treeModel->element(index);
            if (element) {
                dataStream << element->uid().toString();
                if (dragIcon.isNull()) {
                    QIcon icon = treeModel->icon(index);
                    if (!icon.isNull())
                        dragIcon = icon;
                }
            }
        }
    }

    auto mimeData = new QMimeData;
    mimeData->setData(QStringLiteral("text/model-elements"), dragData);

    if (dragIcon.isNull())
        dragIcon = QIcon(QStringLiteral(":/modelinglib/48x48/generic.png"));

    QPixmap pixmap(48, 48);
    pixmap = dragIcon.pixmap(48, 48);

    auto drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
    drag->setPixmap(pixmap);

    drag->exec();
}

void ModelTreeView::dragEnterEvent(QDragEnterEvent *event)
{
    QTreeView::dragEnterEvent(event);
    event->acceptProposedAction();
}

void ModelTreeView::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeView::dragMoveEvent(event);
    bool accept = false;
    QModelIndex dropIndex = indexAt(event->pos());
    QModelIndex dropSourceModelIndex = m_sortedTreeModel->mapToSource(dropIndex);
    if (dropSourceModelIndex.isValid()) {
        TreeModel *treeModel = m_sortedTreeModel->treeModel();
        QMT_CHECK(treeModel);
        MElement *modelElement = treeModel->element(dropSourceModelIndex);
        if (dynamic_cast<MObject*>(modelElement))
            accept = true;
        if (m_autoDelayIndex == dropIndex) {
            if (m_autoDelayStartTime.elapsed() > 1000) {
                setExpanded(dropIndex, !isExpanded(dropIndex));
                m_autoDelayStartTime.start();
            }
        } else {
            m_autoDelayIndex = dropIndex;
            m_autoDelayStartTime = QTime::currentTime();
            m_autoDelayStartTime.start();
        }
    }
    event->setAccepted(accept);
}

void ModelTreeView::dragLeaveEvent(QDragLeaveEvent *event)
{
    QTreeView::dragLeaveEvent(event);
}

void ModelTreeView::dropEvent(QDropEvent *event)
{
    bool accept = false;
    event->setDropAction(Qt::MoveAction);
    if (event->mimeData()->hasFormat(QStringLiteral("text/model-elements"))) {
        QModelIndex dropIndex = indexAt(event->pos());
        QModelIndex dropSourceModelIndex = m_sortedTreeModel->mapToSource(dropIndex);
        if (dropSourceModelIndex.isValid()) {
            TreeModel *treeModel = m_sortedTreeModel->treeModel();
            QMT_CHECK(treeModel);
            MElement *targetModelElement = treeModel->element(dropSourceModelIndex);
            if (auto targetModelObject = dynamic_cast<MObject *>(targetModelElement)) {
                QDataStream dataStream(event->mimeData()->data(QStringLiteral("text/model-elements")));
                while (dataStream.status() == QDataStream::Ok) {
                    QString key;
                    dataStream >> key;
                    if (!key.isEmpty()) {
                        MElement *modelElement = treeModel->modelController()->findElement(Uid(key));
                        if (modelElement) {
                            if (auto modelObject = dynamic_cast<MObject*>(modelElement)) {
                                if (auto targetModelPackage = dynamic_cast<MPackage *>(targetModelObject)) {
                                    treeModel->modelController()->moveObject(targetModelPackage, modelObject);
                                } else if ((targetModelPackage = dynamic_cast<MPackage *>(targetModelObject->owner()))) {
                                    treeModel->modelController()->moveObject(targetModelPackage, modelObject);
                                } else {
                                    QMT_CHECK(false);
                                }
                            } else if (auto modelRelation = dynamic_cast<MRelation *>(modelElement)) {
                                treeModel->modelController()->moveRelation(targetModelObject, modelRelation);
                            }
                        }
                    }
                }
            }
        }
    }
    event->setAccepted(accept);
}

void ModelTreeView::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event);

    emit treeViewActivated();
}

void ModelTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    QModelIndex sourceModelIndex = m_sortedTreeModel->mapToSource(index);
    if (sourceModelIndex.isValid()) {
        TreeModel *treeModel = m_sortedTreeModel->treeModel();
        QMT_CHECK(treeModel);
        MElement *melement = treeModel->element(sourceModelIndex);
        QMT_CHECK(melement);

        QMenu menu;
        bool addSeparator = false;
        if (m_elementTasks->hasClassDefinition(melement)) {
            menu.addAction(new ContextMenuAction(tr("Show Definition"), QStringLiteral("showDefinition"), &menu));
            addSeparator = true;
        }
        if (m_elementTasks->hasDiagram(melement)) {
            menu.addAction(new ContextMenuAction(tr("Open Diagram"), QStringLiteral("openDiagram"), &menu));
            addSeparator = true;
        }
        if (melement->owner()) {
            if (addSeparator)
                menu.addSeparator();
            menu.addAction(new ContextMenuAction(tr("Delete"), QStringLiteral("delete"),
                                                 QKeySequence(Qt::CTRL + Qt::Key_D), &menu));
        }
        QAction *selectedAction = menu.exec(event->globalPos());
        if (selectedAction) {
            auto action = dynamic_cast<ContextMenuAction *>(selectedAction);
            QMT_CHECK(action);
            if (action->id() == QStringLiteral("showDefinition")) {
                m_elementTasks->openClassDefinition(melement);
            } else if (action->id() == QStringLiteral("openDiagram")) {
                m_elementTasks->openDiagram(melement);
            } else if (action->id() == QStringLiteral("delete")) {
                MSelection selection;
                selection.append(melement->uid(), melement->owner()->uid());
                m_sortedTreeModel->treeModel()->modelController()->deleteElements(selection);
            }
        }
        event->accept();
    }
}

} // namespace qmt
