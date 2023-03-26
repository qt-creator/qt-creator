// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "relationstarterstyle.h"

namespace qmt {

RelationStarterStyle::RelationStarterStyle()
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
    setFillBrush(QBrush(QColor("black")));
    setExtraFillBrush(QBrush(QColor("white")));
    QFont normalFont;
    setNormalFont(normalFont);
    QFont smallFont;
    smallFont.setPointSizeF(QFont().pointSizeF() * 0.80);
    setSmallFont(smallFont);
    QFont headerFont;
    headerFont.setPointSizeF(QFont().pointSizeF() * 1.25);
    setHeaderFont(headerFont);
}

RelationStarterStyle::~RelationStarterStyle()
{
}

} // namespace qmt
