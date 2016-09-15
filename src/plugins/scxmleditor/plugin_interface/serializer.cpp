/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "serializer.h"

using namespace ScxmlEditor::PluginInterface;

Serializer::Serializer()
    : m_separator(";")
{
}

void Serializer::seek(int pos)
{
    m_index = qBound(0, pos, m_data.count() - 1);
}

void Serializer::clear()
{
    m_data.clear();
    m_index = 0;
}

void Serializer::append(double d)
{
    m_data.append(QString::fromLatin1("%1").arg(d, 0, 'f', 2).remove(".00"));
    m_index = m_data.count() - 1;
}

QString Serializer::data() const
{
    return m_data.join(m_separator);
}

void Serializer::append(const QPolygonF &d)
{
    appendPolygon(d);
}
void Serializer::append(const QPolygon &d)
{
    appendPolygon(d);
}
void Serializer::append(const QRectF &d)
{
    appendRect(d);
}
void Serializer::append(const QRect &d)
{
    appendRect(d);
}
void Serializer::append(const QPointF &d)
{
    appendPoint(d);
}
void Serializer::append(const QPoint &d)
{
    appendPoint(d);
}

void Serializer::read(QPolygonF &d)
{
    readPolygon<QPointF>(d);
}
void Serializer::read(QPolygon &d)
{
    readPolygon<QPoint>(d);
}
void Serializer::read(QRectF &d)
{
    readRect(d);
}
void Serializer::read(QRect &d)
{
    readRect(d);
}
void Serializer::read(QPointF &d)
{
    readPoint(d);
}
void Serializer::read(QPoint &d)
{
    readPoint(d);
}

void Serializer::setData(const QString &d)
{
    m_data = d.split(m_separator, QString::SkipEmptyParts);
    m_index = 0;
}

double Serializer::readNext()
{
    double data = 0.0;
    if (m_index >= 0 && m_index < m_data.count())
        data = m_data[m_index].toDouble();

    m_index++;
    return data;
}
