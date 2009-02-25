/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "tabpositionindicator.h"

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QBrush>
#include <QtGui/QPalette>

using namespace Core::Internal;

TabPositionIndicator::TabPositionIndicator()
    : QWidget(0, Qt::ToolTip)
{
}

void TabPositionIndicator::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    QPen pen = p.pen();
    pen.setWidth(2);
    pen.setColor(palette().color(QPalette::Active, QPalette::LinkVisited));
    pen.setStyle(Qt::DotLine);
    p.setPen(pen);
    p.drawLine(event->rect().topLeft(), event->rect().bottomLeft());
}
