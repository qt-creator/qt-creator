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

#ifndef QMT_STYLE_H
#define QMT_STYLE_H

#include "qmt/infrastructure/qmt_global.h"

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

public:

    explicit Style(Type type);

    virtual ~Style();

public:

    Type getType() const { return _type; }

    QPen getLinePen() const { return _line_pen; }

    void setLinePen(const QPen &pen);

    QPen getOuterLinePen() const { return _outer_line_pen; }

    void setOuterLinePen(const QPen &pen);

    QPen getInnerLinePen() const { return _inner_line_pen; }

    void setInnerLinePen(const QPen &pen);

    QPen getExtraLinePen() const { return _extra_line_pen; }

    void setExtraLinePen(const QPen &pen);

    QBrush getTextBrush() const { return _text_brush; }

    void setTextBrush(const QBrush &brush);

    QBrush getFillBrush() const { return _fill_brush; }

    void setFillBrush(const QBrush &brush);

    QBrush getExtraFillBrush() const { return _extra_fill_brush; }

    void setExtraFillBrush(const QBrush &brush);

    QFont getNormalFont() const { return _normal_font; }

    void setNormalFont(const QFont &font);

    QFont getSmallFont() const { return _small_font; }

    void setSmallFont(const QFont &font);

    QFont getHeaderFont() const { return _header_font; }

    void setHeaderFont(const QFont &font);

private:

    Type _type;

    QPen _line_pen;

    QPen _outer_line_pen;

    QPen _inner_line_pen;

    QPen _extra_line_pen;

    QBrush _text_brush;

    QBrush _fill_brush;

    QBrush _extra_fill_brush;

    QFont _normal_font;

    QFont _small_font;

    QFont _header_font;

};

}

#endif // QMT_STYLE_H
