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

#include "snippet.h"

#include <QLatin1Char>
#include <QLatin1String>
#include <QTextDocument>

using namespace TextEditor;

const QChar Snippet::kVariableDelimiter(QLatin1Char('$'));

Snippet::Snippet(const QString &groupId, const QString &id) :
    m_isRemoved(false), m_isModified(false), m_groupId(groupId), m_id(id)
{}

Snippet::~Snippet()
{}

const QString &Snippet::id() const
{
    return m_id;
}

const QString &Snippet::groupId() const
{
    return m_groupId;
}

bool Snippet::isBuiltIn() const
{
    return !m_id.isEmpty();
}

void Snippet::setTrigger(const QString &trigger)
{
    m_trigger = trigger;
}

const QString &Snippet::trigger() const
{
    return m_trigger;
}

void Snippet::setContent(const QString &content)
{
    m_content = content;
}

const QString &Snippet::content() const
{
    return m_content;
}

void Snippet::setComplement(const QString &complement)
{
    m_complement = complement;
}

const QString &Snippet::complement() const
{
    return m_complement;
}

void Snippet::setIsRemoved(bool removed)
{
    m_isRemoved = removed;
}

bool Snippet::isRemoved() const
{
    return m_isRemoved;
}

void Snippet::setIsModified(bool modified)
{
    m_isModified = modified;
}

bool Snippet::isModified() const
{
    return m_isModified;
}

QString Snippet::generateTip() const
{
    static const QLatin1Char kNewLine('\n');
    static const QLatin1Char kSpace(' ');
    static const QLatin1String kBr("<br>");
    static const QLatin1String kNbsp("&nbsp;");
    static const QLatin1String kNoBr("<nobr>");
    static const QLatin1String kOpenBold("<b>");
    static const QLatin1String kCloseBold("</b>");
    static const QLatin1String kEllipsis("...");

    QString escapedContent(Qt::escape(m_content));
    escapedContent.replace(kNewLine, kBr);
    escapedContent.replace(kSpace, kNbsp);

    QString tip(kNoBr);
    int count = 0;
    for (int i = 0; i < escapedContent.count(); ++i) {
        if (escapedContent.at(i) != kVariableDelimiter) {
            tip += escapedContent.at(i);
            continue;
        }
        if (++count % 2) {
            tip += kOpenBold;
        } else {
            if (escapedContent.at(i-1) == kVariableDelimiter)
                tip += kEllipsis;
            tip += kCloseBold;
        }
    }

    return tip;
}

Snippet::ParsedSnippet Snippet::parse(const QString &snippet)
{
    Snippet::ParsedSnippet result;
    result.success = true;

    const int count = snippet.count();
    bool success = true;
    int start = -1;

    result.text.reserve(count);

    for (int i = 0; i < count; ++i) {
        QChar current = snippet.at(i);
        QChar next = (i + 1) < count ? snippet.at(i + 1) : QChar();

        if (current == QLatin1Char('\\')) {
            if (next.isNull()) {
                success = false;
                break;
            }
            result.text.append(next);
            ++i;
            continue;
        }

        if (current == Snippet::kVariableDelimiter) {
            if (start < 0) {
                // start delimiter:
                start = result.text.count();
            } else {
                result.ranges << ParsedSnippet::Range(start, result.text.count() - start);
                start = -1;
            }
            continue;
        }
        result.text.append(current);
    }

    if (start >= 0)
        success = false;

    result.success = success;

    if (!success) {
        result.ranges.clear();
        result.text = snippet;
    }

    return result;
}

#ifdef WITH_TESTS
#   include <QTest>

#   include "texteditorplugin.h"

void Internal::TextEditorPlugin::testSnippetParsing_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("success");
    QTest::addColumn<QList<int> >("ranges_start");
    QTest::addColumn<QList<int> >("ranges_length");

    QTest::newRow("no input")
            << QString() << QString() << true << (QList<int>()) << (QList<int>());
    QTest::newRow("empty input")
            << QString::fromLatin1("") << QString::fromLatin1("") << true << (QList<int>()) << (QList<int>());

    QTest::newRow("simple identifier")
            << QString::fromLatin1("$test$") << QString::fromLatin1("test") << true
            << (QList<int>() << 0) << (QList<int>() << 4);
    QTest::newRow("escaped string")
            << QString::fromLatin1("\\$test\\$") << QString::fromLatin1("$test$") << true
            << (QList<int>()) << (QList<int>());
    QTest::newRow("escaped escape")
            << QString::fromLatin1("\\\\$test\\\\$") << QString::fromLatin1("\\test\\") << true
            << (QList<int>() << 1) << (QList<int>() << 5);

    QTest::newRow("Q_PROPERTY")
            << QString::fromLatin1("Q_PROPERTY($type$ $name$ READ $name$ WRITE set$name$ NOTIFY $name$Changed)")
            << QString::fromLatin1("Q_PROPERTY(type name READ name WRITE setname NOTIFY nameChanged)") << true
            << (QList<int>() << 11 << 16 << 26 << 40 << 52)
            << (QList<int>() << 4 << 4 << 4 << 4 << 4);

    QTest::newRow("broken escape")
            << QString::fromLatin1("\\\\$test\\\\$\\") << QString::fromLatin1("\\\\$test\\\\$\\") << false
            << (QList<int>()) << (QList<int>());
    QTest::newRow("open identifier")
            << QString::fromLatin1("$test") << QString::fromLatin1("$test") << false
            << (QList<int>()) << (QList<int>());
}

void Internal::TextEditorPlugin::testSnippetParsing()
{
    QFETCH(QString, input);
    QFETCH(QString, text);
    QFETCH(bool, success);
    QFETCH(QList<int>, ranges_start);
    QFETCH(QList<int>, ranges_length);
    Q_ASSERT(ranges_start.count() == ranges_length.count()); // sanity check for the test data

    Snippet::ParsedSnippet result = Snippet::parse(input);

    QCOMPARE(result.text, text);
    QCOMPARE(result.success, success);
    QCOMPARE(result.ranges.count(), ranges_start.count());
    for (int i = 0; i < ranges_start.count(); ++i) {
        QCOMPARE(result.ranges.at(i).start, ranges_start.at(i));
        QCOMPARE(result.ranges.at(i).length, ranges_length.at(i));
    }
}
#endif

