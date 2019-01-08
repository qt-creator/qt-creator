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

#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace Nim {

struct SExprLexer {
    enum Result {
        Finished,
        TokenAvailable,
        Error
    };

    enum TokenType : std::size_t {
        STRING,
        NUMBER,
        IDENTIFIER,
        OPEN_BRACE,
        CLOSE_BRACE,
    };

    struct Token {
        TokenType type;
        std::size_t start;
        std::size_t end;
    };

    SExprLexer(const char *data, std::size_t length)
        : m_data(data)
        , m_dataLength(length)
        , m_pos(0)
    {}

    SExprLexer(const char *data)
        : SExprLexer(data, ::strlen(data))
    {}

    SExprLexer(const std::string &data)
        : SExprLexer(data.c_str(), data.length())
    {}

    const char *data() const
    {
        return m_data;
    }

    std::size_t dataLength() const
    {
        return m_dataLength;
    }

    static size_t tokenLength(Token &token)
    {
        return token.end - token.start + 1;
    }

    std::string tokenValue(Token &token) const
    {
        return std::string(m_data + token.start, tokenLength(token));
    }

    Result next(Token &token)
    {
        while (m_pos < m_dataLength) {
            if (m_data[m_pos] == '(') {
                token = Token{OPEN_BRACE, m_pos, m_pos + 1};
                m_pos++;
                return TokenAvailable;
            } else if (m_data[m_pos] == ')') {
                token = Token{CLOSE_BRACE, m_pos, m_pos + 1};
                m_pos++;
                return TokenAvailable;
            } else if (m_data[m_pos] == '"') {
                Token token_tmp {STRING, m_pos, m_pos};
                char previous = '"';
                m_pos++;
                while (true) {
                    if (m_pos >= m_dataLength)
                        return Error;
                    if (m_data[m_pos] == '"' && previous != '\\')
                        break;
                    previous = m_data[m_pos];
                    m_pos++;
                }
                token_tmp.end = m_pos;
                token = std::move(token_tmp);
                m_pos++;
                return TokenAvailable;
            } else if (std::isdigit(m_data[m_pos])) {
                bool decimal_separator_found = false;
                token = {NUMBER, m_pos, m_pos};
                m_pos++;
                while (true) {
                    if (m_pos >= m_dataLength)
                        break;
                    if (m_data[m_pos] == ',' || m_data[m_pos] == '.') {
                        if (decimal_separator_found)
                            return Error;
                        decimal_separator_found = true;
                    } else if (!std::isdigit(m_data[m_pos]))
                        break;
                    m_pos++;
                }
                token.end = m_pos - 1;
                return TokenAvailable;
            } else if (!std::isspace(m_data[m_pos])) {
                token = {IDENTIFIER, m_pos, m_pos};
                m_pos++;
                while (true) {
                    if (m_pos >= m_dataLength)
                        break;
                    if (std::isspace(m_data[m_pos]) || m_data[m_pos] == '(' || m_data[m_pos] == ')')
                        break;
                    m_pos++;
                }
                token.end = m_pos - 1;
                return TokenAvailable;
            } else {
                m_pos++;
            }
        }
        return Finished;
    }

    static std::vector<Token> parse(const std::string &data)
    {
        return parse(data.data(), data.size());
    }

    static std::vector<Token> parse(const char *data)
    {
        return parse(data, ::strlen(data));
    }

    static std::vector<Token> parse(const char *data, std::size_t data_size)
    {
        std::vector<Token> result;
        SExprLexer lexer(data, data_size);
        Token token {};
        while (lexer.next(token)) {
            result.push_back(token);
            token = {};
        }
        return result;
    }

private:
    const char *m_data = nullptr;
    std::size_t m_dataLength = 0;
    std::size_t m_pos = 0;
};

} // namespace Nim
