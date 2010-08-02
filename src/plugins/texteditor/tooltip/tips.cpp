/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "tips.h"
#include "tipcontents.h"

#include <QtCore/QRect>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

using namespace TextEditor;
using namespace Internal;

namespace {
    // @todo: Reuse...
    QPixmap tilePixMap(int size)
    {
        const int checkerbordSize= size;
        QPixmap tilePixmap(checkerbordSize * 2, checkerbordSize * 2);
        tilePixmap.fill(Qt::white);
        QPainter tilePainter(&tilePixmap);
        QColor color(220, 220, 220);
        tilePainter.fillRect(0, 0, checkerbordSize, checkerbordSize, color);
        tilePainter.fillRect(checkerbordSize, checkerbordSize, checkerbordSize, checkerbordSize, color);
        return tilePixmap;
    }
}

Tip::Tip(QWidget *parent) : QFrame(parent)
{
    setWindowFlags(Qt::ToolTip);
    setAutoFillBackground(true);
    ensurePolished();
}

Tip::~Tip()
{}

void Tip::setContent(const QSharedPointer<TipContent> &content)
{
    m_content = content;
    configure();
}

const QSharedPointer<TipContent> &Tip::content() const
{ return m_content; }

ColorTip::ColorTip(QWidget *parent) : Tip(parent)
{
    setFrameStyle(QFrame::Box);
    resize(QSize(40, 40));

    m_tilePixMap = tilePixMap(9);
}

ColorTip::~ColorTip()
{}

void ColorTip::configure()
{
    update();
}

void ColorTip::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QRect r(1, 1, width() - 2, height() - 2);
    painter.drawTiledPixmap(r, m_tilePixMap);
    painter.setBrush(static_cast<QColorContent *>(content().data())->color());
    painter.drawRect(rect());

    QFrame::paintEvent(event);
}
