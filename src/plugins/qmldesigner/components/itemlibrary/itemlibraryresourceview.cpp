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

#include "itemlibraryresourceview.h"

#include "customfilesystemmodel.h"

#include <QAction>
#include <QActionGroup>
#include <QDebug>
#include <QDrag>
#include <QFileSystemModel>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>

#include <QProxyStyle>

#include <functional>

enum { debug = 0 };

namespace QmlDesigner {

void ItemLibraryResourceView::addSizeAction(QActionGroup *group, const QString &text, int gridSize, int iconSize)
{
    QAction *action = new QAction(text, group);
    group->addAction(action);
    action->setCheckable(true);
    QAction::connect(action, &QAction::triggered, this, [this, gridSize, iconSize]() {
        setViewMode(QListView::IconMode);
        setGridSize(QSize(gridSize, gridSize));
        setIconSize(QSize(iconSize, iconSize));
        setDragEnabled(true);
        setWrapping(true);
    });
}

ItemLibraryResourceView::ItemLibraryResourceView(QWidget *parent) :
        QListView(parent)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    setGridSize(QSize(128, 128));
    setIconSize(QSize(96, 96));
    setSpacing(4);

    setViewMode(QListView::IconMode);
    setMovement(QListView::Static);
    setResizeMode(QListView::Adjust);
    setSelectionRectVisible(false);
    setWrapping(true);
    setWordWrap(true);

    setDragDropMode(QAbstractItemView::DragOnly);

    setContextMenuPolicy(Qt::ActionsContextMenu);

    QActionGroup *actionGroup = new QActionGroup(this);
    actionGroup->setExclusive(true);

    addSizeAction(actionGroup, tr("Large Icons"), 256, 192);
    addSizeAction(actionGroup, tr("Medium Icons"), 128, 96);
    addSizeAction(actionGroup, tr("Small Icons"), 96, 48);

    QAction *action = new QAction(tr("List"), actionGroup);
    actionGroup->addAction(action);
    action->setCheckable(true);
    QAction::connect(action, &QAction::triggered, this, [this](){
        setViewMode(QListView::ListMode);
        setGridSize(QSize());
        setIconSize(QSize(32, 32));
        setDragEnabled(true);
        setWrapping(false);
    });

    QAction *defaultAction = actionGroup->actions().at(1);
    defaultAction->toggle();

    addActions(actionGroup->actions());
}

void ItemLibraryResourceView::startDrag(Qt::DropActions /* supportedActions */)
{
    if (debug)
        qDebug() << Q_FUNC_INFO;
    QMimeData *mimeData = model()->mimeData(selectedIndexes());

    if (!mimeData)
        return;

    CustomFileSystemModel *fileSystemModel = qobject_cast<CustomFileSystemModel*>(model());
    Q_ASSERT(fileSystemModel);
    QFileInfo fileInfo = fileSystemModel->fileInfo(selectedIndexes().front());
    QPixmap pixmap(fileInfo.absoluteFilePath());
    if (!pixmap.isNull()) {
        QDrag *drag = new QDrag(this);
        drag->setPixmap(QIcon(pixmap).pixmap(128, 128));
        QMimeData *mimeData = new QMimeData;
        mimeData->setData(QLatin1String("application/vnd.bauhaus.libraryresource"), fileInfo.absoluteFilePath().toUtf8());
        drag->setMimeData(mimeData);
        drag->exec();
    }
}

} // namespace QmlDesigner

