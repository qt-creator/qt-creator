/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#pragma once

#include <QChar>
#include <QString>

namespace Nim {

class SourceCodeStream
{
public:
    SourceCodeStream(const QChar *text, int length)
        : m_text(text)
        , m_textLength(length)
        , m_position(0)
        , m_markedPosition(0)
    {}

    inline void setAnchor()
    {
        m_markedPosition = m_position;
    }

    inline void move(int amount = 1)
    {
        m_position += amount;
    }

    inline void moveToEnd()
    {
        m_position = m_textLength;
    }

    inline int pos()
    {
        return m_position;
    }

    inline int length() const
    {
        return m_position - m_markedPosition;
    }

    inline int anchor() const
    {
        return m_markedPosition;
    }

    inline bool isEnd() const
    {
        return m_position >= m_textLength;
    }

    inline QChar peek(int offset = 0) const
    {
        int pos = m_position + offset;
        if (pos >= m_textLength)
            return QLatin1Char('\0');
        return m_text[pos];
    }

    inline QString value() const
    {
        const QChar *start = m_text + m_markedPosition;
        return QString(start, length());
    }

    inline QString value(int begin, int length) const
    {
        return QString(m_text + begin, length);
    }

private:
    const QChar *m_text;
    const int m_textLength;
    int m_position;
    int m_markedPosition;
};

}
