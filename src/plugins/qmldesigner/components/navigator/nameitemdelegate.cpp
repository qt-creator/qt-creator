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

#include "nameitemdelegate.h"

#include <qmath.h>

#include "navigatorview.h"
#include "navigatortreeview.h"
#include "qproxystyle.h"

#include <metainfo.h>
#include <modelnodecontextmenu.h>
#include <qmlobjectnode.h>

#include <coreplugin/messagebox.h>
#include <utils/qtcassert.h>

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

NameItemDelegate::NameItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

static int drawIcon(QPainter *painter, const QStyleOptionViewItem &styleOption, const QModelIndex &modelIndex)
{
    QIcon icon = modelIndex.data(Qt::DecorationRole).value<QIcon>();

    const int pixmapSize = icon.isNull() ? 4 : 16;

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
        displayString = modelIndex.data(Qt::DisplayRole).toString();
    QPoint displayStringOffset;
    int width = 0;

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

static bool isVisible(const QModelIndex &modelIndex)
{
    return modelIndex.model()->data(modelIndex, ItemIsVisibleRole).toBool();
}

static ModelNode getModelNode(const QModelIndex &modelIndex)
{
    return modelIndex.model()->data(modelIndex, ModelNodeRole).value<ModelNode>();
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

    if (!isVisible(modelIndex))
        painter->setOpacity(0.5);

    QRect textFrame = drawText(painter, styleOption, modelIndex, iconOffset);

    if (QmlObjectNode(getModelNode(modelIndex)).hasError())
        drawRedWavyUnderLine(painter, styleOption, textFrame);

    painter->restore();
}

static void openContextMenu(const QModelIndex &index, const QPoint &pos)
{
    const ModelNode modelNode = getModelNode(index);
    QTC_ASSERT(modelNode.isValid(), return);
    ModelNodeContextMenu::showContextMenu(modelNode.view(), pos, QPoint(), false);
}

bool NameItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *, const QStyleOptionViewItem &, const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            openContextMenu(index, mouseEvent->globalPos());
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
    if (!getModelNode(index).isValid())
        return 0;

    return new QLineEdit(parent);
}

void NameItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    const ModelNode node = getModelNode(index);
    const QString value = node.id();

    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setText(value);
}

static void setId(const QModelIndex &index, const QString &newId)
{
    ModelNode modelNode = getModelNode(index);

    if (!modelNode.isValid())
        return;

    if (modelNode.id() == newId)
        return;

    if (!modelNode.isValidId(newId)) {
        Core::AsynchronousMessageBox::warning(NavigatorTreeView::tr("Invalid Id"),
                                              NavigatorTreeView::tr("%1 is an invalid id.").arg(newId));
    } else if (modelNode.view()->hasId(newId)) {
        Core::AsynchronousMessageBox::warning(NavigatorTreeView::tr("Invalid Id"),
                                              NavigatorTreeView::tr("%1 already exists.").arg(newId));
    } else  {
        modelNode.setIdWithRefactoring(newId);
    }
}


void NameItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    Q_UNUSED(model);
    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    setId(index,lineEdit->text());
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
