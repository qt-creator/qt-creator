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

QModelIndex ModelTreeView::getCurrentSourceModelIndex() const
{
    return m_sortedTreeModel->mapToSource(currentIndex());
}

QList<QModelIndex> ModelTreeView::getSelectedSourceModelIndexes() const
{
    QList<QModelIndex> indexes;
    if (selectionModel()) {
        foreach (const QModelIndex &index, selectionModel()->selection().indexes()) {
            indexes.append(m_sortedTreeModel->mapToSource(index));
        }
    }
    return indexes;
}

void ModelTreeView::setTreeModel(SortedTreeModel *model)
{
    m_sortedTreeModel = model;
    QTreeView::setModel(model);
}

void ModelTreeView::setElementTasks(IElementTasks *element_tasks)
{
    m_elementTasks = element_tasks;
}

QModelIndex ModelTreeView::mapToSourceModelIndex(const QModelIndex &index) const
{
    return m_sortedTreeModel->mapToSource(index);
}

void ModelTreeView::selectFromSourceModelIndex(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    QModelIndex sorted_index = m_sortedTreeModel->mapFromSource(index);
    scrollTo(sorted_index);
    setCurrentIndex(sorted_index);
    if (selectionModel()) {
        selectionModel()->select(sorted_index, QItemSelectionModel::ClearAndSelect);
    }
}

void ModelTreeView::startDrag(Qt::DropActions supported_actions)
{
    Q_UNUSED(supported_actions);

    TreeModel *tree_model = m_sortedTreeModel->getTreeModel();
    QMT_CHECK(tree_model);

    QByteArray drag_data;
    QDataStream data_stream(&drag_data, QIODevice::WriteOnly);

    QIcon drag_icon;

    QModelIndexList indexes;
    if (selectionModel()) {
        indexes = getSelectedSourceModelIndexes();
    } else if (getCurrentSourceModelIndex().isValid()) {
        indexes.append(getCurrentSourceModelIndex());
    }
    if (!indexes.isEmpty()) {
        foreach (const QModelIndex &index, indexes) {
            MElement *element = tree_model->getElement(index);
            if (element) {
                data_stream << element->getUid().toString();
                if (drag_icon.isNull()) {
                    QIcon icon = tree_model->getIcon(index);
                    if (!icon.isNull()) {
                        drag_icon = icon;
                    }
                }
            }
        }
    }

    QMimeData *mime_data = new QMimeData;
    mime_data->setData(QStringLiteral("text/model-elements"), drag_data);

    if (drag_icon.isNull()) {
        drag_icon = QIcon(QStringLiteral(":/modelinglib/48x48/generic.png"));
    }

    QPixmap pixmap(48, 48);
    pixmap = drag_icon.pixmap(48, 48);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mime_data);
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
    QModelIndex drop_index = indexAt(event->pos());
    QModelIndex drop_source_model_index = m_sortedTreeModel->mapToSource(drop_index);
    if (drop_source_model_index.isValid()) {
        TreeModel *tree_model = m_sortedTreeModel->getTreeModel();
        QMT_CHECK(tree_model);
        MElement *model_element = tree_model->getElement(drop_source_model_index);
        if (dynamic_cast<MObject*>(model_element)) {
            accept = true;
        }
        if (m_autoDelayIndex == drop_index) {
            if (m_autoDelayStartTime.elapsed() > 1000) {
                setExpanded(drop_index, !isExpanded(drop_index));
                m_autoDelayStartTime.start();
            }
        } else {
            m_autoDelayIndex = drop_index;
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
        QModelIndex drop_index = indexAt(event->pos());
        QModelIndex drop_source_model_index = m_sortedTreeModel->mapToSource(drop_index);
        if (drop_source_model_index.isValid()) {
            TreeModel *tree_model = m_sortedTreeModel->getTreeModel();
            QMT_CHECK(tree_model);
            MElement *target_model_element = tree_model->getElement(drop_source_model_index);
            if (MObject *target_model_object = dynamic_cast<MObject *>(target_model_element)) {
                QDataStream data_stream(event->mimeData()->data(QStringLiteral("text/model-elements")));
                while (data_stream.status() == QDataStream::Ok) {
                    QString key;
                    data_stream >> key;
                    if (!key.isEmpty()) {
                        MElement *model_element = tree_model->getModelController()->findElement(Uid(key));
                        if (model_element) {
                            if (MObject *model_object = dynamic_cast<MObject*>(model_element)) {
                                if (MPackage *target_model_package = dynamic_cast<MPackage*>(target_model_object)) {
                                    tree_model->getModelController()->moveObject(target_model_package, model_object);
                                } else if ((target_model_package = dynamic_cast<MPackage *>(target_model_object->getOwner()))) {
                                    tree_model->getModelController()->moveObject(target_model_package, model_object);
                                } else {
                                    QMT_CHECK(false);
                                }
                            } else if (MRelation *model_relation = dynamic_cast<MRelation *>(model_element)) {
                                tree_model->getModelController()->moveRelation(target_model_object, model_relation);
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
    QModelIndex source_model_index = m_sortedTreeModel->mapToSource(index);
    if (source_model_index.isValid()) {
        TreeModel *tree_model = m_sortedTreeModel->getTreeModel();
        QMT_CHECK(tree_model);
        MElement *melement = tree_model->getElement(source_model_index);
        QMT_CHECK(melement);

        QMenu menu;
        bool add_separator = false;
        if (m_elementTasks->hasClassDefinition(melement)) {
            menu.addAction(new ContextMenuAction(tr("Show Definition"), QStringLiteral("showDefinition"), &menu));
            add_separator = true;
        }
        if (m_elementTasks->hasDiagram(melement)) {
            menu.addAction(new ContextMenuAction(tr("Open Diagram"), QStringLiteral("openDiagram"), &menu));
            add_separator = true;
        }
        if (melement->getOwner()) {
            if (add_separator) {
                menu.addSeparator();
            }
            menu.addAction(new ContextMenuAction(tr("Delete"), QStringLiteral("delete"), QKeySequence(Qt::CTRL + Qt::Key_D), &menu));
        }
        QAction *selected_action = menu.exec(event->globalPos());
        if (selected_action) {
            ContextMenuAction *action = dynamic_cast<ContextMenuAction *>(selected_action);
            QMT_CHECK(action);
            if (action->getId() == QStringLiteral("showDefinition")) {
                m_elementTasks->openClassDefinition(melement);
            } else if (action->getId() == QStringLiteral("openDiagram")) {
                m_elementTasks->openDiagram(melement);
            } else if (action->getId() == QStringLiteral("delete")) {
                MSelection selection;
                selection.append(melement->getUid(), melement->getOwner()->getUid());
                m_sortedTreeModel->getTreeModel()->getModelController()->deleteElements(selection);
            }
        }
        event->accept();
    }
}

}
