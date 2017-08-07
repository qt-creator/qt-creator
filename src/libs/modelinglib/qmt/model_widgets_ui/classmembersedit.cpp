/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "classmembersedit.h"

#include "qmt/model/mclassmember.h"
#include "qmt/model_ui/stereotypescontroller.h"
#include "qmt/infrastructure/qmtassert.h"

namespace qmt {

class ClassMembersEdit::Cursor
{
public:
    explicit Cursor(const QString &text);

    bool isValid() const { return m_isValid; }
    bool atEnd() const { return m_pos == m_text.length(); }
    int position() const { return m_pos; }
    void setPosition(int pos);

    QString readWord();
    bool skip(const QString &s);
    QString readUntil(const QString &delimiter);
    void unreadWord();
    void skipUntilOrNewline(const QString &delimiter);

    QString readWordFromRight();
    bool skipFromRight(const QString &s);
    bool searchOutsideOfBracketsFromRight(const QString &s);

    QString extractSubstr(int start, int stop);

    void skipWhitespaces();
    void skipWhitespacesFromRight();

private:
    QString preparse(const QString &text);

    QString m_text;
    bool m_isValid = true;
    int m_pos = 0;
    int m_lastPos = -1;
};

ClassMembersEdit::Cursor::Cursor(const QString &text)
    : m_text(preparse(text))
{
}

void ClassMembersEdit::Cursor::setPosition(int pos)
{
    if (m_isValid)
        m_pos = pos;
}

QString ClassMembersEdit::Cursor::readWord()
{
    skipWhitespaces();
    QString word;
    if (m_isValid && m_pos < m_text.length()) {
        m_lastPos = m_pos;
        QChar c = m_text.at(m_pos);
        ++m_pos;
        if (c.isLetterOrNumber() || c == QLatin1Char('_')) {
            word = c;
            while (m_isValid && m_pos < m_text.length()
                   && (m_text.at(m_pos).isLetterOrNumber() || m_text.at(m_pos) == QLatin1Char('_'))) {
                word += m_text.at(m_pos);
                ++m_pos;
            }
        } else {
            if (c == QLatin1Char('<') && m_pos < m_text.length() && m_text.at(m_pos) == QLatin1Char('<')) {
                ++m_pos;
                word = "<<";
            } else if (c == QLatin1Char('>') && m_pos < m_text.length() && m_text.at(m_pos) == QLatin1Char('>')) {
                ++m_pos;
                word = ">>";
            } else {
                word = c;
            }
        }
    } else {
        m_isValid = false;
    }
    return word;
}

bool ClassMembersEdit::Cursor::skip(const QString &s)
{
    skipWhitespaces();
    if (m_isValid && m_pos + s.length() <= m_text.length()
            && s.compare(m_text.mid(m_pos, s.length()), s, Qt::CaseInsensitive) == 0) {
        m_pos += s.length();
        return true;
    }
    return false;
}

QString ClassMembersEdit::Cursor::readUntil(const QString &delimiter)
{
    QString s;
    while (m_isValid) {
        if (m_pos >= m_text.length() || m_text.at(m_pos) == '\n') {
            m_isValid = false;
            return s;
        }
        if (m_pos + delimiter.length() <= m_text.length()
                && s.compare(m_text.mid(m_pos, delimiter.length()), delimiter, Qt::CaseInsensitive) == 0) {
            m_pos += delimiter.length();
            return s;
        }
        s += m_text.at(m_pos);
        ++m_pos;
    }
    return s;
}

void ClassMembersEdit::Cursor::unreadWord()
{
    if (!m_isValid)
        return;
    if (m_lastPos < 0) {
        m_isValid = false;
        return;
    }
    m_pos = m_lastPos;
}

void ClassMembersEdit::Cursor::skipUntilOrNewline(const QString &delimiter)
{
    while (m_isValid) {
        if (m_pos >= m_text.length())
            return;
        if (m_text.at(m_pos) == '\n')
            return;
        if (m_pos + delimiter.length() <= m_text.length()
                && QString::compare(m_text.mid(m_pos, delimiter.length()), delimiter, Qt::CaseInsensitive) == 0) {
            m_pos += delimiter.length();
            return;
        }
        ++m_pos;
    }
}

QString ClassMembersEdit::Cursor::readWordFromRight()
{
    skipWhitespacesFromRight();
    QString word;
    if (m_isValid && m_pos >= 0) {
        m_lastPos = m_pos;
        QChar c = m_text.at(m_pos);
        --m_pos;
        if (c.isLetterOrNumber() || c == QLatin1Char('_')) {
            word = c;
            while (m_isValid && m_pos >= 0
                   && (m_text.at(m_pos).isLetterOrNumber() || m_text.at(m_pos) == QLatin1Char('_'))) {
                word = m_text.at(m_pos) + word;
                --m_pos;
            }
        } else {
            if (c == QLatin1Char('<') && m_pos >= 0 && m_text.at(m_pos) == QLatin1Char('<')) {
                --m_pos;
                word = "<<";
            } else if (c == QLatin1Char('>') && m_pos >= 0 && m_text.at(m_pos) == QLatin1Char('>')) {
                --m_pos;
                word = ">>";
            } else {
                word = c;
            }
        }
    } else {
        m_isValid = false;
    }
    return word;

}

bool ClassMembersEdit::Cursor::skipFromRight(const QString &s)
{
    skipWhitespacesFromRight();
    if (m_isValid && m_pos - s.length() >= 0
            && s.compare(m_text.mid(m_pos - s.length() + 1, s.length()), s, Qt::CaseInsensitive) == 0) {
        m_pos -= s.length();
        return true;
    }
    return false;
}

bool ClassMembersEdit::Cursor::searchOutsideOfBracketsFromRight(const QString &s)
{
    if (!m_isValid)
        return false;
    int startPos = m_pos;
    int lastPos = m_lastPos;
    int brackets = 0;
    for (;;) {
        const QString word = readWordFromRight();
        if (!m_isValid || word == "\n")
            break;
        if (brackets == 0 && word == s)
            return true;
        else if (word == "(" || word == "[" || word == "{")
            ++brackets;
        else if (word == ")" || word == "]" || word == "}")
            --brackets;
    }
    m_isValid = true;
    m_pos = startPos;
    m_lastPos = lastPos;
    return false;
}

QString ClassMembersEdit::Cursor::extractSubstr(int start, int stop)
{
    if (m_isValid && start >= 0 && start < m_text.length() && stop >= start && stop < m_text.length())
        return m_text.mid(start, stop - start + 1);
    m_isValid = false;
    return QString();
}

void ClassMembersEdit::Cursor::skipWhitespaces()
{
    while (m_isValid && m_pos < m_text.length() && m_text.at(m_pos).isSpace() && m_text.at(m_pos) != '\n')
        ++m_pos;
    if (m_pos >= m_text.length())
        m_isValid = false;
}

void ClassMembersEdit::Cursor::skipWhitespacesFromRight()
{
    while (m_isValid && m_pos >= 0 && m_text.at(m_pos).isSpace() && m_text.at(m_pos) != '\n')
        --m_pos;
    if (m_pos < 0)
        m_isValid = false;
}

QString ClassMembersEdit::Cursor::preparse(const QString &text)
{
    QString parsedText;
    if (!text.isEmpty()) {
        QChar lastChar = QLatin1Char(' ');
        bool inCComment = false;
        bool inCppComment = false;
        int braces = 0;
        foreach (QChar c, text) {
            if (!inCComment && !inCppComment && lastChar == QLatin1Char('/') && c == QLatin1Char('/')) {
                inCppComment = true;
                lastChar = QLatin1Char(' ');
            } else if (!inCComment && !inCppComment && lastChar == QLatin1Char('/') && c == QLatin1Char('*')) {
                inCComment = true;
                lastChar = QLatin1Char(' ');
            } else if (inCComment && !inCppComment && lastChar == QLatin1Char('*') && c == QLatin1Char('/')) {
                inCComment = false;
                lastChar = QLatin1Char(' ');
            } else if (!inCComment && inCppComment && c == QLatin1Char('\n')) {
                inCppComment = false;
                lastChar = QLatin1Char('\n');
            } else if (inCComment || inCppComment) {
                lastChar = c;
            } else {
                if (c == QLatin1Char('(') || c == QLatin1Char('{') || c == QLatin1Char('['))
                    ++braces;
                else if (c == QLatin1Char(')') || c == QLatin1Char('}') || c == QLatin1Char(']'))
                    --braces;
                else if (c == QLatin1Char('\n') && braces != 0)
                    c = QLatin1Char(' ');
                parsedText += lastChar;
                lastChar = c;
            }
        }
        if (!inCComment && !inCppComment)
            parsedText += lastChar;
    }
    return parsedText;
}

class ClassMembersEdit::ClassMembersEditPrivate
{
public:
    bool m_isValid = true;
    QList<MClassMember> m_members;
};

ClassMembersEdit::ClassMembersEdit(QWidget *parent)
    : QPlainTextEdit(parent),
      d(new ClassMembersEditPrivate)
{
    setTabChangesFocus(true);
    connect(this, &QPlainTextEdit::textChanged, this, &ClassMembersEdit::onTextChanged);
}

ClassMembersEdit::~ClassMembersEdit()
{
    delete d;
}

QList<MClassMember> ClassMembersEdit::members() const
{
    return d->m_members;
}

void ClassMembersEdit::setMembers(const QList<MClassMember> &members)
{
    d->m_members = members;

    setPlainText(build(d->m_members));
}

void ClassMembersEdit::reparse()
{
    bool ok = false;
    QList<MClassMember> members = parse(toPlainText(), &ok);
    if (ok != d->m_isValid) {
        d->m_isValid = ok;
        emit statusChanged(d->m_isValid);
    }
    if (ok) {
        if (members != d->m_members) {
            d->m_members = members;
            emit membersChanged(d->m_members);
        }
        setPlainText(build(members));
    }
}

void ClassMembersEdit::onTextChanged()
{
    bool ok = false;
    QList<MClassMember> members = parse(toPlainText(), &ok);
    if (ok != d->m_isValid) {
        d->m_isValid = ok;
        emit statusChanged(d->m_isValid);
    }
    if (ok) {
        if (members != d->m_members) {
            d->m_members = members;
            emit membersChanged(d->m_members);
        }
    }
}

QString ClassMembersEdit::build(const QList<MClassMember> &members)
{
    MClassMember::Visibility currentVisibility = MClassMember::VisibilityUndefined;
    QString currentGroup;

    QString text;
    foreach (const MClassMember &member, members) {
        bool addNewline = false;
        bool addSpace = false;
        if (member.visibility() != currentVisibility) {
            if (member.visibility() != MClassMember::VisibilityUndefined) {
                QString vis;
                switch (member.visibility()) {
                case MClassMember::VisibilityUndefined:
                    break;
                case MClassMember::VisibilityPublic:
                    vis = "public:";
                    break;
                case MClassMember::VisibilityProtected:
                    vis = "protected:";
                    break;
                case MClassMember::VisibilityPrivate:
                    vis = "private:";
                    break;
                case MClassMember::VisibilitySignals:
                    vis = "signals:";
                    break;
                case MClassMember::VisibilityPrivateSlots:
                    vis = "private slots:";
                    break;
                case MClassMember::VisibilityProtectedSlots:
                    vis = "protected slots:";
                    break;
                case MClassMember::VisibilityPublicSlots:
                    vis = "public slots:";
                    break;
                }
                if (!text.isEmpty())
                    text += "\n";
                text += vis;
                addNewline = true;
                addSpace = true;
            }
            currentVisibility = member.visibility();
        }
        if (member.group() != currentGroup) {
            if (addSpace)
                text += " ";
            else if (!text.isEmpty())
                text += "\n";
            text += QString("[%1]").arg(member.group());
            addNewline = true;
            currentGroup = member.group();
        }
        if (addNewline)
            text += "\n";

        if (!member.stereotypes().isEmpty()) {
            StereotypesController ctrl;
            text += QString("<<%1>> ").arg(ctrl.toString(member.stereotypes()));
        }
        if (member.properties() & MClassMember::PropertyQsignal)
            text += "signal ";
        if (member.properties() & MClassMember::PropertyQslot)
            text += "slot ";
        if (member.properties() & MClassMember::PropertyQinvokable)
            text += "invokable ";
        if (member.properties() & MClassMember::PropertyStatic)
            text += "static ";
        if (member.properties() & MClassMember::PropertyVirtual)
            text += "virtual ";
        if (member.properties() & MClassMember::PropertyConstexpr)
            text += "constexpr ";
        text += member.declaration();
        if (member.properties() & MClassMember::PropertyConst)
            text += " const";
        if (member.properties() & MClassMember::PropertyOverride)
            text += " override";
        if (member.properties() & MClassMember::PropertyFinal)
            text += " final";
        if (member.properties() & MClassMember::PropertyAbstract)
            text += " = 0";
        text += ";\n";
    }

    return text;
}

QList<MClassMember> ClassMembersEdit::parse(const QString &text, bool *ok)
{
    QMT_ASSERT(ok, return QList<MClassMember>());

    *ok = true;
    QList<MClassMember> members;
    MClassMember member;
    MClassMember::Visibility currentVisibility = MClassMember::VisibilityUndefined;
    QString currentGroup;

    Cursor cursor(text);
    while (cursor.isValid() && *ok) {
        cursor.skipWhitespaces();
        if (!cursor.isValid())
            return members;
        member = MClassMember();
        QString word = cursor.readWord().toLower();
        for (;;) {
            if (word == "public") {
                currentVisibility = MClassMember::VisibilityPublic;
                word = cursor.readWord().toLower();
            } else if (word == "protected") {
                currentVisibility = MClassMember::VisibilityProtected;
                word = cursor.readWord().toLower();
            } else if (word == "private") {
                currentVisibility = MClassMember::VisibilityPrivate;
                word = cursor.readWord().toLower();
            } else if (word == "signals") {
                currentVisibility = MClassMember::VisibilitySignals;
                word = cursor.readWord().toLower();
            } else if (word == "slots") {
                switch (currentVisibility) {
                case MClassMember::VisibilityPrivate:
                    currentVisibility = MClassMember::VisibilityPrivateSlots;
                    break;
                case MClassMember::VisibilityProtected:
                    currentVisibility = MClassMember::VisibilityProtectedSlots;
                    break;
                case MClassMember::VisibilityPublic:
                    currentVisibility = MClassMember::VisibilityPublicSlots;
                    break;
                default:
                    currentVisibility = MClassMember::VisibilityPrivateSlots;
                    break;
                }
                word = cursor.readWord().toLower();
            } else if (word == "[") {
                currentGroup = cursor.readUntil("]");
                word = cursor.readWord().toLower();
            } else if (word == "<<") {
                QString stereotypes = cursor.readUntil(">>");
                StereotypesController ctrl;
                member.setStereotypes(ctrl.fromString(stereotypes));
                word = cursor.readWord().toLower();
            } else if (word == "static") {
                member.setProperties(member.properties() | MClassMember::PropertyStatic);
                word = cursor.readWord().toLower();
            } else if (word == "virtual") {
                member.setProperties(member.properties() | MClassMember::PropertyVirtual);
                word = cursor.readWord().toLower();
            } else if (word == "constexpr" || word == "q_decl_constexpr"
                       || word == "q_decl_relaxed_constexpr") {
                member.setProperties(member.properties() | MClassMember::PropertyConstexpr);
                word = cursor.readWord().toLower();
            } else if (word == "signal" || word == "q_signal") {
                member.setProperties(member.properties() | MClassMember::PropertyQsignal);
                word = cursor.readWord().toLower();
            } else if (word == "slot" || word == "q_slot") {
                member.setProperties(member.properties() | MClassMember::PropertyQslot);
                word = cursor.readWord().toLower();
            } else if (word == "invokable" || word == "q_invokable") {
                member.setProperties(member.properties() | MClassMember::PropertyQinvokable);
                word = cursor.readWord().toLower();
            } else if (word == ":") {
                word = cursor.readWord().toLower();
            } else {
                cursor.unreadWord();
                break;
            }
        }
        member.setVisibility(currentVisibility);
        member.setGroup(currentGroup);
        if (word != "\n") {
            int declarationStart = cursor.position();
            cursor.skipUntilOrNewline(";");
            int nextLinePosition = cursor.position();
            cursor.setPosition(nextLinePosition - 1);
            word = cursor.readWordFromRight().toLower();
            if (word != ";")
                cursor.unreadWord();

            int endPosition = cursor.position();
            QString expr;
            if (cursor.searchOutsideOfBracketsFromRight("="))
                expr = cursor.extractSubstr(cursor.position() + 2, endPosition).trimmed();
            word = cursor.readWordFromRight().toLower();
            for (;;) {
                // TODO ignore throw(), noexpect(*),
                if (word == "final" || word == "q_decl_final") {
                    member.setProperties(member.properties() | MClassMember::PropertyFinal);
                    word = cursor.readWordFromRight().toLower();
                } else if (word == "override" || word == "q_decl_override") {
                    member.setProperties(member.properties() | MClassMember::PropertyOverride);
                    word = cursor.readWordFromRight().toLower();
                } else if (word == "const") {
                    member.setProperties(member.properties() | MClassMember::PropertyConst);
                    word = cursor.readWordFromRight().toLower();
                } else if (word == "noexpect") {
                    // just ignore it
                    word = cursor.readWordFromRight().toLower();
                } else {
                    cursor.unreadWord();
                    break;
                }
            }
            int declarationStop = cursor.position();
            QString declaration = cursor.extractSubstr(declarationStart, declarationStop).trimmed();
            if (!cursor.isValid())
                break;
            if (!declaration.isEmpty()) {
                member.setDeclaration(declaration);
                // TODO recognize function pointer
                if (declaration.endsWith(")")) {
                    member.setMemberType(MClassMember::MemberMethod);
                    if (expr == "0")
                        member.setProperties(member.properties() | MClassMember::PropertyAbstract);
                } else {
                    member.setMemberType(MClassMember::MemberAttribute);
                }
                members.append(member);
            }
            cursor.setPosition(nextLinePosition);
            if (cursor.atEnd())
                return members;
            cursor.skip("\n");
        } else {
            word = cursor.readWord().toLower();
        }
    }
    if (!cursor.isValid())
        *ok = false;

    return members;
}

} // namespace qmt
