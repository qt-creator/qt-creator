// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"
#include "qmt/infrastructure/uid.h"

#include <QString>
#include <QPen>
#include <QBrush>
#include <QFont>

namespace qmt {

class QMT_EXPORT Style
{
public:
    enum Type {
        GlobalStyle,
        ModelSpecificStyle
    };

    explicit Style(Type type);
    virtual ~Style();

    Uid uid() const { return m_uid; }
    Type type() const { return m_type; }
    QPen linePen() const { return m_linePen; }
    void setLinePen(const QPen &pen);
    QPen outerLinePen() const { return m_outerLinePen; }
    void setOuterLinePen(const QPen &pen);
    QPen innerLinePen() const { return m_innerLinePen; }
    void setInnerLinePen(const QPen &pen);
    QPen extraLinePen() const { return m_extraLinePen; }
    void setExtraLinePen(const QPen &pen);
    QBrush textBrush() const { return m_textBrush; }
    void setTextBrush(const QBrush &brush);
    QBrush fillBrush() const { return m_fillBrush; }
    void setFillBrush(const QBrush &brush);
    QBrush extraFillBrush() const { return m_extraFillBrush; }
    void setExtraFillBrush(const QBrush &brush);
    QFont normalFont() const { return m_normalFont; }
    void setNormalFont(const QFont &font);
    QFont smallFont() const { return m_smallFont; }
    void setSmallFont(const QFont &font);
    QFont headerFont() const { return m_headerFont; }
    void setHeaderFont(const QFont &font);

private:
    Uid m_uid;
    Type m_type;
    QPen m_linePen;
    QPen m_outerLinePen;
    QPen m_innerLinePen;
    QPen m_extraLinePen;
    QBrush m_textBrush;
    QBrush m_fillBrush;
    QBrush m_extraFillBrush;
    QFont m_normalFont;
    QFont m_smallFont;
    QFont m_headerFont;
};

} // namespace qmt
