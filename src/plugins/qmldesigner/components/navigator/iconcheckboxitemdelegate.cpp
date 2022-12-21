// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iconcheckboxitemdelegate.h"

#include <qmath.h>

#include "navigatorview.h"
#include "navigatortreeview.h"
#include "navigatortreemodel.h"
#include "qproxystyle.h"

#include <metainfo.h>
#include <theme.h>

#include <utils/qtcassert.h>

#include <QLineEdit>
#include <QPen>
#include <QPixmapCache>
#include <QMouseEvent>
#include <QPainter>

namespace QmlDesigner {

IconCheckboxItemDelegate::IconCheckboxItemDelegate(QObject *parent, const QIcon &icon)
    : QStyledItemDelegate(parent),
      m_icon(icon)
{}

QSize IconCheckboxItemDelegate::sizeHint(const QStyleOptionViewItem & /*option*/,
                                         const QModelIndex & /*modelIndex*/) const
{
    return {15, 20};
}

static bool isChecked(const QModelIndex &modelIndex)
{
    return modelIndex.model()->data(modelIndex, Qt::CheckStateRole) == Qt::Checked;
}

static bool isThisOrAncestorLocked(const QModelIndex &modelIndex)
{
    return modelIndex.model()->data(modelIndex, ItemOrAncestorLocked).toBool();
}

static ModelNode getModelNode(const QModelIndex &modelIndex)
{
    return modelIndex.model()->data(modelIndex, ModelNodeRole).value<ModelNode>();
}

static bool rowIsPropertyRole(const QAbstractItemModel *model, const QModelIndex &modelIndex)
{
    return model->data(modelIndex, RowIsPropertyRole).toBool();
}

void IconCheckboxItemDelegate::paint(QPainter *painter,
                                     const QStyleOptionViewItem &styleOption,
                                     const QModelIndex &modelIndex) const
{
    QIcon::Mode mode = QIcon::Mode::Normal;

    if (styleOption.state & QStyle::State_MouseOver && !isThisOrAncestorLocked(modelIndex)) {
        painter->fillRect(styleOption.rect.adjusted(0, delegateMargin, 0, -delegateMargin),
                          Theme::getColor(Theme::Color::DSnavigatorItemBackgroundHover));
        mode = QIcon::Mode::Active; // hover
    }

    if (styleOption.state & QStyle::State_Selected) {
        NavigatorTreeView::drawSelectionBackground(painter, styleOption);
        mode = QIcon::Mode::Selected;
    }

    bool isVisibilityIcon = modelIndex.column() != NavigatorTreeModel::ColumnType::Visibility;
    // We need to invert the check status if visibility icon
    bool checked = isVisibilityIcon ? isChecked(modelIndex) : !isChecked(modelIndex);
    if (!(styleOption.state & QStyle::State_MouseOver) && !checked)
        return;

    if (rowIsPropertyRole(modelIndex.model(), modelIndex) || getModelNode(modelIndex).isRootNode())
        return; // Do not paint icons for property rows or root node

    QWindow *window = dynamic_cast<QWidget*>(painter->device())->window()->windowHandle();
    QTC_ASSERT(window, return);

    const QSize iconSize(16, 16);
    const QPoint iconPosition(styleOption.rect.left() + (styleOption.rect.width() - iconSize.width()) / 2,
                              styleOption.rect.top() + 2 + delegateMargin);

    const QIcon::State state = isChecked(modelIndex) ? QIcon::State::On : QIcon::State::Off;
    const QPixmap iconPixmap = m_icon.pixmap(window, iconSize, mode, state);

    painter->save();

    if (isThisOrAncestorLocked(modelIndex))
        painter->setOpacity(0.5);

    painter->drawPixmap(iconPosition, iconPixmap);

    painter->restore();
}


bool IconCheckboxItemDelegate::editorEvent(QEvent *event,
                                           QAbstractItemModel *model,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &index)
{
    Q_ASSERT(event);
    Q_ASSERT(model);

    // make sure that the item is checkable
    Qt::ItemFlags flags = model->flags(index);
    if (!(flags & Qt::ItemIsUserCheckable) || !(option.state & QStyle::State_Enabled)
        || !(flags & Qt::ItemIsEnabled))
        return false;

    // make sure that we have a check state
    QVariant value = index.data(Qt::CheckStateRole);
    if (!value.isValid())
        return false;

    // make sure that we have the right event type
    if ((event->type() == QEvent::MouseButtonRelease)
        || (event->type() == QEvent::MouseButtonDblClick)
        || (event->type() == QEvent::MouseButtonPress)) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton || !option.rect.contains(me->pos()))
            return false;

        if ((event->type() == QEvent::MouseButtonPress)
            || (event->type() == QEvent::MouseButtonDblClick))
            return true;

    } else if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space
         && static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select)
            return false;
    } else {
        return false;
    }

    Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
    if (flags & Qt::ItemIsUserTristate)
        state = ((Qt::CheckState)((state + 1) % 3));
    else
        state = (state == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
    return model->setData(index, state, Qt::CheckStateRole);
}

} // namespace QmlDesigner
