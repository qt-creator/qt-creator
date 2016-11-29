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

#include "snippet.h"

#include <coreplugin/id.h>

#include <utils/templateengine.h>

#include <QLatin1Char>
#include <QLatin1String>
#include <QTextDocument>

using namespace TextEditor;

const char UCMANGLER_ID[] = "TextEditor::UppercaseMangler";
const char LCMANGLER_ID[] = "TextEditor::LowercaseMangler";
const char TCMANGLER_ID[] = "TextEditor::TitlecaseMangler";

// --------------------------------------------------------------------
// Manglers:
// --------------------------------------------------------------------

NameMangler::~NameMangler() = default;

class UppercaseMangler : public NameMangler
{
public:
    Core::Id id() const final { return UCMANGLER_ID; }
    QString mangle(const QString &unmangled) const final { return unmangled.toUpper(); }
};

class LowercaseMangler : public NameMangler
{
public:
    Core::Id id() const final { return LCMANGLER_ID; }
    QString mangle(const QString &unmangled) const final { return unmangled.toLower(); }
};

class TitlecaseMangler : public NameMangler
{
public:
    Core::Id id() const final { return TCMANGLER_ID; }
    QString mangle(const QString &unmangled) const final
    {
        QString result = unmangled;
        if (!result.isEmpty())
            result[0] = unmangled.at(0).toTitleCase();
        return result;
    }
};

// --------------------------------------------------------------------
// Snippet:
// --------------------------------------------------------------------

const QChar Snippet::kVariableDelimiter(QLatin1Char('$'));
const QChar Snippet::kEscapeChar(QLatin1Char('\\'));

Snippet::Snippet(const QString &groupId, const QString &id) : m_groupId(groupId), m_id(id)
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

    QString escapedContent(m_content.toHtmlEscaped());
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
    static UppercaseMangler ucMangler;
    static LowercaseMangler lcMangler;
    static TitlecaseMangler tcMangler;

    Snippet::ParsedSnippet result;

    QString errorMessage;
    QString preprocessedSnippet
            = Utils::TemplateEngine::processText(Utils::globalMacroExpander(), snippet,
                                                 &errorMessage);

    result.success = errorMessage.isEmpty();
    if (!result.success) {
        result.text = snippet;
        result.errorMessage = errorMessage;
        return result;
    }

    const int count = preprocessedSnippet.count();
    bool success = true;
    int start = -1;
    NameMangler *mangler = nullptr;

    result.text.reserve(count);

    for (int i = 0; i < count; ++i) {
        QChar current = preprocessedSnippet.at(i);
        QChar next = (i + 1) < count ? preprocessedSnippet.at(i + 1) : QChar();

        if (current == Snippet::kVariableDelimiter) {
            if (start < 0) {
                // start delimiter:
                start = result.text.count();
            } else {
                int length = result.text.count() - start;
                result.ranges << ParsedSnippet::Range(start, length, mangler);
                mangler = 0;
                start = -1;
            }
            continue;
        }

        if (mangler) {
            success = false;
            break;
        }

        if (current == QLatin1Char(':') && start >= 0) {
            if (next == QLatin1Char('l')) {
                mangler = &lcMangler;
            } else if (next == QLatin1Char('u')) {
                mangler = &ucMangler;
            } else if (next == QLatin1Char('c')) {
                mangler = &tcMangler;
            } else {
                success = false;
                break;
            }
            ++i;
            continue;
        }

        if (current == kEscapeChar && (next == kEscapeChar || next == kVariableDelimiter)) {
            result.text.append(next);
            ++i;
            continue;
        }

        result.text.append(current);
    }

    if (start >= 0)
        success = false;

    result.success = success;

    if (!success) {
        result.ranges.clear();
        result.text = preprocessedSnippet;
    }

    return result;
}

#ifdef WITH_TESTS
#   include <QTest>

#   include "../texteditorplugin.h"

const char NOMANGLER_ID[] = "TextEditor::NoMangler";

