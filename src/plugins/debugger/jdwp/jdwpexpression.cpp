// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jdwpexpression.h"

#include "../debuggertr.h"

using namespace Utils;

namespace Debugger::Internal {

QString JdwpExpression::typeName() const
{
    switch (kind) {
    case Kind::Int: return "int";
    case Kind::Long: return "long";
    case Kind::Float: return "float";
    case Kind::Double: return "double";
    case Kind::Boolean: return "boolean";
    case Kind::Char: return "char";
    case Kind::String: return "java.lang.String";
    case Kind::Null: return "null";
    case Kind::Name:
    case Kind::This:
    case Kind::Field:
    case Kind::Index:
    case Kind::Unary:
    case Kind::Binary:
        break;
    }
    return {};
}

// A hand-written scanner and a recursive descent over it, which is all the
// grammar below needs. Java's own is far larger; what is missing from here
// says so when it is met, rather than guessing at a meaning.
class Parser
{
public:
    explicit Parser(const QString &text) : m_text(text) {}

    Result<JdwpExpression> run()
    {
        skipSpace();
        if (atEnd())
            return ResultError(Tr::tr("The expression is empty."));
        const Result<JdwpExpression> expression = parseBinary(0);
        if (!expression)
            return expression;
        skipSpace();
        if (!atEnd())
            return unexpected();
        return expression;
    }

private:
    bool atEnd() const { return m_pos >= m_text.size(); }
    QChar current() const { return atEnd() ? QChar() : m_text.at(m_pos); }
    QChar at(int offset) const
    {
        const int pos = m_pos + offset;
        return pos < m_text.size() ? m_text.at(pos) : QChar();
    }

    void skipSpace()
    {
        while (!atEnd() && current().isSpace())
            ++m_pos;
    }

    ResultError unexpected() const
    {
        return ResultError(Tr::tr("Cannot read \"%1\" as a Java expression, from \"%2\" on.")
                               .arg(m_text, m_text.mid(m_pos)));
    }

    // The binary operators by how tightly they bind, loosest first, which is
    // the order Java gives them.
    static const QList<QList<QPair<QString, JdwpExpression::Operator>>> &levels()
    {
        using Op = JdwpExpression::Operator;
        static const QList<QList<QPair<QString, Op>>> table = {
            {{"||", Op::Or}},
            {{"&&", Op::And}},
            {{"==", Op::Equal}, {"!=", Op::NotEqual}},
            {{"<=", Op::LessEqual}, {">=", Op::GreaterEqual}, {"<", Op::Less}, {">", Op::Greater}},
            {{"+", Op::Add}, {"-", Op::Subtract}},
            {{"*", Op::Multiply}, {"/", Op::Divide}, {"%", Op::Remainder}},
        };
        return table;
    }

    Result<JdwpExpression> parseBinary(int level)
    {
        if (level == levels().size())
            return parseUnary();
        Result<JdwpExpression> left = parseBinary(level + 1);
        if (!left)
            return left;
        while (true) {
            skipSpace();
            JdwpExpression::Operator op = JdwpExpression::Operator::None;
            for (const auto &[text, candidate] : levels().at(level)) {
                if (!m_text.mid(m_pos).startsWith(text))
                    continue;
                // A "<" is not the start of a "<=", and an "=" alone is not an
                // operator this reads at all.
                op = candidate;
                m_pos += text.size();
                break;
            }
            if (op == JdwpExpression::Operator::None)
                return left;
            const Result<JdwpExpression> right = parseBinary(level + 1);
            if (!right)
                return right;
            JdwpExpression binary;
            binary.kind = JdwpExpression::Kind::Binary;
            binary.op = op;
            binary.base = std::make_shared<JdwpExpression>(*left);
            binary.right = std::make_shared<JdwpExpression>(*right);
            left = binary;
        }
    }

    Result<JdwpExpression> parseUnary()
    {
        skipSpace();
        JdwpExpression::Operator op = JdwpExpression::Operator::None;
        if (current() == '!' && at(1) != '=') {
            op = JdwpExpression::Operator::Not;
        } else if (current() == '-' && !at(1).isDigit() && at(1) != '.') {
            // A minus in front of a number belongs to the number.
            op = JdwpExpression::Operator::Negate;
        }
        if (op == JdwpExpression::Operator::None)
            return parsePostfix();
        ++m_pos;
        const Result<JdwpExpression> operand = parseUnary();
        if (!operand)
            return operand;
        JdwpExpression unary;
        unary.kind = JdwpExpression::Kind::Unary;
        unary.op = op;
        unary.base = std::make_shared<JdwpExpression>(*operand);
        return unary;
    }

