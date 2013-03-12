/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PYTHONFORMATTOKEN_H
#define PYTHONFORMATTOKEN_H

#include <stdlib.h>

namespace PythonEditor {
namespace Internal {

enum Format {
    Format_Number = 0,
    Format_String,
    Format_Keyword,
    Format_Type,
    Format_ClassField,
    Format_MagicAttr, // magic class attribute/method, like __name__, __init__
    Format_Operator,
    Format_Comment,
    Format_Doxygen,
    Format_Identifier,
    Format_Whitespace,
    Format_ImportedModule,

    Format_FormatsAmount,
    Format_EndOfBlock
};

class FormatToken
{
public:
    FormatToken() {}

    FormatToken(Format format, size_t position, size_t length)
        :m_format(format)
        ,m_position(position)
        ,m_length(length)
    {}

    inline Format format() const { return m_format; }
    inline int begin() const { return m_position; }
    inline int end() const { return m_position + m_length; }
    inline int length() const { return m_length; }

private:
    Format m_format;
    int m_position;
    int m_length;
};

} // namespace Internal
} // namespace PythonEditor

#endif // PYTHONFORMATTOKEN_H
