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

    explicit Style(Type type);
    virtual ~Style();

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

#endif // QMT_STYLE_H
