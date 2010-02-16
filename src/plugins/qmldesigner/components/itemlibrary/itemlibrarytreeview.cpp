/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "itemlibrarytreeview.h"
#include "itemlibrary.h"
#include "customdraganddrop.h"

#include <QtCore/QMimeData>
#include <QtCore/QDebug>

#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QDrag>
#include <QPainter>
#include <QLabel>
#include <itemlibraryinfo.h>
#include <QDirModel>


enum { debug = 0 };


namespace QmlDesigner {

namespace Internal {


ItemLibraryTreeView::ItemLibraryTreeView(QWidget *parent) :
        QTreeView(parent)
{
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setUniformRowHeights(true);
    connect(this, SIGNAL(clicked(const QModelIndex &)), this, SLOT(activateItem(const QModelIndex &)));
}

// We need to implement startDrag ourselves since we cannot
// otherwise influence drag pixmap and hotspot in the standard
// implementation.
void ItemLibraryTreeView::startDrag(Qt::DropActions /* supportedActions */)
{
    if (debug)
        qDebug() << Q_FUNC_INFO;
    QMimeData *mimeData = model()->mimeData(selectedIndexes());
    if (!mimeData)
        return;

    QDirModel *dirModel = qobject_cast<QDirModel*>(model());
    Q_ASSERT(dirModel);
    QFileInfo fileInfo = dirModel->fileInfo(selectedIndexes().front());
    QPixmap pixmap(fileInfo.absoluteFilePath());
    if (!pixmap.isNull()) {
        CustomItemLibraryDrag *drag = new CustomItemLibraryDrag(this);
        drag->setPreview(pixmap);
        drag->setPixmap(QIcon(pixmap).pixmap(128, 128));
        QMimeData *mimeData = new QMimeData;
        mimeData->setData("application/vnd.bauhaus.libraryresource", fileInfo.absoluteFilePath().toLatin1());
        drag->setMimeData(mimeData);
        drag->exec();
    }
}

void ItemLibraryTreeView::activateItem( const QModelIndex & /*index*/)
{
    QMimeData *mimeData = model()->mimeData(selectedIndexes());
    if (!mimeData)
        return;

    QString name;
    QDirModel *dirModel = qobject_cast<QDirModel*>(model());
    Q_ASSERT(dirModel);
    QFileInfo fileInfo = dirModel->fileInfo(selectedIndexes().front());
    QPixmap pixmap(fileInfo.absoluteFilePath());
    if (!pixmap.isNull()) {
        name = "image^" + fileInfo.absoluteFilePath();
        emit itemActivated(name);
    }
}

} // namespace Internal

} // namespace QmlDesigner

