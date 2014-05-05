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
                                                   QString checkedPixmapURL,
                                                   QString uncheckedPixmapURL,
                                                   NavigatorTreeModel *treeModel)
    : QStyledItemDelegate(parent),
      offPix(uncheckedPixmapURL),
      onPix(checkedPixmapURL),
      m_TreeModel(treeModel)
{}

QSize IconCheckboxItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    if (!index.data(Qt::UserRole).isValid())
        return QSize();

    return QSize(15, 20);
}

void IconCheckboxItemDelegate::paint(QPainter *painter,
                                     const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.data(Qt::UserRole).isValid())
        return;

    painter->save();
    if (option.state & QStyle::State_Selected)
        drawSelectionBackground(painter, option);

    if (!m_TreeModel->nodeForIndex(index).isRootNode()) {

        bool isChecked= (m_TreeModel->itemFromIndex(index)->checkState() == Qt::Checked);

        if (m_TreeModel->isNodeInvisible( index ))
            painter->setOpacity(0.5);

        if (isChecked)
            painter->drawPixmap(option.rect.x()+2,option.rect.y()+2,onPix);
        else
            painter->drawPixmap(option.rect.x()+2,option.rect.y()+2,offPix);
    }
    painter->restore();
}

} // namespace QmlDesigner
