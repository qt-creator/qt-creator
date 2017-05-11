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
#include <QLineEdit>
#include <QPen>
#include <QPixmapCache>
#include <QMouseEvent>
#include <QPainter>

namespace QmlDesigner {

IconCheckboxItemDelegate::IconCheckboxItemDelegate(QObject *parent,
                                                   const QPixmap &checkedPixmap,
                                                   const QPixmap &uncheckedPixmap,
                                                   NavigatorTreeModel *treeModel)
    : QStyledItemDelegate(parent),
      m_checkedPixmap(checkedPixmap),
      m_uncheckedPixmap(uncheckedPixmap),
      m_navigatorTreeModel(treeModel)
{}

QSize IconCheckboxItemDelegate::sizeHint(const QStyleOptionViewItem & /*option*/,
                                         const QModelIndex & /*modelIndex*/) const
{
    return QSize(15, 20);
}

static bool isChecked(const QAbstractItemModel *model, const QModelIndex &modelIndex)
{
    return model->data(modelIndex, Qt::CheckStateRole) == Qt::Checked;
}

static bool isVisible(const QAbstractItemModel *model, const QModelIndex &modelIndex)
{
    return model->data(modelIndex, ItemIsVisibleRole).toBool();
}

void IconCheckboxItemDelegate::paint(QPainter *painter,
                                     const QStyleOptionViewItem &styleOption,
                                     const QModelIndex &modelIndex) const
{
    const int yOffset = (styleOption.rect.height()
                         - (m_checkedPixmap.height() / painter->device()->devicePixelRatio())) / 2;
    const int xOffset = 2;

    painter->save();
    if (styleOption.state & QStyle::State_Selected)
        NavigatorTreeView::drawSelectionBackground(painter, styleOption);

    if (!m_navigatorTreeModel->modelNodeForIndex(modelIndex).isRootNode()) {

        if (!isVisible(modelIndex.model(), modelIndex))
            painter->setOpacity(0.5);

        const bool checked = isChecked(modelIndex.model(), modelIndex);
        painter->drawPixmap(styleOption.rect.x() + xOffset, styleOption.rect.y() + yOffset,
                            checked ? m_checkedPixmap : m_uncheckedPixmap);
    }

    painter->restore();
}

} // namespace QmlDesigner
