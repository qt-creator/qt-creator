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

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "sexprlexer.h"

namespace Nim {

struct SExprParser {
    enum NodeType : uint8_t {
        ATOM_STRING = 1,
        ATOM_NUMBER = 2,
        ATOM_IDENTIFIER = 4,
        LIST = 8
    };

    struct Node {
        NodeType kind;
        std::size_t from;
        std::size_t end;
        std::vector<Node> nodes;
        std::string value;
    };

    SExprParser(const std::string &str)
        : SExprParser(str.data(), str.length())
    {}

    SExprParser(const char *data)
        : SExprParser(data, ::strlen(data))
    {}

    SExprParser(const char *data, std::size_t length)
        : m_lexer(SExprLexer(data, length))
    {}

    bool parse(Node &node)
    {
        SExprLexer::Token token;
        if (m_lexer.next(token) != SExprLexer::Result::TokenAvailable
                || token.type != SExprLexer::OPEN_BRACE)
            return false;
        node = Node {LIST, token.start, token.start, {}, {}};
        return parseList(node);
    }

private:
    bool parseList(Node &node)
    {
        while (true) {
            SExprLexer::Token token;
            if (m_lexer.next(token) != SExprLexer::Result::TokenAvailable)
                return false;

            switch (token.type) {
            case SExprLexer::STRING: {
                std::string value = m_lexer.tokenValue(token);
                value.pop_back();
                value.erase(value.begin());
                node.nodes.emplace_back(Node{ATOM_STRING, token.start, token.end, {}, std::move(value)});
                break;
            }
            case SExprLexer::NUMBER: {
                node.nodes.emplace_back(Node{ATOM_NUMBER, token.start, token.end, {}, m_lexer.tokenValue(token)});
                break;
            }
            case SExprLexer::IDENTIFIER: {
                node.nodes.emplace_back(Node{ATOM_IDENTIFIER, token.start, token.end, {}, m_lexer.tokenValue(token)});
                break;
            }
            case SExprLexer::OPEN_BRACE: {
                Node list {LIST, token.start, token.start, {}, {}};
                if (!parseList(list)) {
                    return false;
                }
                node.nodes.emplace_back(std::move(list));
                break;
            }
            case SExprLexer::CLOSE_BRACE: {
                node.end = token.end;
                return true;
            }
            }
        }
    }

    SExprLexer m_lexer;
};

} // namespace Nim