    Result<JdwpExpression> parsePostfix()
    {
        Result<JdwpExpression> value = parsePrimary();
        if (!value)
            return value;
        while (true) {
            skipSpace();
            if (current() == '.') {
                ++m_pos;
                skipSpace();
                const QString name = takeIdentifier();
                if (name.isEmpty())
                    return unexpected();
                JdwpExpression field;
                field.kind = JdwpExpression::Kind::Field;
                field.text = name;
                field.base = std::make_shared<JdwpExpression>(*value);
                value = field;
                continue;
            }
            if (current() == '[') {
                ++m_pos;
                const Result<JdwpExpression> inner = parseBinary(0);
                if (!inner)
                    return inner;
                skipSpace();
                if (current() != ']')
                    return ResultError(Tr::tr("The index in \"%1\" is not closed.").arg(m_text));
                ++m_pos;
                JdwpExpression element;
                element.kind = JdwpExpression::Kind::Index;
                element.base = std::make_shared<JdwpExpression>(*value);
                element.index = std::make_shared<JdwpExpression>(*inner);
                value = element;
                continue;
            }
            return value;
        }
    }

    Result<JdwpExpression> parsePrimary()
    {
        skipSpace();
        if (atEnd())
            return unexpected();
        const QChar c = current();
        if (c == '(') {
            ++m_pos;
            const Result<JdwpExpression> inner = parseBinary(0);
            if (!inner)
                return inner;
            skipSpace();
            if (current() != ')')
                return ResultError(Tr::tr("The parenthesis in \"%1\" is not closed.").arg(m_text));
            ++m_pos;
            return inner;
        }
        if (c == '"')
            return parseString();
        if (c == '\'')
            return parseChar();
        if (c == '-' || c == '+' || c.isDigit())
            return parseNumber();

        const QString name = takeIdentifier();
        if (name.isEmpty())
            return unexpected();
        JdwpExpression expression;
        if (name == "true" || name == "false") {
            expression.kind = JdwpExpression::Kind::Boolean;
            expression.integer = name == "true" ? 1 : 0;
        } else if (name == "null") {
            expression.kind = JdwpExpression::Kind::Null;
        } else if (name == "this") {
            expression.kind = JdwpExpression::Kind::This;
        } else {
            expression.kind = JdwpExpression::Kind::Name;
            expression.text = name;
        }
        return expression;
    }

    QString takeIdentifier()
    {
        const int start = m_pos;
        if (!atEnd() && (current().isLetter() || current() == '_' || current() == '$'))
            ++m_pos;
        else
            return {};
        while (!atEnd() && (current().isLetterOrNumber() || current() == '_' || current() == '$'))
            ++m_pos;
        return m_text.mid(start, m_pos - start);
    }

