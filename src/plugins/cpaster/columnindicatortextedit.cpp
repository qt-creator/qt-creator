// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "columnindicatortextedit.h"

#include <QPainter>
#include <QScrollBar>

namespace CodePaster {

ColumnIndicatorTextEdit::ColumnIndicatorTextEdit()
{
    QFont font;
    font.setFamily(QLatin1String("Courier New"));
    setFont(font);
    setReadOnly(true);
    setUndoRedoEnabled(false);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setVerticalStretch(3);
    setSizePolicy(sizePolicy);
    m_columnIndicator = QFontMetrics(font).horizontalAdvance(QLatin1Char('W')) * 100
            + contentsMargins().left() + 1;
    m_columnIndicatorFont.setFamily(QLatin1String("Times"));
    m_columnIndicatorFont.setPointSizeF(7.0);
}

void ColumnIndicatorTextEdit::paintEvent(QPaintEvent *event)
{
    QTextEdit::paintEvent(event);

    QPainter p(viewport());
    p.setFont(m_columnIndicatorFont);
    p.setPen(QPen(QColor(0xa0, 0xa0, 0xa0, 0xa0)));
    p.drawLine(m_columnIndicator, 0, m_columnIndicator, viewport()->height());
    int yOffset = verticalScrollBar()->value();
    p.drawText(m_columnIndicator + 1, m_columnIndicatorFont.pointSize() - yOffset, QLatin1String("100"));
}

} // namespace CodePaster
