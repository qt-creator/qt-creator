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

    bool isValid() const { return m_isValid; }

    bool atEnd() const { return m_pos == m_text.length(); }

    int position() const { return m_pos; }

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

    bool m_isValid;

    int m_pos;

    int m_lastPos;

};

ClassMembersEdit::Cursor::Cursor(const QString &text)
    : m_text(preparse(text)),
      m_isValid(true),
      m_pos(0),
      m_lastPos(-1)
{
}

void ClassMembersEdit::Cursor::setPosition(int pos)
{
    if (m_isValid) {
        m_pos = pos;
    }
}

QString ClassMembersEdit::Cursor::readWord()
{
    skipWhitespaces();
    QString word;
    if (m_isValid && m_pos < m_text.length()) {
        m_lastPos = m_pos;
        QChar c = m_text.at(m_pos);
        ++m_pos;
        if (c.isLetterOrNumber() ||c == QLatin1Char('_')) {
            word = c;
            while (m_isValid && m_pos < m_text.length() && (m_text.at(m_pos).isLetterOrNumber() || m_text.at(m_pos) == QLatin1Char('_'))) {
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
        m_isValid = false;
    }
    return word;
}

bool ClassMembersEdit::Cursor::skip(const QString &s)
{
    skipWhitespaces();
    if (m_isValid && m_pos + s.length() <= m_text.length() && s.compare(m_text.mid(m_pos, s.length()), s, Qt::CaseInsensitive) == 0) {
        m_pos += s.length();
        return true;
    }
    return false;
}

QString ClassMembersEdit::Cursor::readUntil(const QString &delimiter)
{
    QString s;
    while (m_isValid) {
        if (m_pos >= m_text.length() || m_text.at(m_pos) == QStringLiteral("\n")) {
            m_isValid = false;
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
    if (!m_isValid) {
        return;
    }
    if (m_lastPos < 0) {
        m_isValid = false;
        return;
    }
    m_pos = m_lastPos;
}

void ClassMembersEdit::Cursor::skipUntilOrNewline(const QString &delimiter)
{
    while (m_isValid) {
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
    if (m_isValid && m_pos >= 0) {
        m_lastPos = m_pos;
        QChar c = m_text.at(m_pos);
        --m_pos;
        if (c.isLetterOrNumber() || c == QLatin1Char('_')) {
            word = c;
            while (m_isValid && m_pos >= 0 && (m_text.at(m_pos).isLetterOrNumber() || m_text.at(m_pos) == QLatin1Char('_'))) {
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

QString ClassMembersEdit::Cursor::extractSubstr(int start, int stop)
{
    if (m_isValid && start >= 0 && start < m_text.length() && stop >= start && stop < m_text.length()) {
        return m_text.mid(start, stop - start + 1);
    }
    m_isValid = false;
    return QStringLiteral("");
}

void ClassMembersEdit::Cursor::skipWhitespaces()
{
    while (m_isValid && m_pos < m_text.length() && m_text.at(m_pos).isSpace() && m_text.at(m_pos) != QStringLiteral("\n")) {
        ++m_pos;
    }
    if (m_pos >= m_text.length()) {
        m_isValid = false;
    }
}

void ClassMembersEdit::Cursor::skipWhitespacesFromRight()
{
    while (m_isValid && m_pos >= 0 && m_text.at(m_pos).isSpace() && m_text.at(m_pos) != QStringLiteral("\n")) {
        --m_pos;
    }
    if (m_pos < 0) {
        m_isValid = false;
    }
}

QString ClassMembersEdit::Cursor::preparse(const QString &text)
{
    QString parsedText;
    if (!text.isEmpty()) {
        QChar lastChar = QLatin1Char(' ');
        bool inCComment = false;
        bool inCppComment = false;
        foreach (const QChar &c, text) {
            if (!inCComment && !inCppComment && lastChar == QLatin1Char('/') && c == QLatin1Char('/')) {
                inCppComment = true;
                lastChar = QLatin1Char('\n');
            } else if (!inCComment && !inCppComment && lastChar == QLatin1Char('/') && c == QLatin1Char('*')) {
                inCComment = true;
                lastChar = QLatin1Char(' ');
            } else if (inCComment && !inCppComment && lastChar == QLatin1Char('*') && c == QLatin1Char('/')) {
                inCComment = false;
            } else if (!inCComment && inCppComment && c == QLatin1Char('\n')) {
                inCppComment = false;
            } else if (inCComment || inCppComment) {
                // ignore char
            } else {
                parsedText += lastChar;
                lastChar = c;
            }
        }
        parsedText += lastChar;
    }
    return parsedText;
}


class ClassMembersEdit::ClassMembersEditPrivate
{
public:
    ClassMembersEditPrivate()
        : m_isValid(true)
    {
    }

    bool m_isValid;
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
                    vis = QStringLiteral("public:");
                    break;
                case MClassMember::VisibilityProtected:
                    vis = QStringLiteral("protected:");
                    break;
                case MClassMember::VisibilityPrivate:
                    vis = QStringLiteral("private:");
                    break;
                case MClassMember::VisibilitySignals:
                    vis = QStringLiteral("signals:");
                    break;
                case MClassMember::VisibilityPrivateSlots:
                    vis = QStringLiteral("private slots:");
                    break;
                case MClassMember::VisibilityProtectedSlots:
                    vis = QStringLiteral("protected slots:");
                    break;
                case MClassMember::VisibilityPublicSlots:
                    vis = QStringLiteral("public slots:");
                    break;
                }
                if (!text.isEmpty()) {
                    text += QStringLiteral("\n");
                }
                text += vis;
                addNewline = true;
                addSpace = true;
            }
            currentVisibility = member.visibility();
        }
        if (member.group() != currentGroup) {
            if (addSpace) {
                text += QStringLiteral(" ");
            } else if (!text.isEmpty()) {
                text += QStringLiteral("\n");
            }
            text += QString(QStringLiteral("[%1]")).arg(member.group());
            addNewline = true;
            currentGroup = member.group();
        }
        if (addNewline) {
            text += QStringLiteral("\n");
        }

        if (!member.stereotypes().isEmpty()) {
            StereotypesController ctrl;
            text += QString(QStringLiteral("<<%1>> ")).arg(ctrl.toString(member.stereotypes()));
        }
        if (member.properties() & MClassMember::PropertyQsignal) {
            text += QStringLiteral("signal ");
        }
        if (member.properties() & MClassMember::PropertyQslot) {
            text += QStringLiteral("slot ");
        }
        if (member.properties() & MClassMember::PropertyQinvokable) {
            text += QStringLiteral("invokable ");
        }
        if (member.properties() & MClassMember::PropertyVirtual) {
            text += QStringLiteral("virtual ");
        }
        text += member.declaration();
        if (member.properties() & MClassMember::PropertyConst) {
            text += QStringLiteral(" const");
        }
        if (member.properties() & MClassMember::PropertyOverride) {
            text += QStringLiteral(" override");
        }
        if (member.properties() & MClassMember::PropertyFinal) {
            text += QStringLiteral(" final");
        }
        if (member.properties() & MClassMember::PropertyAbstract) {
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
    MClassMember::Visibility currentVisibility = MClassMember::VisibilityUndefined;
    QString currentGroup;

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
                currentVisibility = MClassMember::VisibilityPublic;
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("protected")) {
                currentVisibility = MClassMember::VisibilityProtected;
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("private")) {
                currentVisibility = MClassMember::VisibilityPrivate;
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("signals")) {
                currentVisibility = MClassMember::VisibilitySignals;
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("slots")) {
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
            } else if (word == QStringLiteral("[")) {
                currentGroup = cursor.readUntil(QStringLiteral("]"));
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("<<")) {
                QString stereotypes = cursor.readUntil(QStringLiteral(">>"));
                StereotypesController ctrl;
                member.setStereotypes(ctrl.fromString(stereotypes));
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("virtual")) {
                member.setProperties(member.properties() | MClassMember::PropertyVirtual);
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("signal") || word == QStringLiteral("qSignal")) {
                member.setProperties(member.properties() | MClassMember::PropertyQsignal);
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("slot") || word == QStringLiteral("qSlot")) {
                member.setProperties(member.properties() | MClassMember::PropertyQslot);
                word = cursor.readWord().toLower();
            } else if (word == QStringLiteral("invokable") || word == QStringLiteral("qInvokable")) {
                member.setProperties(member.properties() | MClassMember::PropertyQinvokable);
            } else if (word == QStringLiteral(":")) {
                word = cursor.readWord().toLower();
            } else {
                cursor.unreadWord();
                break;
            }
        }
        member.setVisibility(currentVisibility);
        member.setGroup(currentGroup);
        if (word != QStringLiteral("\n")) {
            int declarationStart = cursor.position();
            cursor.skipUntilOrNewline(QStringLiteral(";"));
            int nextLinePosition = cursor.position();
            cursor.setPosition(nextLinePosition - 1);
            word = cursor.readWordFromRight().toLower();
            if (word == QStringLiteral(";")) {
                word = cursor.readWordFromRight().toLower();
            }
            for (;;) {
                if (word == QStringLiteral("0")) {
                    member.setProperties(member.properties() | MClassMember::PropertyAbstract);
                    word = cursor.readWordFromRight().toLower();
                } else if (word == QStringLiteral("=")) {
                    word = cursor.readWordFromRight().toLower();
                } else if (word == QStringLiteral("final")) {
                    member.setProperties(member.properties() | MClassMember::PropertyFinal);
                    word = cursor.readWordFromRight().toLower();
                } else if (word == QStringLiteral("override")) {
                    member.setProperties(member.properties() | MClassMember::PropertyOverride);
                    word = cursor.readWordFromRight().toLower();
                } else if (word == QStringLiteral("const")) {
                    member.setProperties(member.properties() | MClassMember::PropertyConst);
                    word = cursor.readWordFromRight().toLower();
                } else {
                    cursor.unreadWord();
                    break;
                }
            }
            int declarationStop = cursor.position();
            QString declaration = cursor.extractSubstr(declarationStart, declarationStop).trimmed();
            if (!cursor.isValid()) {
                break;
            }
            if (!declaration.isEmpty()) {
                member.setDeclaration(declaration);
                if (declaration.endsWith(QStringLiteral(")"))) {
                    member.setMemberType(MClassMember::MemberMethod);
                } else {
                    member.setMemberType(MClassMember::MemberAttribute);
                }
                members.append(member);
            }
            cursor.setPosition(nextLinePosition);
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
