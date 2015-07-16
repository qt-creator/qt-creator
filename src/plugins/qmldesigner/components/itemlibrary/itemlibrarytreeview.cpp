/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "itemlibrarytreeview.h"

#include "resourceitemdelegate.h"

#include <QMimeData>
#include <QDebug>

#include <QPixmap>
#include <QDrag>
#include <QPainter>
#include <QFileSystemModel>
#include <QProxyStyle>

enum { debug = 0 };

namespace QmlDesigner {

void ItemLibraryTreeView::drawSelectionBackground(QPainter *painter, const QStyleOption &option)
{
    painter->save();
    QLinearGradient gradient;
    QColor highlight = option.palette.highlight().color();
    gradient.setColorAt(0, highlight.lighter(130));
    gradient.setColorAt(1, highlight.darker(130));
    gradient.setStart(option.rect.topLeft());
    gradient.setFinalStop(option.rect.bottomLeft());
    painter->fillRect(option.rect, gradient);
    painter->setPen(highlight.lighter());
    painter->drawLine(option.rect.topLeft(),option.rect.topRight());
    painter->setPen(highlight.darker());
    painter->drawLine(option.rect.bottomLeft(),option.rect.bottomRight());
    painter->restore();
}

namespace {
// This style basically allows us to span the entire row
// including the arrow indicators which would otherwise not be
// drawn by the delegate
class TreeViewStyle : public QProxyStyle
{
public:
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget * = 0) const
    {
        if (element == QStyle::PE_PanelItemViewRow) {
            if (option->state & QStyle::State_Selected)
                ItemLibraryTreeView::drawSelectionBackground(painter, *option);
        }
    }
    int styleHint(StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0, QStyleHintReturn *returnData = 0) const {
        if (hint == SH_ItemView_ShowDecorationSelected)
            return 0;
        else
            return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

}
ItemLibraryTreeView::ItemLibraryTreeView(QWidget *parent) :
        QTreeView(parent)
{
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setUniformRowHeights(true);
    connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(activateItem(QModelIndex)));
    setHeaderHidden(true);
    setIndentation(20);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    TreeViewStyle *style = new TreeViewStyle;
    setStyle(style);
    style->setParent(this);
    m_delegate = new ResourceItemDelegate(this);
    setItemDelegateForColumn(0, m_delegate);
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

    QFileSystemModel *fileSystemModel = qobject_cast<QFileSystemModel*>(model());
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

void ItemLibraryTreeView::setModel(QAbstractItemModel *model)
{
    QFileSystemModel *fileSystemModel = dynamic_cast<QFileSystemModel *>(model);
    if (fileSystemModel) {
        QTreeView::setModel(model);
        m_delegate->setModel(fileSystemModel);
        setColumnHidden(1, true);
        setColumnHidden(2, true);
        setColumnHidden(3, true);
        setSortingEnabled(true);
    }
}

void ItemLibraryTreeView::activateItem( const QModelIndex & /*index*/)
{
    QMimeData *mimeData = model()->mimeData(selectedIndexes());
    if (!mimeData)
        return;

    QString name;
    QFileSystemModel *fileSystemModel = qobject_cast<QFileSystemModel*>(model());
    Q_ASSERT(fileSystemModel);
    QFileInfo fileInfo = fileSystemModel->fileInfo(selectedIndexes().front());
    QPixmap pixmap(fileInfo.absoluteFilePath());
    if (!pixmap.isNull()) {
        name = QLatin1String("image^") + fileInfo.absoluteFilePath();
        emit itemActivated(name);
    }
}



} // namespace QmlDesigner

