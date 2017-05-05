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

#pragma once

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

    Format_FormatsAmount
};

class FormatToken
{
public:
    FormatToken() {}

    FormatToken(Format format, int position, int length)
        : m_format(format), m_position(position), m_length(length)
    {}

    bool isEndOfBlock() { return m_position == -1; }

    Format format() const { return m_format; }
    int begin() const { return m_position; }
    int end() const { return m_position + m_length; }
    int length() const { return m_length; }

private:
    Format m_format = Format_FormatsAmount;
    int m_position = -1;
    int m_length = -1;
};

} // namespace Internal
} // namespace PythonEditor
