/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "itemlibrarycomponents.h"
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
#include <QProxyStyle>

enum { debug = 0 };

namespace QmlDesigner {

namespace Internal {

static void drawSelectionBackground(QPainter *painter, const QStyleOption &option)
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
                drawSelectionBackground(painter, *option);
        }
    }
    int styleHint(StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0, QStyleHintReturn *returnData = 0) const {
        if (hint == SH_ItemView_ShowDecorationSelected)
            return 0;
        else
            return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

ItemLibraryTreeView::ItemLibraryTreeView(QWidget *parent) :
        QTreeView(parent)
{
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setUniformRowHeights(true);
    connect(this, SIGNAL(clicked(const QModelIndex &)), this, SLOT(activateItem(const QModelIndex &)));
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

    QDirModel *dirModel = qobject_cast<QDirModel*>(model());
    Q_ASSERT(dirModel);
    QFileInfo fileInfo = dirModel->fileInfo(selectedIndexes().front());
    QPixmap pixmap(fileInfo.absoluteFilePath());
    if (!pixmap.isNull()) {
        CustomItemLibraryDrag *drag = new CustomItemLibraryDrag(this);
        drag->setPreview(pixmap);
        drag->setPixmap(QIcon(pixmap).pixmap(128, 128));
        QMimeData *mimeData = new QMimeData;
        mimeData->setData("application/vnd.bauhaus.libraryresource", fileInfo.absoluteFilePath().toUtf8());
        drag->setMimeData(mimeData);
        drag->exec();
    }
}

void ItemLibraryTreeView::setModel(QAbstractItemModel *model)
{
    QDirModel *dirModel = dynamic_cast<QDirModel *>(model);
    if (dirModel) {
        QTreeView::setModel(model);
        m_delegate->setModel(dirModel);
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
    QDirModel *dirModel = qobject_cast<QDirModel*>(model());
    Q_ASSERT(dirModel);
    QFileInfo fileInfo = dirModel->fileInfo(selectedIndexes().front());
    QPixmap pixmap(fileInfo.absoluteFilePath());
    if (!pixmap.isNull()) {
        name = "image^" + fileInfo.absoluteFilePath();
        emit itemActivated(name);
    }
}

void ResourceItemDelegate::paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (option.state & QStyle::State_Selected)
        drawSelectionBackground(painter, option);

    painter->save();

    QIcon icon(m_model->fileIcon(index));
    QPixmap pixmap(icon.pixmap(icon.availableSizes().front()));
    painter->drawPixmap(option.rect.x(),option.rect.y()+2,pixmap);
    QString myString(m_model->fileName(index));

    // Check text length does not exceed available space
    int extraSpace=12+pixmap.width();
    QFontMetrics fm(option.font);
    myString = fm.elidedText(myString,Qt::ElideMiddle,option.rect.width()-extraSpace);

    painter->drawText(option.rect.bottomLeft()+QPoint(3+pixmap.width(),-8),myString);

    painter->restore();
}

QSize ResourceItemDelegate::sizeHint(const QStyleOptionViewItem &/*option*/,
                                     const QModelIndex &index) const
{
    QIcon icon(m_model->fileIcon(index));
    return icon.availableSizes().front() + QSize(25, 4);
}

void ResourceItemDelegate::setModel(QDirModel *model)
{
    m_model = model;
}

} // namespace Internal

} // namespace QmlDesigner

