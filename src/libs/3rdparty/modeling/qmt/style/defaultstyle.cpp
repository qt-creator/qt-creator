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

#include "defaultstyle.h"

namespace qmt {

DefaultStyle::DefaultStyle()
    : Style(GlobalStyle)
{
    QPen line_pen;
    line_pen.setColor("black");
    line_pen.setWidth(1);
    setLinePen(line_pen);
    setOuterLinePen(line_pen);
    setInnerLinePen(line_pen);
    setExtraLinePen(line_pen);
    setTextBrush(QBrush(QColor("black")));
    setFillBrush(QBrush(QColor("yellow")));
    setExtraFillBrush(QBrush(QColor("white")));
    QFont normal_font;
    // TODO the standard font family is "MS Shell Dlg 2" which is not good for small fonts in diagrams
    //normal_font.setFamily(QStringLiteral("ModelEditor"));
    //normal_font.setPointSizeF(9.0);
    normal_font.setPixelSize(11);
    setNormalFont(normal_font);
    QFont small_font(normal_font);
    //small_font.setPointSizeF(normal_font.pointSizeF() * 0.80);
    small_font.setPixelSize(9);
    setSmallFont(small_font);
    QFont header_font(normal_font);
    //header_font.setPointSizeF(normal_font.pointSizeF() * 1.4);
    header_font.setPixelSize(16);
    setHeaderFont(header_font);
}

DefaultStyle::~DefaultStyle()
{
}

}
