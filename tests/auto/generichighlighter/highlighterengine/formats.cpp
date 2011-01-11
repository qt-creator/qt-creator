/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "formats.h"

#include <QtCore/Qt>

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
