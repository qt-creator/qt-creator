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
    QPixmap pixmap;

    const qreal radiusBase = qMax(qreal(1), maxRadius);
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

    return pixmap;
}

static QPixmap getWavyPixmap(qreal maxRadius, const QPen &pen)
{
    const QString pixmapKey = QStringLiteral("WaveUnderline-Bauhaus");

    QPixmap pixmap;
    if (QPixmapCache::find(pixmapKey, pixmap))
        return pixmap;

    pixmap = generateWavyPixmap(maxRadius, pen);

    QPixmapCache::insert(pixmapKey, pixmap);

    return pixmap;
}

NameItemDelegate::NameItemDelegate(QObject *parent, NavigatorTreeModel *treeModel)
    : QStyledItemDelegate(parent),
      m_navigatorTreeModel(treeModel)
{
}

static int drawIcon(QPainter *painter, const QStyleOptionViewItem &styleOption, const QModelIndex &modelIndex)
{
    QIcon icon = modelIndex.data(Qt::DecorationRole).value<QIcon>();
    int pixmapSize = 16;

    QPixmap pixmap = icon.pixmap(pixmapSize, pixmapSize);
    painter->drawPixmap(styleOption.rect.x() + 1 , styleOption.rect.y() + 2, pixmap);

    return pixmapSize;
}

static QRect drawText(QPainter *painter,
                     const QStyleOptionViewItem &styleOption,
                     const QModelIndex &modelIndex,
                     int iconOffset)
{
    QString displayString = modelIndex.data(Qt::DisplayRole).toString();
    if (displayString.isEmpty())
        displayString = modelIndex.data(NavigatorTreeModel::SimplifiedTypeNameRole).toString();
    QPoint displayStringOffset;
    int width = 0;

    if (modelIndex.data(NavigatorTreeModel::InvisibleRole).toBool())
        painter->setOpacity(0.5);

    // Check text length does not exceed available space
    int extraSpace = 12 + iconOffset;

    displayString = styleOption.fontMetrics.elidedText(displayString, Qt::ElideMiddle, styleOption.rect.width() - extraSpace);
    displayStringOffset = QPoint(5 + iconOffset, -5);
    width = styleOption.fontMetrics.width(displayString);

    QPoint textPosition = styleOption.rect.bottomLeft() + displayStringOffset;
    painter->drawText(textPosition, displayString);

    QRect textFrame;
    textFrame.setTopLeft(textPosition);
    textFrame.setWidth(width);

    return textFrame;
}

static void drawRedWavyUnderLine(QPainter *painter,
                                 const QStyleOptionViewItem &styleOption,
                                 const QRect &textFrame)
{
    painter->translate(0, textFrame.y() + 1);
    QPen pen;
    pen.setColor(Qt::red);
    const qreal underlineOffset = styleOption.fontMetrics.underlinePos();
    const QPixmap wave = getWavyPixmap(qMax(underlineOffset, pen.widthF()), pen);
    const int descent = styleOption.fontMetrics.descent();

    painter->setBrushOrigin(painter->brushOrigin().x(), 0);
    painter->fillRect(textFrame.x(), 0, qCeil(textFrame.width()), qMin(wave.height(), descent), wave);
}

void NameItemDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem &styleOption,
                             const QModelIndex &modelIndex) const
{
    painter->save();
    if (styleOption.state & QStyle::State_Selected)
        NavigatorTreeView::drawSelectionBackground(painter, styleOption);

    int iconOffset = drawIcon(painter, styleOption, modelIndex);

    QRect textFrame = drawText(painter, styleOption, modelIndex, iconOffset);

    if (modelIndex.data(NavigatorTreeModel::ErrorRole).toBool())
        drawRedWavyUnderLine(painter, styleOption, textFrame);

    painter->restore();
}

bool NameItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *, const QStyleOptionViewItem &, const QModelIndex &)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            m_navigatorTreeModel->openContextMenu(mouseEvent->globalPos());
            mouseEvent->accept();
            return true;
        }
    }
    return false;
}

QWidget *NameItemDelegate::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem & /*option*/,
                                        const QModelIndex &index) const
{
    if (!m_navigatorTreeModel->hasNodeForIndex(index))
        return 0;

    return new QLineEdit(parent);
}

void NameItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ModelNode node = m_navigatorTreeModel->nodeForIndex(index);
    QString value = node.id();

    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setText(value);
}

void NameItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    Q_UNUSED(model);
    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    m_navigatorTreeModel->setId(index,lineEdit->text());
    lineEdit->clearFocus();
}

void NameItemDelegate::updateEditorGeometry(QWidget *editor,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex & /*index*/) const
{
    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setGeometry(option.rect);
}

} // namespace QmlDesigner
