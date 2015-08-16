/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#include "contextlabelitem.h"

#include <QFontMetricsF>

namespace qmt {

ContextLabelItem::ContextLabelItem(QGraphicsItem *parent)
    : QGraphicsSimpleTextItem(parent),
      _max_width(0.0)
{
}

ContextLabelItem::~ContextLabelItem()
{
}

void ContextLabelItem::setMaxWidth(double max_width)
{
    if (_max_width != max_width) {
        _max_width = max_width;
        update();
    }
}

void ContextLabelItem::resetMaxWidth()
{
    if (_max_width != 0.0) {
        _max_width = 0.0;
        update();
    }
}

void ContextLabelItem::setContext(const QString &context)
{
    if (_context != context) {
        _context = context;
        update();
    }
}

double ContextLabelItem::getHeight() const
{
    return QFontMetricsF(font()).height();
}

void ContextLabelItem::update()
{
    setText(QString(QStringLiteral("(from %1)")).arg(_context));
    if (_max_width > 0.0) {
        double context_width = boundingRect().width();
        if (context_width > _max_width) {
            setText(QString(QStringLiteral("(%1)")).arg(_context));
            context_width = boundingRect().width();
        }
        if (context_width > _max_width) {
            QFontMetricsF metrics(font());
            setText(metrics.elidedText(QString(QStringLiteral("(%1)")).arg(_context), Qt::ElideMiddle, _max_width));
        }
    }
}

}
