/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "columnindicatortextedit.h"

#include <QPainter>
#include <QScrollBar>

namespace CodePaster {

ColumnIndicatorTextEdit::ColumnIndicatorTextEdit(QWidget *parent) :
        QTextEdit(parent), m_columnIndicator(0)
{
    QFont font;
    font.setFamily(QLatin1String("Courier New"));
    setFont(font);
    setReadOnly(true);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setVerticalStretch(3);
    setSizePolicy(sizePolicy);
    int cmx = 0, cmy = 0, cmw = 0, cmh = 0;
    getContentsMargins(&cmx, &cmy, &cmw, &cmh);
    m_columnIndicator = QFontMetrics(font).width(QLatin1Char('W')) * 100 + cmx + 1;
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
