/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "formats.h"

#include <Qt>

Formats::Formats()
{
    m_keywordFormat.setForeground(Qt::darkGray);
    m_dataTypeFormat.setForeground(Qt::gray);
    m_decimalFormat.setForeground(Qt::lightGray);
    m_baseNFormat.setForeground(Qt::red);
    m_floatFormat.setForeground(Qt::green);
    m_charFormat.setForeground(Qt::blue);
    m_stringFormat.setForeground(Qt::cyan);
    m_commentFormat.setForeground(Qt::magenta);
    m_alertFormat.setForeground(Qt::yellow);
    m_errorFormat.setForeground(Qt::darkRed);
    m_functionFormat.setForeground(Qt::darkGreen);
    m_regionMarkerFormat.setForeground(Qt::darkBlue);
    m_othersFormat.setForeground(Qt::darkCyan);
}

Formats &Formats::instance()
{
    static Formats formats;
    return formats;
}

QString Formats::name(const QTextCharFormat &format) const
{
    if (format == QTextCharFormat())
        return "Default format";
    else if (format == m_keywordFormat)
        return "Keyword";
    else if (format == m_dataTypeFormat)
        return "Data type format";
    else if (format == m_decimalFormat)
        return "Decimal format";
    else if (format == m_baseNFormat)
        return "Base N format";
    else if (format == m_floatFormat)
        return "Float format";
    else if (format == m_charFormat)
        return "Char format";
    else if (format == m_stringFormat)
        return "String format";
    else if (format == m_commentFormat)
        return "Comment format";
    else if (format == m_alertFormat)
        return "Alert format";
    else if (format == m_errorFormat)
        return "Error format";
    else if (format == m_functionFormat)
        return "Function format";
    else if (format == m_regionMarkerFormat)
        return "Region Marker format";
    else if (format == m_othersFormat)
        return "Others format";
    else
        return "Unidentified format";
}
