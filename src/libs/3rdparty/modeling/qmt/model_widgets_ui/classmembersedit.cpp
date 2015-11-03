/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#include "classmembersedit.h"

#include "qmt/model/mclassmember.h"
#include "qmt/model_ui/stereotypescontroller.h"
#include "qmt/infrastructure/qmtassert.h"

// TODO implement multi-line signatures for attributes and methods

namespace qmt {

class ClassMembersEdit::Cursor
{
public:
    explicit Cursor(const QString &text);

public:

    bool isValid() const { return m_valid; }

    bool atEnd() const { return m_pos == m_text.length(); }

    int getPosition() const { return m_pos; }

    void setPosition(int pos);

    QString readWord();

    bool skip(const QString &s);

    QString readUntil(const QString &delimiter);

    void unreadWord();

    void skipUntilOrNewline(const QString &delimiter);

public:

    QString readWordFromRight();

    bool skipFromRight(const QString &s);

public:

    QString extractSubstr(int start, int stop);

public:

    void skipWhitespaces();

    void skipWhitespacesFromRight();

private:

    QString preparse(const QString &text);

private:

    QString m_text;

    bool m_valid;

    int m_pos;

    int m_lastPos;

};

ClassMembersEdit::Cursor::Cursor(const QString &text)
    : m_text(preparse(text)),
      m_valid(true),
      m_pos(0),
      m_lastPos(-1)
{
}

void ClassMembersEdit::Cursor::setPosition(int pos)
{
    if (m_valid) {
        m_pos = pos;
    }
}

QString ClassMembersEdit::Cursor::readWord()
{
    skipWhitespaces();
    QString word;
    if (m_valid && m_pos < m_text.length()) {
        m_lastPos = m_pos;
        QChar c = m_text.at(m_pos);
        ++m_pos;
        if (c.isLetterOrNumber() ||c == QLatin1Char('_')) {
            word = c;
            while (m_valid && m_pos < m_text.length() && (m_text.at(m_pos).isLetterOrNumber() || m_text.at(m_pos) == QLatin1Char('_'))) {
                word += m_text.at(m_pos);
                ++m_pos;
            }
        } else {
            if (c == QLatin1Char('<') && m_pos < m_text.length() && m_text.at(m_pos) == QLatin1Char('<')) {
                ++m_pos;
                word = QStringLiteral("<<");
            } else if (c == QLatin1Char('>') && m_pos < m_text.length() && m_text.at(m_pos) == QLatin1Char('>')) {
                ++m_pos;
                word = QStringLiteral(">>");
            } else {
                word = c;
            }
        }
    } else {
        m_valid = false;
    }
    return word;
}

bool ClassMembersEdit::Cursor::skip(const QString &s)
{
    skipWhitespaces();
    if (m_valid && m_pos + s.length() <= m_text.length() && s.compare(m_text.mid(m_pos, s.length()), s, Qt::CaseInsensitive) == 0) {
        m_pos += s.length();
        return true;
    }
    return false;
}

QString ClassMembersEdit::Cursor::readUntil(const QString &delimiter)
{
    QString s;
    while (m_valid) {
        if (m_pos >= m_text.length() || m_text.at(m_pos) == QStringLiteral("\n")) {
            m_valid = false;
            return s;
        }
        if (m_pos + delimiter.length() <= m_text.length() && s.compare(m_text.mid(m_pos, delimiter.length()), delimiter, Qt::CaseInsensitive) == 0) {
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
    if (!m_valid) {
        return;
    }
    if (m_lastPos < 0) {
        m_valid = false;
        return;
    }
    m_pos = m_lastPos;
}

void ClassMembersEdit::Cursor::skipUntilOrNewline(const QString &delimiter)
{
    while (m_valid) {
        if (m_pos >= m_text.length()) {
            return;
        }
        if (m_text.at(m_pos) == QStringLiteral("\n")) {
            return;
        }
        if (m_pos + delimiter.length() <= m_text.length() && QString::compare(m_text.mid(m_pos, delimiter.length()), delimiter, Qt::CaseInsensitive) == 0) {
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
    if (m_valid && m_pos >= 0) {
        m_lastPos = m_pos;
        QChar c = m_text.at(m_pos);
        --m_pos;
        if (c.isLetterOrNumber() || c == QLatin1Char('_')) {
            word = c;
            while (m_valid && m_pos >= 0 && (m_text.at(m_pos).isLetterOrNumber() || m_text.at(m_pos) == QLatin1Char('_'))) {
                word = m_text.at(m_pos) + word;
                --m_pos;
            }
        } else {
            if (c == QLatin1Char('<') && m_pos >= 0 && m_text.at(m_pos) == QLatin1Char('<')) {
                --m_pos;
                word = QStringLiteral("<<");
            } else if (c == QLatin1Char('>') && m_pos >= 0 && m_text.at(m_pos) == QLatin1Char('>')) {
                --m_pos;
                word = QStringLiteral(">>");
            } else {
                word = c;
            }
        }
    } else {
        m_valid = false;
    }
    return word;

}

bool ClassMembersEdit::Cursor::skipFromRight(const QString &s)
{
    skipWhitespacesFromRight();
    if (m_valid && m_pos - s.length() >= 0
            && s.compare(m_text.mid(m_pos - s.length() + 1, s.length()), s, Qt::CaseInsensitive) == 0) {
        m_pos -= s.length();
        return true;
    }
    return false;
}

QString ClassMembersEdit::Cursor::extractSubstr(int start, int stop)
{
    if (m_valid && start >= 0 && start < m_text.length() && stop >= start && stop < m_text.length()) {
        return m_text.mid(start, stop - start + 1);
    }
    m_valid = false;
    return QStringLiteral("");
}

void ClassMembersEdit::Cursor::skipWhitespaces()
{
    while (m_valid && m_pos < m_text.length() && m_text.at(m_pos).isSpace() && m_text.at(m_pos) != QStringLiteral("\n")) {
        ++m_pos;
    }
    if (m_pos >= m_text.length()) {
        m_valid = false;
    }
}

void ClassMembersEdit::Cursor::skipWhitespacesFromRight()
{
    while (m_valid && m_pos >= 0 && m_text.at(m_pos).isSpace() && m_text.at(m_pos) != QStringLiteral("\n")) {
        --m_pos;
    }
    if (m_pos < 0) {
        m_valid = false;
    }
}

QString ClassMembersEdit::Cursor::preparse(const QString &text)
{
    QString parsed_text;
    if (!text.isEmpty()) {
        QChar last_char = QLatin1Char(' ');
        bool in_c_comment = false;
        bool in_cpp_comment = false;
        foreach (const QChar &c, text) {
            if (!in_c_comment && !in_cpp_comment && last_char == QLatin1Char('/') && c == QLatin1Char('/')) {
                in_cpp_comment = true;
                last_char = QLatin1Char('\n');
            } else if (!in_c_comment && !in_cpp_comment && last_char == QLatin1Char('/') && c == QLatin1Char('*')) {
                in_c_comment = true;
                last_char = QLatin1Char(' ');
            } else if (in_c_comment && !in_cpp_comment && last_char == QLatin1Char('*') && c == QLatin1Char('/')) {
                in_c_comment = false;
            } else if (!in_c_comment && in_cpp_comment && c == QLatin1Char('\n')) {
                in_cpp_comment = false;
            } else if (in_c_comment || in_cpp_comment) {
                // ignore char
            } else {
                parsed_text += last_char;
                last_char = c;
            }
        }
        parsed_text += last_char;
    }
    return parsed_text;
}


struct ClassMembersEdit::ClassMembersEditPrivate {
    ClassMembersEditPrivate()
        : m_valid(true)
    {
    }

    bool m_valid;
    QList<MClassMember> m_members;
};

ClassMembersEdit::ClassMembersEdit(QWidget *parent)
    : QPlainTextEdit(parent),
      d(new ClassMembersEditPrivate)
{
    setTabChangesFocus(true);
    connect(this, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
}

ClassMembersEdit::~ClassMembersEdit()
{
    delete d;
}

QList<MClassMember> ClassMembersEdit::getMembers() const
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
    if (ok != d->m_valid) {
        d->m_valid = ok;
        emit statusChanged(d->m_valid);
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
    if (ok != d->m_valid) {
        d->m_valid = ok;
        emit statusChanged(d->m_valid);
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
    MClassMember::Visibility current_visibility = MClassMember::VISIBILITY_UNDEFINED;
    QString current_group;

    QString text;
    foreach (const MClassMember &member, members) {
        bool add_newline = false;
        bool add_space = false;
        if (member.getVisibility() != current_visibility) {
            if (member.getVisibility() != MClassMember::VISIBILITY_UNDEFINED) {
                QString vis;
                switch (member.getVisibility()) {
                case MClassMember::VISIBILITY_UNDEFINED:
                    break;
                case MClassMember::VISIBILITY_PUBLIC:
                    vis = QStringLiteral("public:");
                    break;
                case MClassMember::VISIBILITY_PROTECTED:
                    vis = QStringLiteral("protected:");
                    break;
                case MClassMember::VISIBILITY_PRIVATE:
                    vis = QStringLiteral("private:");
                    break;
                case MClassMember::VISIBILITY_SIGNALS:
                    vis = QStringLiteral("signals:");
                    break;
                case MClassMember::VISIBILITY_PRIVATE_SLOTS:
                    vis = QStringLiteral("private slots:");
                    break;
                case MClassMember::VISIBILITY_PROTECTED_SLOTS:
                    vis = QStringLiteral("protected slots:");
                    break;
                case MClassMember::VISIBILITY_PUBLIC_SLOTS:
                    vis = QStringLiteral("public slots:");
                    break;
                }
                if (!text.isEmpty()) {
                    text += QStringLiteral("\n");
                }
                text += vis;
                add_newline = true;
                add_space = true;
            }
            current_visibility = member.getVisibility();
        }
        if (member.getGroup() != current_group) {
            if (add_space) {
                text += QStringLiteral(" ");
            } else if (!text.isEmpty()) {
                text += QStringLiteral("\n");
            }
            text += QString(QStringLiteral("[%1]")).arg(member.getGroup());
            add_newline = true;
            current_group = member.getGroup();
        }
        if (add_newline) {
            text += QStringLiteral("\n");
        }

        if (!member.getStereotypes().isEmpty()) {
            StereotypesController ctrl;
            text += QString(QStringLiteral("<<%1>> ")).arg(ctrl.toString(member.getStereotypes()));
        }
        if (member.getProperties() & MClassMember::PROPERTY_QSIGNAL) {
            text += QStringLiteral("signal ");
        }
        if (member.getProperties() & MClassMember::PROPERTY_QSLOT) {
            text += QStringLiteral("slot ");
        }
        if (member.getProperties() & MClassMember::PROPERTY_QINVOKABLE) {
            text += QStringLiteral("invokable ");
        }
        if (member.getProperties() & MClassMember::PROPERTY_VIRTUAL) {
            text += QStringLiteral("virtual ");
        }
        text += member.getDeclaration();
        if (member.getProperties() & MClassMember::PROPERTY_CONST) {
            text += QStringLiteral(" const");
        }
        if (member.getProperties() & MClassMember::PROPERTY_OVERRIDE) {
            text += QStringLiteral(" override");
        }
        if (member.getProperties() & MClassMember::PROPERTY_FINAL) {
            text += QStringLiteral(" final");
        }
        if (member.getProperties() & MClassMember::PROPERTY_ABSTRACT) {
            text += QStringLiteral(" = 0");
        }
        text += QStringLiteral(";\n");
    }

    return text;
}

QList<MClassMember> ClassMembersEdit::parse(const QString &text, bool *ok)
{
    QMT_CHECK(ok);

    *ok = true;
    QList<MClassMember> members;
    MClassMember member;
    MClassMember::Visibility current_visibility = MClassMember::VISIBILITY_UNDEFINED;
    QString current_group;

    Cursor cursor(text);
    while (cursor.isValid() && *ok) {
        cursor.skipWhitespaces();
        if (!cursor.isValid()) {
            return members;
        }
        member = MClassMember();
        QString word = cursor.readWord().toLower();
        for (;;) {
            if (word == QStringLiteral("public")) {
                current_visibility = MClassMember::VISIBILITY_PUBLIC;
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("protected")) {
                current_visibility = MClassMember::VISIBILITY_PROTECTED;
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("private")) {
                current_visibility = MClassMember::VISIBILITY_PRIVATE;
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("signals")) {
                current_visibility = MClassMember::VISIBILITY_SIGNALS;
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("slots")) {
                switch (current_visibility) {
                case MClassMember::VISIBILITY_PRIVATE:
                    current_visibility = MClassMember::VISIBILITY_PRIVATE_SLOTS;
                    break;
                case MClassMember::VISIBILITY_PROTECTED:
                    current_visibility = MClassMember::VISIBILITY_PROTECTED_SLOTS;
                    break;
                case MClassMember::VISIBILITY_PUBLIC:
                    current_visibility = MClassMember::VISIBILITY_PUBLIC_SLOTS;
                    break;
                default:
                    current_visibility = MClassMember::VISIBILITY_PRIVATE_SLOTS;
                    break;
                }
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("[")) {
                current_group = cursor.readUntil(QStringLiteral("]"));
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("<<")) {
                QString stereotypes = cursor.readUntil(QStringLiteral(">>"));
                StereotypesController ctrl;
                member.setStereotypes(ctrl.fromString(stereotypes));
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("virtual")) {
                member.setProperties(member.getProperties() | MClassMember::PROPERTY_VIRTUAL);
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("signal") || word == QStringLiteral("q_signal")) {
                member.setProperties(member.getProperties() | MClassMember::PROPERTY_QSIGNAL);
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("slot") || word == QStringLiteral("q_slot")) {
                member.setProperties(member.getProperties() | MClassMember::PROPERTY_QSLOT);
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("invokable") || word == QStringLiteral("q_invokable")) {
                member.setProperties(member.getProperties() | MClassMember::PROPERTY_QINVOKABLE);
            } else if (word == QStringLiteral(":")) {
                word = cursor.readWord().toLower();
            } else {
                cursor.unreadWord();
                break;
            }
        }
        member.setVisibility(current_visibility);
        member.setGroup(current_group);
        if (word != QStringLiteral("\n")) {
            int declaration_start = cursor.getPosition();
            cursor.skipUntilOrNewline(QStringLiteral(";"));
            int next_line_position = cursor.getPosition();
            cursor.setPosition(next_line_position - 1);
            word = cursor.readWordFromRight().toLower();
            if (word == QStringLiteral(";")) {
                word = cursor.readWordFromRight().toLower();
            }
            for (;;) {
                if (word == QStringLiteral("0")) {
                    member.setProperties(member.getProperties() | MClassMember::PROPERTY_ABSTRACT);
                    word = cursor.readWordFromRight().toLower();
                } else if (word == QStringLiteral("=")) {
                    word = cursor.readWordFromRight().toLower();
                } else if (word == QStringLiteral("final")) {
                    member.setProperties(member.getProperties() | MClassMember::PROPERTY_FINAL);
                    word = cursor.readWordFromRight().toLower();
                } else if (word == QStringLiteral("override")) {
                    member.setProperties(member.getProperties() | MClassMember::PROPERTY_OVERRIDE);
                    word = cursor.readWordFromRight().toLower();
                } else if (word == QStringLiteral("const")) {
                    member.setProperties(member.getProperties() | MClassMember::PROPERTY_CONST);
                    word = cursor.readWordFromRight().toLower();
                } else {
                    cursor.unreadWord();
                    break;
                }
            }
            int declaration_stop = cursor.getPosition();
            QString declaration = cursor.extractSubstr(declaration_start, declaration_stop).trimmed();
            if (!cursor.isValid()) {
                break;
            }
            if (!declaration.isEmpty()) {
                member.setDeclaration(declaration);
                if (declaration.endsWith(QStringLiteral(")"))) {
                    member.setMemberType(MClassMember::MEMBER_METHOD);
                } else {
                    member.setMemberType(MClassMember::MEMBER_ATTRIBUTE);
                }
                members.append(member);
            }
            cursor.setPosition(next_line_position);
            if (cursor.atEnd()) {
                return members;
            }
            cursor.skip(QStringLiteral("\n"));
        } else {
            word = cursor.readWord().toLower();
        }
    }
    if (!cursor.isValid()) {
        *ok = false;
    }

    return members;
}

}
