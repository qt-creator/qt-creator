// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Python::Internal {

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
    Format_LParen,
    Format_RParen,

    Format_FormatsAmount
};

class FormatToken
{
public:
    FormatToken() = default;

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

} // Python::Internal
