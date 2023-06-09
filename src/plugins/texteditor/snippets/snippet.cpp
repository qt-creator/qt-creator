// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "snippet.h"

#include "../texteditortr.h"

#include <utils/algorithm.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/templateengine.h>

#include <QTextDocument>

namespace TextEditor {

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
    Utils::Id id() const final { return UCMANGLER_ID; }
    QString mangle(const QString &unmangled) const final { return unmangled.toUpper(); }
};

class LowercaseMangler : public NameMangler
{
public:
    Utils::Id id() const final { return LCMANGLER_ID; }
    QString mangle(const QString &unmangled) const final { return unmangled.toLower(); }
};

class TitlecaseMangler : public NameMangler
{
public:
    Utils::Id id() const final { return TCMANGLER_ID; }
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

Snippet::~Snippet() = default;

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

bool Snippet::isValidTrigger(const QString &trigger)
{
    if (trigger.isEmpty() || trigger.at(0).isNumber())
        return false;
    return Utils::allOf(trigger, [](const QChar &c) { return c.isLetterOrNumber() || c == '_'; });
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

static QString tipPart(const ParsedSnippet::Part &part)
{
    static const char kOpenBold[] = "<b>";
    static const char kCloseBold[] = "</b>";
    static const QHash<QChar, QString> replacements = {{'\n', "<br>"},
                                                       {' ', "&nbsp;"},
                                                       {'"', "&quot;"},
                                                       {'&', "&amp;"},
                                                       {'<', "&lt;"},
                                                       {'>', "&gt;"}};

    QString text;
    text.reserve(part.text.size());

    for (const QChar &c : part.text)
        text.append(replacements.value(c, c));

    if (part.variableIndex >= 0)
        text = kOpenBold + (text.isEmpty() ? QString("...") : part.text) + kCloseBold;

    return text;
}

QString Snippet::generateTip() const
{
    SnippetParseResult result = Snippet::parse(m_content);

    if (std::holds_alternative<SnippetParseError>(result))
        return std::get<SnippetParseError>(result).htmlMessage();
    QTC_ASSERT(std::holds_alternative<ParsedSnippet>(result), return {});
    const ParsedSnippet parsedSnippet = std::get<ParsedSnippet>(result);

    QString tip("<nobr>");
    for (const ParsedSnippet::Part &part : parsedSnippet.parts)
        tip.append(tipPart(part));
    return tip;
}

SnippetParseResult Snippet::parse(const QString &snippet)
{
    static UppercaseMangler ucMangler;
    static LowercaseMangler lcMangler;
    static TitlecaseMangler tcMangler;

    ParsedSnippet result;

    QString errorMessage;
    QString preprocessedSnippet
            = Utils::TemplateEngine::processText(Utils::globalMacroExpander(), snippet,
                                                 &errorMessage);

    if (!errorMessage.isEmpty())
        return {SnippetParseError{errorMessage, {}, -1}};

    const int count = preprocessedSnippet.size();
    NameMangler *mangler = nullptr;

    QMap<QString, int> variableIndexes;
    bool inVar = false;

    ParsedSnippet::Part currentPart;

    for (int i = 0; i < count; ++i) {
        QChar current = preprocessedSnippet.at(i);

        if (current == Snippet::kVariableDelimiter) {
            if (inVar) {
                const QString variable = currentPart.text;
                const int index = variableIndexes.value(currentPart.text, result.variables.size());
                if (index == result.variables.size()) {
                    variableIndexes[variable] = index;
                    result.variables.append(QList<int>());
                }
                currentPart.variableIndex = index;
                currentPart.mangler = mangler;
                mangler = nullptr;
                result.variables[index] << result.parts.size() - 1;
            } else if (currentPart.text.isEmpty()) {
                inVar = !inVar;
                continue;
            }
            result.parts << currentPart;
            currentPart = ParsedSnippet::Part();
            inVar = !inVar;
            continue;
        }

        if (mangler) {
            return SnippetParseResult{SnippetParseError{Tr::tr("Expected delimiter after mangler ID."),
                                                        preprocessedSnippet,
                                                        i}};
        }

        if (current == ':' && inVar) {
            QChar next = (i + 1) < count ? preprocessedSnippet.at(i + 1) : QChar();
            if (next == 'l') {
                mangler = &lcMangler;
            } else if (next == 'u') {
                mangler = &ucMangler;
            } else if (next == 'c') {
                mangler = &tcMangler;
            } else {
                return SnippetParseResult{
                    SnippetParseError{Tr::tr("Expected mangler ID \"l\" (lowercase), \"u\" (uppercase), "
                                         "or \"c\" (titlecase) after colon."),
                                      preprocessedSnippet,
                                      i}};
            }
            ++i;
            continue;
        }

        if (current == kEscapeChar){
            QChar next = (i + 1) < count ? preprocessedSnippet.at(i + 1) : QChar();
            if (next == kEscapeChar || next == kVariableDelimiter) {
                currentPart.text.append(next);
                ++i;
                continue;
            }
        }

        currentPart.text.append(current);
    }

    if (inVar) {
        return SnippetParseResult{
            SnippetParseError{Tr::tr("Missing closing variable delimiter for:"), currentPart.text, 0}};
    }

    if (!currentPart.text.isEmpty())
        result.parts << currentPart;

    return SnippetParseResult(result);
}

} // Texteditor

using namespace TextEditor;

#ifdef WITH_TESTS
#   include <QTest>

#   include "../texteditorplugin.h"

const char NOMANGLER_ID[] = "TextEditor::NoMangler";

struct SnippetPart
{
    SnippetPart() = default;
    explicit SnippetPart(const QString &text,
                         int index = -1,
                         const Utils::Id &manglerId = NOMANGLER_ID)
        : text(text)
        , variableIndex(index)
        , manglerId(manglerId)
    {}
    QString text;
    int variableIndex = -1; // if variable index is >= 0 the text is interpreted as a variable
    Utils::Id manglerId;
};
Q_DECLARE_METATYPE(SnippetPart);

using Parts = QList<SnippetPart>;

void Internal::TextEditorPlugin::testSnippetParsing_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("success");
    QTest::addColumn<Parts>("parts");

