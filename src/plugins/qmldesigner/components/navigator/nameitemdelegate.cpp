// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nameitemdelegate.h"

#include <qmath.h>

#include "navigatorview.h"
#include "navigatortreeview.h"
#include "navigatorwidget.h"
#include "choosefrompropertylistdialog.h"
#include "qproxystyle.h"

#include <metainfo.h>
#include <model/modelutils.h>
#include <modelnodecontextmenu.h>
#include <theme.h>
#include <qmldesignerconstants.h>
#include <qmlobjectnode.h>

#include <coreplugin/messagebox.h>
#include <utils/qtcassert.h>

#include <QLineEdit>
#include <QPen>
#include <QPixmapCache>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

namespace QmlDesigner {

int NameItemDelegate::iconOffset = 0;

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
    if (QPixmapCache::find(pixmapKey, &pixmap))
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
    const QIcon icon = modelIndex.data(Qt::DecorationRole).value<QIcon>();
    int pixmapSize = icon.isNull() ? 4 : 16;
    QPixmap pixmap = icon.pixmap(pixmapSize, pixmapSize);

    painter->drawPixmap(styleOption.rect.x() + 1 + delegateMargin,
                        styleOption.rect.y() + 2 + delegateMargin,
                        pixmap);

    pixmapSize += delegateMargin;

    return pixmapSize;
}

static QRect drawText(QPainter *painter,
                      const QStyleOptionViewItem &styleOption,
                      const QModelIndex &modelIndex,
                      int iconOffset)
{
    QString displayString = modelIndex.data(Qt::DisplayRole).toString();

    // Check text length does not exceed available space
    const int extraSpace = 12 + iconOffset;

    displayString = styleOption.fontMetrics.elidedText(displayString,
                                                       Qt::ElideMiddle,
                                                       styleOption.rect.width() - extraSpace);
    const QPoint displayStringOffset = QPoint(5 + iconOffset, -5 - delegateMargin);
    const int width = styleOption.fontMetrics.horizontalAdvance(displayString);
    const QPoint textPosition = styleOption.rect.bottomLeft() + displayStringOffset;
    painter->drawText(textPosition, displayString);

    QRect textFrame;
    textFrame.setTopLeft(textPosition);
    textFrame.setWidth(width);

    return textFrame;
}

