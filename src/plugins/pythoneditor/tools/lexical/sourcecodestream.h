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

#ifndef PYTHONEDITOR_SOURCECODESTREAM_H
#define PYTHONEDITOR_SOURCECODESTREAM_H

#include <QString>

namespace PythonEditor {
namespace Internal {

class SourceCodeStream
{
public:
    SourceCodeStream(const QChar *text, int length)
        :m_text(text)
        ,m_textLength(length)
        ,m_position(0)
        ,m_markedPosition(0)
    {}

    inline void setAnchor()
    {
        m_markedPosition = m_position;
    }

    inline void move()
    {
        ++m_position;
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

} // namespace Internal
} // namespace PythonEditor

#endif // PYTHONEDITOR_SOURCECODESTREAM_H
