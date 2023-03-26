// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "defaultstyle.h"

namespace qmt {

DefaultStyle::DefaultStyle()
    : Style(GlobalStyle)
{
    QPen linePen;
    linePen.setColor("black");
    linePen.setWidth(1);
    setLinePen(linePen);
    setOuterLinePen(linePen);
    setInnerLinePen(linePen);
    setExtraLinePen(linePen);
    setTextBrush(QBrush(QColor("black")));
    setFillBrush(QBrush(QColor("yellow")));
    setExtraFillBrush(QBrush(QColor("white")));
    QFont normalFont;
    // TODO use own ModelEditor font
    normalFont.setPixelSize(11);
    setNormalFont(normalFont);
    QFont smallFont(normalFont);
    smallFont.setPixelSize(9);
    setSmallFont(smallFont);
    QFont headerFont(normalFont);
    headerFont.setPixelSize(16);
    setHeaderFont(headerFont);
}

DefaultStyle::~DefaultStyle()
{
}

} // namespace qmt
