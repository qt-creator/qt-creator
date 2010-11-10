
#include "glsllexer.h"
#include "glslparser.h"
#include <cctype>
#include <iostream>

using namespace GLSL;

Lexer::Lexer(const char *source, unsigned size)
    : _source(source),
      _it(source),
      _end(source + size),
      _size(size),
      _yychar('\n'),
      _lineno(0),
      _variant(Variant_Mask & ~Variant_Reserved) // everything except reserved
{
}

Lexer::~Lexer()
{
}

void Lexer::yyinp()
{
    if (_it != _end) {
        _yychar = *_it++;
        if (_yychar == '\n')
            ++_lineno;
    } else
        _yychar = 0;
}

int Lexer::yylex(Token *tk)
{
    const char *pos = 0;
    int line = 0;
    const int kind = yylex_helper(&pos, &line);
    tk->kind = kind;
    tk->position = pos - _source;
    tk->length = _it - pos - 1;
    tk->line = line;
    tk->matchingBrace = 0;
    return kind;
}

int Lexer::yylex_helper(const char **position, int *line)
{
again:
    while (std::isspace(_yychar))
        yyinp();

    *position = _it - 1;
    *line = _lineno;

    if (_yychar == 0)
        return Parser::EOF_SYMBOL;

    const int ch = _yychar;
    yyinp();

    switch (ch) {
    case '#':
        for (; _yychar; yyinp()) {
            if (_yychar == '\n')
                break;
        }
        goto again;

    // one of `!', `!='
    case '!':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_NE_OP;
        }
        return Parser::T_BANG;

        // one of
        //  %
        //  %=
    case '%':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_MOD_ASSIGN;
        }
        return Parser::T_PERCENT;

        // one of
        // &
        // &&
        // &=
    case '&':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_AND_ASSIGN;
        } else if (_yychar == '&') {
            yyinp();
            return Parser::T_AND_OP;
        }
        return Parser::T_AMPERSAND;

        // (
    case '(':
        return Parser::T_LEFT_PAREN;

        // )
    case ')':
        return Parser::T_RIGHT_PAREN;

        // one of
        // *
        // *=
    case '*':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_MUL_ASSIGN;
        }
        return Parser::T_STAR;

        // one of
        // ++
        // +=
        // +
    case '+':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_ADD_ASSIGN;
        } else if (_yychar == '+') {
            yyinp();
            return Parser::T_INC_OP;
        }
        return Parser::T_PLUS;

        // ,
    case ',':
        return Parser::T_COMMA;

        // one of
        // -
        // --
        // -=
    case '-':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_SUB_ASSIGN;
        } else if (_yychar == '-') {
            yyinp();
            return Parser::T_DEC_OP;
        }
        return Parser::T_DASH;

        // one of
        // .
        // float constant
    case '.':
        if (std::isdigit(_yychar)) {
            while (std::isalnum(_yychar)) {
                yyinp();
            }
            return Parser::T_NUMBER;
        }
        return Parser::T_DOT;

        // one of
        // /
        // /=
        // comment
    case '/':
        if (_yychar == '/') {
            for (; _yychar; yyinp()) {
                if (_yychar == '\n')
                    break;
            }
            goto again;
        } else if (_yychar == '*') {
            yyinp();
            while (_yychar) {
                if (_yychar == '*') {
                    yyinp();
                    if (_yychar == '/') {
                        yyinp();
                        goto again;
                    }
                } else {
                    yyinp();
                }
            }
            goto again;
        } else if (_yychar == '=') {
            yyinp();
            return Parser::T_DIV_ASSIGN;
        }
        return Parser::T_SLASH;

        // :
    case ':':
        return Parser::T_COLON;

        // ;
    case ';':
        return Parser::T_SEMICOLON;

        // one of
        // <
        // <=
        // <<
        // <<=
    case '<':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_LE_OP;
        } else if (_yychar == '<') {
            yyinp();
            if (_yychar == '=') {
                yyinp();
                return Parser::T_LEFT_ASSIGN;
            }
            return Parser::T_LEFT_OP;
        }
        return Parser::T_LEFT_ANGLE;

        // one of
        // =
        // ==
    case '=':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_EQ_OP;
        }
        return Parser::T_EQUAL;

        // one of
        // >
        // >=
        // >>=
        // >>
    case '>':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_GE_OP;
        } else if (_yychar == '>') {
            yyinp();
            if (_yychar == '=') {
                yyinp();
                return Parser::T_RIGHT_ASSIGN;
            }
            return Parser::T_RIGHT_OP;
        }
        return Parser::T_RIGHT_ANGLE;

        // ?
    case '?':
        return Parser::T_QUESTION;

        // [
    case '[':
        return Parser::T_LEFT_BRACKET;

        // ]
    case ']':
        return Parser::T_RIGHT_BRACKET;

        // one of
        // ^
        // ^=
    case '^':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_XOR_ASSIGN;
        }
        return Parser::T_XOR_OP;

        // {
    case '{':
        return Parser::T_LEFT_BRACE;

        // one of
        // |
        // |=
        // ||
    case '|':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_OR_ASSIGN;
        } else if (_yychar == '|') {
            yyinp();
            return Parser::T_OR_OP;
        }
        return Parser::T_VERTICAL_BAR;

        // }
    case '}':
        return Parser::T_RIGHT_BRACE;

        // ~
    case '~':
        return Parser::T_TILDE;

    default:
        if (std::isalpha(ch) || ch == '_') {
            const char *word = _it - 2;
            while (std::isalnum(_yychar) || _yychar == '_') {
                yyinp();
            }
            int t = classify(word, _it - word -1);
            if (!(t & Variant_Mask))
                return t;
            if ((_variant & t & Variant_Mask) == 0) {
                // TODO: issue a proper error for the unsupported keyword
                std::string keyword(word, _it - word -1);
                fprintf(stderr, "unsupported keyword `%s' at line %d\n",
                        keyword.c_str(), _lineno);
            }
            return t & ~Variant_Mask;
        } else if (std::isdigit(ch)) {
            while (std::isalnum(_yychar) || _yychar == '.') {
                yyinp();
            }
            return Parser::T_NUMBER;
        }

    } // switch

    return Parser::T_ERROR;
}
