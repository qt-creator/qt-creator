/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "contextlabelitem.h"

#include <QFontMetricsF>

namespace qmt {

ContextLabelItem::ContextLabelItem(QGraphicsItem *parent)
    : QGraphicsSimpleTextItem(parent)
{
}

ContextLabelItem::~ContextLabelItem()
{
}

void ContextLabelItem::setMaxWidth(double maxWidth)
{
    if (m_maxWidth != maxWidth) {
        m_maxWidth = maxWidth;
        update();
    }
}

void ContextLabelItem::resetMaxWidth()
{
    if (m_maxWidth != 0.0) {
        m_maxWidth = 0.0;
        update();
    }
}

void ContextLabelItem::setContext(const QString &context)
{
    if (m_context != context) {
        m_context = context;
        update();
    }
}

double ContextLabelItem::height() const
{
    return QFontMetricsF(font()).height();
}

void ContextLabelItem::update()
{
    setText(QString("(from %1)").arg(m_context));
    if (m_maxWidth > 0.0) {
        double contextWidth = boundingRect().width();
        if (contextWidth > m_maxWidth) {
            setText(QString("(%1)").arg(m_context));
            contextWidth = boundingRect().width();
        }
        if (contextWidth > m_maxWidth) {
            QFontMetricsF metrics(font());
            setText(metrics.elidedText(QString("(%1)").arg(m_context), Qt::ElideMiddle, m_maxWidth));
        }
    }
}

} // namespace qmt