void Internal::TextEditorPlugin::testSnippetParsing_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("success");
    QTest::addColumn<QList<int> >("ranges_start");
    QTest::addColumn<QList<int> >("ranges_length");
    QTest::addColumn<QList<Core::Id> >("ranges_mangler");

    QTest::newRow("no input")
            << QString() << QString() << true
            << (QList<int>()) << (QList<int>()) << (QList<Core::Id>());
    QTest::newRow("empty input")
            << QString::fromLatin1("") << QString::fromLatin1("") << true
            << (QList<int>()) << (QList<int>()) << (QList<Core::Id>());
    QTest::newRow("newline only")
            << QString::fromLatin1("\n") << QString::fromLatin1("\n") << true
            << (QList<int>()) << (QList<int>()) << (QList<Core::Id>());

    QTest::newRow("simple identifier")
            << QString::fromLatin1("$tESt$") << QString::fromLatin1("tESt") << true
            << (QList<int>() << 0) << (QList<int>() << 4)
            << (QList<Core::Id>() << NOMANGLER_ID);
    QTest::newRow("simple identifier with lc")
            << QString::fromLatin1("$tESt:l$") << QString::fromLatin1("tESt") << true
            << (QList<int>() << 0) << (QList<int>() << 4)
            << (QList<Core::Id>() << LCMANGLER_ID);
    QTest::newRow("simple identifier with uc")
            << QString::fromLatin1("$tESt:u$") << QString::fromLatin1("tESt") << true
            << (QList<int>() << 0) << (QList<int>() << 4)
            << (QList<Core::Id>() << UCMANGLER_ID);
    QTest::newRow("simple identifier with tc")
            << QString::fromLatin1("$tESt:c$") << QString::fromLatin1("tESt") << true
            << (QList<int>() << 0) << (QList<int>() << 4)
            << (QList<Core::Id>() << TCMANGLER_ID);

    QTest::newRow("escaped string")
            << QString::fromLatin1("\\\\$test\\\\$") << QString::fromLatin1("$test$") << true
            << (QList<int>()) << (QList<int>())
            << (QList<Core::Id>());
    QTest::newRow("escaped escape")
            << QString::fromLatin1("\\\\\\\\$test$\\\\\\\\") << QString::fromLatin1("\\test\\") << true
            << (QList<int>() << 1) << (QList<int>() << 4)
            << (QList<Core::Id>() << NOMANGLER_ID);
    QTest::newRow("broken escape")
            << QString::fromLatin1("\\\\$test\\\\\\\\$\\\\") << QString::fromLatin1("\\$test\\\\$\\") << false
            << (QList<int>()) << (QList<int>())
            << (QList<Core::Id>());

    QTest::newRow("Q_PROPERTY")
            << QString::fromLatin1("Q_PROPERTY($type$ $name$ READ $name$ WRITE set$name:c$ NOTIFY $name$Changed)")
            << QString::fromLatin1("Q_PROPERTY(type name READ name WRITE setname NOTIFY nameChanged)") << true
            << (QList<int>() << 11 << 16 << 26 << 40 << 52)
            << (QList<int>() << 4 << 4 << 4 << 4 << 4)
            << (QList<Core::Id>() << NOMANGLER_ID << NOMANGLER_ID << NOMANGLER_ID << TCMANGLER_ID << NOMANGLER_ID);

    QTest::newRow("open identifier")
            << QString::fromLatin1("$test") << QString::fromLatin1("$test") << false
            << (QList<int>()) << (QList<int>())
            << (QList<Core::Id>());
    QTest::newRow("wrong mangler")
            << QString::fromLatin1("$test:X$") << QString::fromLatin1("$test:X$") << false
            << (QList<int>()) << (QList<int>())
            << (QList<Core::Id>());

    QTest::newRow("multiline with :")
            << QString::fromLatin1("class $name$\n"
                                   "{\n"
                                   "public:\n"
                                   "    $name$() {}\n"
                                   "};")
            << QString::fromLatin1("class name\n"
                                   "{\n"
                                   "public:\n"
                                   "    name() {}\n"
                                   "};")
            << true
            << (QList<int>() << 6 << 25)
            << (QList<int>() << 4 << 4)
            << (QList<Core::Id>() << NOMANGLER_ID << NOMANGLER_ID);

    QTest::newRow("escape sequences")
            << QString::fromLatin1("class $name$\\n"
                                   "{\\n"
                                   "public\\\\:\\n"
                                   "\\t$name$() {}\\n"
                                   "};")
            << QString::fromLatin1("class name\n"
                                   "{\n"
                                   "public\\:\n"
                                   "\tname() {}\n"
                                   "};")
            << true
            << (QList<int>() << 6 << 23)
            << (QList<int>() << 4 << 4)
            << (QList<Core::Id>() << NOMANGLER_ID << NOMANGLER_ID);

}

void Internal::TextEditorPlugin::testSnippetParsing()
{
    QFETCH(QString, input);
    QFETCH(QString, text);
    QFETCH(bool, success);
    QFETCH(QList<int>, ranges_start);
    QFETCH(QList<int>, ranges_length);
    QFETCH(QList<Core::Id>, ranges_mangler);
    Q_ASSERT(ranges_start.count() == ranges_length.count()); // sanity check for the test data
    Q_ASSERT(ranges_start.count() == ranges_mangler.count()); // sanity check for the test data

    Snippet::ParsedSnippet result = Snippet::parse(input);

    QCOMPARE(result.text, text);
    QCOMPARE(result.success, success);
    QCOMPARE(result.ranges.count(), ranges_start.count());
    for (int i = 0; i < ranges_start.count(); ++i) {
        QCOMPARE(result.ranges.at(i).start, ranges_start.at(i));
        QCOMPARE(result.ranges.at(i).length, ranges_length.at(i));
        Core::Id id = NOMANGLER_ID;
        if (result.ranges.at(i).mangler)
            id = result.ranges.at(i).mangler->id();
        QCOMPARE(id, ranges_mangler.at(i));
    }
}
#endif

