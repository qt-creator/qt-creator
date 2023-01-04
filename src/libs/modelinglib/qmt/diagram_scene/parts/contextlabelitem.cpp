// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