    Result<JdwpExpression> parseNumber()
    {
        const int start = m_pos;
        if (current() == '-' || current() == '+')
            ++m_pos;
        int base = 10;
        if (current() == '0' && (at(1) == 'x' || at(1) == 'X')) {
            base = 16;
            m_pos += 2;
        } else if (current() == '0' && (at(1) == 'b' || at(1) == 'B')) {
            base = 2;
            m_pos += 2;
        }
        QString digits;
        bool isReal = false;
        while (!atEnd()) {
            const QChar c = current();
            if (c == '_') {
                ++m_pos;
                continue;
            }
            if (c.isDigit() || (base == 16 && isHexLetter(c))) {
                digits += c;
                ++m_pos;
                continue;
            }
            if (base == 10 && (c == '.' || c == 'e' || c == 'E')) {
                isReal = true;
                digits += c;
                ++m_pos;
                // The sign of an exponent belongs to the number, not to what follows it.
                if ((c == 'e' || c == 'E') && (current() == '-' || current() == '+')) {
                    digits += current();
                    ++m_pos;
                }
                continue;
            }
            break;
        }
        if (digits.isEmpty())
            return unexpected();

        JdwpExpression expression;
        const QChar suffix = current();
        const QString sign = m_text.at(start) == '-' ? QString("-") : QString();
        if (suffix == 'f' || suffix == 'F') {
            ++m_pos;
            expression.kind = JdwpExpression::Kind::Float;
        } else if (suffix == 'd' || suffix == 'D') {
            ++m_pos;
            expression.kind = JdwpExpression::Kind::Double;
        } else if (suffix == 'l' || suffix == 'L') {
            ++m_pos;
            expression.kind = JdwpExpression::Kind::Long;
        } else {
            expression.kind = isReal ? JdwpExpression::Kind::Double : JdwpExpression::Kind::Int;
        }

        bool ok = false;
        const QString literal = sign + digits;
        if (expression.kind == JdwpExpression::Kind::Float
            || expression.kind == JdwpExpression::Kind::Double) {
            expression.number = literal.toDouble(&ok);
        } else {
            expression.integer = literal.toLongLong(&ok, base);
            if (ok && expression.kind == JdwpExpression::Kind::Int) {
                // As Java reads an int: a decimal literal holds what an int
                // holds, while a hexadecimal or binary one may spell out the
                // bits of a negative number instead.
                if (base != 10 && quint64(expression.integer) <= 0xffffffffULL)
                    expression.integer = qint32(quint32(expression.integer));
                else
                    ok = expression.integer == qint64(qint32(expression.integer));
            }
        }
        if (!ok) {
            return ResultError(Tr::tr("\"%1\" is not a number a virtual machine could hold.")
                                   .arg(m_text.mid(start, m_pos - start)));
        }
        return expression;
    }

    static bool isHexLetter(QChar c)
    {
        const QChar lower = c.toLower();
        return lower >= 'a' && lower <= 'f';
    }

    Result<JdwpExpression> parseString()
    {
        ++m_pos; // The opening quote.
        QString contents;
        while (!atEnd() && current() != '"') {
            const Result<QChar> c = takeCharacter();
            if (!c)
                return ResultError(c.error());
            contents += *c;
        }
        if (atEnd())
            return ResultError(Tr::tr("The string in \"%1\" is not closed.").arg(m_text));
        ++m_pos; // The closing quote.
        JdwpExpression expression;
        expression.kind = JdwpExpression::Kind::String;
        expression.text = contents;
        return expression;
    }

    Result<JdwpExpression> parseChar()
    {
        ++m_pos; // The opening quote.
        if (atEnd() || current() == '\'')
            return ResultError(Tr::tr("The character in \"%1\" is empty.").arg(m_text));
        const Result<QChar> c = takeCharacter();
        if (!c)
            return ResultError(c.error());
        if (current() != '\'')
            return ResultError(Tr::tr("The character in \"%1\" is not closed.").arg(m_text));
        ++m_pos; // The closing quote.
        JdwpExpression expression;
        expression.kind = JdwpExpression::Kind::Char;
        expression.integer = c->unicode();
        return expression;
    }

    Result<QChar> takeCharacter()
    {
        const QChar c = current();
        ++m_pos;
        if (c != '\\')
            return c;
        const QChar escape = current();
        ++m_pos;
        switch (escape.unicode()) {
        case 'n': return QChar('\n');
        case 't': return QChar('\t');
        case 'r': return QChar('\r');
        case 'b': return QChar('\b');
        case 'f': return QChar('\f');
        case 's': return QChar(' ');
        case '0': return QChar(QChar::Null);
        case '\'': return QChar('\'');
        case '"': return QChar('"');
        case '\\': return QChar('\\');
        case 'u': {
            const QString digits = m_text.mid(m_pos, 4);
            bool ok = false;
            const uint code = digits.toUInt(&ok, 16);
            if (!ok || digits.size() < 4)
                return ResultError(Tr::tr("\"\\u%1\" is not a character.").arg(digits));
            m_pos += 4;
            return QChar(code);
        }
        default:
            break;
        }
        return ResultError(Tr::tr("\"\\%1\" is not an escape Java knows.").arg(escape));
    }

    const QString m_text;
    int m_pos = 0;
};

Result<JdwpExpression> parseJdwpExpression(const QString &expression)
{
    return Parser(expression).run();
}

} // namespace Debugger::Internal
