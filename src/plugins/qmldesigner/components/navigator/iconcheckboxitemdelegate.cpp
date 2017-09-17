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

#include "iconcheckboxitemdelegate.h"

#include <qmath.h>

#include "navigatorview.h"
#include "navigatortreeview.h"
#include "navigatortreemodel.h"
#include "qproxystyle.h"

#include "metainfo.h"

#include <utils/qtcassert.h>

#include <QLineEdit>
#include <QPen>
#include <QPixmapCache>
#include <QMouseEvent>
#include <QPainter>

namespace QmlDesigner {

IconCheckboxItemDelegate::IconCheckboxItemDelegate(QObject *parent,
                                                   const QIcon &checkedIcon,
                                                   const QIcon &uncheckedIcon)
    : QStyledItemDelegate(parent),
      m_checkedIcon(checkedIcon),
      m_uncheckedIcon(uncheckedIcon)
{}

QSize IconCheckboxItemDelegate::sizeHint(const QStyleOptionViewItem & /*option*/,
                                         const QModelIndex & /*modelIndex*/) const
{
    return QSize(15, 20);
}

static bool isChecked(const QModelIndex &modelIndex)
{
    return modelIndex.model()->data(modelIndex, Qt::CheckStateRole) == Qt::Checked;
}

static bool isVisible(const QModelIndex &modelIndex)
{
    return modelIndex.model()->data(modelIndex, ItemIsVisibleRole).toBool();
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
    if (rowIsPropertyRole(modelIndex.model(), modelIndex))
        return; //Do not paint icons for property rows

    if (styleOption.state & QStyle::State_Selected)
        NavigatorTreeView::drawSelectionBackground(painter, styleOption);

    if (!getModelNode(modelIndex).isRootNode()) {
        QWindow *window = dynamic_cast<QWidget*>(painter->device())->window()->windowHandle();
        QTC_ASSERT(window, return);

        const QRect iconRect(styleOption.rect.left() + 2, styleOption.rect.top() + 2, 16, 16);
        const QIcon &icon = isChecked(modelIndex) ? m_checkedIcon : m_uncheckedIcon;
        const QPixmap iconPixmap = icon.pixmap(window, iconRect.size());
        const bool visible = isVisible(modelIndex);

        if (!visible) {
            painter->save();
            painter->setOpacity(0.5);
        }

        painter->drawPixmap(iconRect.topLeft(), iconPixmap);

        if (!visible)
            painter->restore();
    }
}

} // namespace QmlDesigner