static bool isThisOrAncestorLocked(const QModelIndex &modelIndex)
{
    return modelIndex.model()->data(modelIndex, ItemOrAncestorLocked).toBool();
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

static void setId(const QModelIndex &index, const QString &newId)
{
    ModelNode modelNode = getModelNode(index);

    if (!modelNode.isValid())
        return;

    if (modelNode.id() == newId)
        return;

    if (!ModelNode::isValidId(newId)) {
        Core::AsynchronousMessageBox::warning(NavigatorTreeView::tr("Invalid Id"),
                                              NavigatorTreeView::tr("%1 is an invalid id.").arg(newId));
    } else if (modelNode.view()->hasId(newId)) {
        Core::AsynchronousMessageBox::warning(NavigatorTreeView::tr("Invalid Id"),
                                              NavigatorTreeView::tr("%1 already exists.").arg(newId));
    } else {
        modelNode.setIdWithRefactoring(newId);
    }
}

static void openContextMenu(const QModelIndex &index, const QPoint &pos)
{
    const ModelNode modelNode = getModelNode(index);
    QTC_ASSERT(modelNode.isValid(), return);
    ModelNodeContextMenu::showContextMenu(modelNode.view(), pos, QPoint(), false);
}

QSize NameItemDelegate::sizeHint(const QStyleOptionViewItem & /*option*/,
                                 const QModelIndex & /*modelIndex*/) const
{
    return {15, 20 + (2 * delegateMargin)};
}

void NameItemDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem &styleOption,
                             const QModelIndex &modelIndex) const
{
    painter->save();

    painter->setPen(Theme::getColor(Theme::Color::DSnavigatorText));

    if (styleOption.state & QStyle::State_MouseOver && !isThisOrAncestorLocked(modelIndex)) {
        painter->fillRect(styleOption.rect.adjusted(0, delegateMargin, 0, -delegateMargin),
                          Theme::getColor(Theme::Color::DSnavigatorItemBackgroundHover));
        painter->setPen(Theme::getColor(Theme::Color::DSnavigatorTextHover));
    }

    if (styleOption.state & QStyle::State_Selected) {
        NavigatorTreeView::drawSelectionBackground(painter, styleOption);
        painter->setPen(Theme::getColor(Theme::Color::DSnavigatorTextSelected));
    }

    ModelNode node = getModelNode(modelIndex);
    if (!ModelUtils::isThisOrAncestorLocked(node)) {
        NavigatorWidget *widget = qobject_cast<NavigatorWidget *>(styleOption.widget->parent());
        if (widget && !widget->dragType().isEmpty()) {
            QByteArray dragType = widget->dragType();
            const NodeMetaInfo metaInfo = node.metaInfo();

            bool validDrop = false;
            if (dragType == Constants::MIME_TYPE_BUNDLE_TEXTURE) {
                validDrop = metaInfo.isQtQuick3DModel();
            } else if (dragType == Constants::MIME_TYPE_ASSET_TEXTURE3D) {
                validDrop = metaInfo.isQtQuick3DModel() || metaInfo.isQtQuick3DTexture();
            } else if (dragType == Constants::MIME_TYPE_ASSET_IMAGE) {
                validDrop = metaInfo.isQtQuick3DModel() || metaInfo.isQtQuick3DTexture()
                        || metaInfo.isQtQuickImage() || metaInfo.isQtQuickBorderImage();
            } else {
                const NodeMetaInfo dragInfo = node.model()->metaInfo(dragType);
                ChooseFromPropertyListFilter *filter = new ChooseFromPropertyListFilter(dragInfo, metaInfo, true);
                validDrop = !filter->propertyList.isEmpty();
                delete filter;
            }
            if (validDrop) {
                painter->setOpacity(0.5);
                painter->fillRect(styleOption.rect.adjusted(0, delegateMargin, 0, -delegateMargin),
                                  Theme::getColor(Theme::Color::DSnavigatorDropIndicatorBackground));
                painter->setOpacity(1.0);
                painter->setPen(Theme::getColor(Theme::Color::DSnavigatorTextSelected));
            }
        }
    }

    iconOffset = drawIcon(painter, styleOption, modelIndex);

    if (isThisOrAncestorLocked(modelIndex))
        painter->setOpacity(0.5);

    QRect textFrame = drawText(painter, styleOption, modelIndex, iconOffset);

    if (QmlObjectNode(getModelNode(modelIndex)).hasError())
        drawRedWavyUnderLine(painter, styleOption, textFrame);

    painter->restore();
}

QWidget *NameItemDelegate::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem &/*option*/,
                                        const QModelIndex &index) const
{
    if (!getModelNode(index).isValid())
        return nullptr;

    return new QLineEdit(parent);
}

void NameItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    const ModelNode node = getModelNode(index);
    const QString value = node.id();

    auto lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setText(value);
}

void NameItemDelegate::setModelData(QWidget *editor,
                                    [[maybe_unused]] QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    auto lineEdit = static_cast<QLineEdit*>(editor);
    setId(index, lineEdit->text());
    lineEdit->clearFocus();
}

bool NameItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *, const QStyleOptionViewItem &, const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            openContextMenu(index, mouseEvent->globalPosition().toPoint());
            mouseEvent->accept();
            return true;
        }
    }
    return false;
}

void NameItemDelegate::updateEditorGeometry(QWidget *editor,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex &/*index*/) const
{
    auto lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setTextMargins(0, 0, 0, 2);
    // + 2 is shifting the QLineEdit to the left so it will align with the text when activated
    lineEdit->setGeometry(option.rect.adjusted(iconOffset + 2, delegateMargin, 0, -delegateMargin));
}

} // namespace QmlDesigner