    QTest::newRow("no input") << QString() << true << Parts();
    QTest::newRow("empty input") << QString("") << true << Parts();
    QTest::newRow("newline only") << QString("\n") << true << Parts{SnippetPart("\n")};

    QTest::newRow("simple identifier")
        << QString("$tESt$") << true << Parts{SnippetPart("tESt", 0)};
    QTest::newRow("simple identifier with lc")
        << QString("$tESt:l$") << true << Parts{SnippetPart("tESt", 0, LCMANGLER_ID)};
    QTest::newRow("simple identifier with uc")
        << QString("$tESt:u$") << true << Parts{SnippetPart("tESt", 0, UCMANGLER_ID)};
    QTest::newRow("simple identifier with tc")
        << QString("$tESt:c$") << true << Parts{SnippetPart("tESt", 0, TCMANGLER_ID)};

    QTest::newRow("escaped string")
        << QString("\\\\$test\\\\$") << true << Parts{SnippetPart("$test$")};
    QTest::newRow("escaped escape") << QString("\\\\\\\\$test$\\\\\\\\") << true
                                    << Parts{
                                           SnippetPart("\\"),
                                           SnippetPart("test", 0),
                                           SnippetPart("\\"),
                                       };
    QTest::newRow("broken escape")
        << QString::fromLatin1("\\\\$test\\\\\\\\$\\\\") << false << Parts();

    QTest::newRow("Q_PROPERTY") << QString(
        "Q_PROPERTY($type$ $name$ READ $name$ WRITE set$name:c$ NOTIFY $name$Changed FINAL)")
                                << true
                                << Parts{SnippetPart("Q_PROPERTY("),
                                         SnippetPart("type", 0),
                                         SnippetPart(" "),
                                         SnippetPart("name", 1),
                                         SnippetPart(" READ "),
                                         SnippetPart("name", 1),
                                         SnippetPart(" WRITE set"),
                                         SnippetPart("name", 1, TCMANGLER_ID),
                                         SnippetPart(" NOTIFY "),
                                         SnippetPart("name", 1),
                                         SnippetPart("Changed FINAL)")};

    QTest::newRow("open identifier") << QString("$test") << false << Parts();
    QTest::newRow("wrong mangler") << QString("$test:X$") << false << Parts();

    QTest::newRow("multiline with :") << QString("class $name$\n"
                                                 "{\n"
                                                 "public:\n"
                                                 "    $name$() {}\n"
                                                 "};")
                                      << true
                                      << Parts{
                                             SnippetPart("class "),
                                             SnippetPart("name", 0),
                                             SnippetPart("\n"
                                                         "{\n"
                                                         "public:\n"
                                                         "    "),
                                             SnippetPart("name", 0),
                                             SnippetPart("() {}\n"
                                                         "};"),
                                         };

    QTest::newRow("escape sequences") << QString("class $name$\\n"
                                                 "{\\n"
                                                 "public\\\\:\\n"
                                                 "\\t$name$() {}\\n"
                                                 "};")
                                      << true
                                      << Parts{
                                             SnippetPart("class "),
                                             SnippetPart("name", 0),
                                             SnippetPart("\n"
                                                         "{\n"
                                                         "public\\:\n"
                                                         "\t"),
                                             SnippetPart("name", 0),
                                             SnippetPart("() {}\n"
                                                         "};"),
                                         };
}

void Internal::TextEditorPlugin::testSnippetParsing()
{
    QFETCH(QString, input);
    QFETCH(bool, success);
    QFETCH(Parts, parts);

    SnippetParseResult result = Snippet::parse(input);
    QCOMPARE(std::holds_alternative<ParsedSnippet>(result), success);
    if (!success)
        return;

    ParsedSnippet snippet = std::get<ParsedSnippet>(result);

    auto rangesCompare = [&](const ParsedSnippet::Part &actual, const SnippetPart &expected) {
        QCOMPARE(actual.text, expected.text);
        QCOMPARE(actual.variableIndex, expected.variableIndex);
        auto manglerId = actual.mangler ? actual.mangler->id() : NOMANGLER_ID;
        QCOMPARE(manglerId, expected.manglerId);
    };

    for (int i = 0; i < parts.size(); ++i)
        rangesCompare(snippet.parts.at(i), parts.at(i));
}
#endif
