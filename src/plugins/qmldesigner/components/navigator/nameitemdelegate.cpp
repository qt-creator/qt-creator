/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "nameitemdelegate.h"

#include <qmath.h>

#include "navigatorview.h"
#include "navigatortreeview.h"
#include "navigatortreemodel.h"
#include "qproxystyle.h"

#include "metainfo.h"
#include <QLineEdit>
#include <QPen>
#include <QPixmapCache>
#include <QMouseEvent>
#include <QPainter>

namespace QmlDesigner {

static QPixmap generateWavyPixmap(qreal maxRadius, const QPen &pen)
{
    const qreal radiusBase = qMax(qreal(1), maxRadius);

    QString key = QLatin1String("WaveUnderline-Bauhaus");

    QPixmap pixmap;
    if (QPixmapCache::find(key, pixmap))
        return pixmap;

    const qreal halfPeriod = qMax(qreal(2), qreal(radiusBase * 1.61803399)); // the golden ratio
    const int width = qCeil(100 / (2 * halfPeriod)) * (2 * halfPeriod);
    const int radius = qFloor(radiusBase);

    QPainterPath path;

    qreal xs = 0;
    qreal ys = radius;

    while (xs < width) {
        xs += halfPeriod;
        ys = -ys;
        path.quadTo(xs - halfPeriod / 2, ys, xs, 0);
    }

    pixmap = QPixmap(width, radius * 2);
    pixmap.fill(Qt::transparent);
    {
        QPen wavePen = pen;
        wavePen.setCapStyle(Qt::SquareCap);

        // This is to protect against making the line too fat, as happens on Mac OS X
        // due to it having a rather thick width for the regular underline.
        const qreal maxPenWidth = .8 * radius;
        if (wavePen.widthF() > maxPenWidth)
            wavePen.setWidth(maxPenWidth);

        QPainter imgPainter(&pixmap);
        imgPainter.setPen(wavePen);
        imgPainter.setRenderHint(QPainter::Antialiasing);
        imgPainter.translate(0, radius);
        imgPainter.drawPath(path);
    }

    QPixmapCache::insert(key, pixmap);

    return pixmap;
}

NameItemDelegate::NameItemDelegate(QObject *parent, NavigatorTreeModel *treeModel)
    : QStyledItemDelegate(parent),
      m_TreeModel(treeModel)
{
}

void NameItemDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (option.state & QStyle::State_Selected)
        drawSelectionBackground(painter, option);

    QString displayString;
    QPoint displayStringOffset;

    painter->save();
    QFontMetrics fm(option.font);
    int width = 0;
    if (index.data(Qt::UserRole).isValid()) {

        int pixmapSide = 16;

        if (m_TreeModel->isNodeInvisible( index ))
            painter->setOpacity(0.5);

        ModelNode node = m_TreeModel->nodeForIndex(index);

        QIcon icon;
        if (node.isValid()) {
            // if node has no own icon, search for it in the itemlibrary
            const ItemLibraryInfo *libraryInfo = node.model()->metaInfo().itemLibraryInfo();
            QList <ItemLibraryEntry> infoList = libraryInfo->entriesForType(node.type(),
                                                                            node.majorVersion(),
                                                                            node.minorVersion());
            foreach (const ItemLibraryEntry &entry, infoList) {
                if (icon.isNull()) {
                    icon = entry.icon();
                    break;
                }
            }
        }
        // if the library was also empty, use the default icon
        if (icon.isNull())
            icon = QIcon(QLatin1String(":/ItemLibrary/images/item-default-icon.png"));
        if (!node.metaInfo().isValid())
            icon = QIcon(QLatin1String(":/ItemLibrary/images/item-invalid-icon.png"));

        // If no icon is present, leave an empty space of 24 pixels anyway
        QPixmap pixmap = icon.pixmap(pixmapSide, pixmapSide);
        painter->drawPixmap(option.rect.x()+1,option.rect.y()+2,pixmap);

        displayString = node.id();
        if (displayString.isEmpty())
            displayString = node.simplifiedTypeName();

        // Check text length does not exceed available space
        int extraSpace=12+pixmapSide;

        displayString = fm.elidedText(displayString,Qt::ElideMiddle,option.rect.width()-extraSpace);
        displayStringOffset = QPoint(5+pixmapSide,-5);
        width = fm.width(displayString);
    } else {
        displayString = index.data(Qt::DisplayRole).toString();
        displayStringOffset = QPoint(0, -2);
    }

    QPoint pos = option.rect.bottomLeft() + displayStringOffset;
    painter->drawText(pos, displayString);

    ModelNode node = m_TreeModel->nodeForIndex(index);

    if (!node.isValid() ||!node.metaInfo().isValid()) {
        painter->translate(0, pos.y() + 1);
        QPen pen;
        pen.setColor(Qt::red);

        const qreal underlineOffset = fm.underlinePos();
        const QPixmap wave = generateWavyPixmap(qMax(underlineOffset, pen.widthF()), pen);
        const int descent = fm.descent();

        painter->setBrushOrigin(painter->brushOrigin().x(), 0);
        painter->fillRect(pos.x(), 0, qCeil(width), qMin(wave.height(), descent), wave);
    }

    painter->restore();
}

bool NameItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *, const QStyleOptionViewItem &, const QModelIndex &)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            m_TreeModel->openContextMenu(mouseEvent->globalPos());
            mouseEvent->accept();
            return true;
        }
    }
    return false;
}

QWidget *NameItemDelegate::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    Q_UNUSED(option);
    if (!index.data(Qt::UserRole).isValid())
        return 0;

    return new QLineEdit(parent);
}

void NameItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ModelNode node = m_TreeModel->nodeForIndex(index);
    QString value = node.id();

    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setText(value);
}

void NameItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    Q_UNUSED(model);
    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    m_TreeModel->setId(index,lineEdit->text());
    lineEdit->clearFocus();
}

void NameItemDelegate::updateEditorGeometry(QWidget *editor,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    Q_UNUSED(index);
    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setGeometry(option.rect);
}

} // namespace QmlDesigner
