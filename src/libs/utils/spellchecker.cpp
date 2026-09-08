// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "spellchecker.h"

#include "algorithm.h"

#include <QTextBoundaryFinder>

namespace Utils {

static bool isGlue(QChar c)
{
    static const QStringView glue = u"_/\\@#$%&*+=<>~|`\"";
    return glue.contains(c);
}

static bool isTokenGlue(QChar c)
{
    return c.isDigit() || isGlue(c);
}

// Prose is what a spell checker has something to say about. Identifiers, paths,
// URLs, options and hashes end up in the same text, and the dictionary has no
// opinion on them worth showing.
static bool isCode(const QString &text, const SpellChecker::Range &range)
{
    const QStringView word = QStringView(text).mid(range.start, range.length);
    if (word.size() < 3 || word.first().isDigit())
        return true;

    for (int i = 1; i < word.size(); ++i) {
        const QChar c = word.at(i);
        if (c.isUpper() || c.isDigit() || c == '.')
            return true;
    }

    const QChar before = range.start > 0 ? text.at(range.start - 1) : QChar::Space;
    const int end = range.start + range.length;
    const QChar after = end < text.size() ? text.at(end) : QChar::Space;
    return isTokenGlue(before) || before == '.' || before == ':'
           || isTokenGlue(after) || after == ':';
}

static bool hasLetter(QStringView word)
{
    return Utils::anyOf(word, [](QChar c) { return c.isLetter(); });
}

static void appendWord(QList<SpellChecker::Range> *ranges, const QString &text, int start,
                       int length)
{
    if (!hasLetter(QStringView(text).mid(start, length)))
        return;

    // A file name or a domain reaches the dictionary as one token, the way the
    // spell checking services of the other platforms hand it over.
    if (!ranges->isEmpty()) {
        SpellChecker::Range &last = ranges->last();
        const int dot = last.start + last.length;
        if (dot + 1 == start && text.at(dot) == '.') {
            last.length = start + length - last.start;
            return;
        }
    }
    ranges->append({start, length});
}

static void appendWords(QList<SpellChecker::Range> *ranges, const QString &text, int start, int end)
{
    for (int i = start, wordStart = start; i <= end; ++i) {
        if (i == end || isGlue(text.at(i))) {
            appendWord(ranges, text, wordStart, i - wordStart);
            wordStart = i + 1;
        }
    }
}

SpellChecker *SpellChecker::instance()
{
    static SpellChecker *checker = []() -> SpellChecker * {
        if (SpellChecker *native = createNativeSpellChecker())
            return native;
        return new SpellChecker;
    }();
    return checker;
}

bool SpellChecker::isAvailable() const
{
    return false;
}

QStringList SpellChecker::availableLanguages() const
{
    return {};
}

QString SpellChecker::defaultLanguage() const
{
    return {};
}

QList<SpellChecker::Range> SpellChecker::misspelledRanges(const QString &text,
                                                          const QString &language) const
{
    QList<Range> ranges = check(text, language);
    Utils::erase(ranges, [&text](const Range &range) { return isCode(text, range); });
    return ranges;
}

QList<SpellChecker::Range> SpellChecker::wordRanges(const QString &text)
{
    QList<Range> ranges;
    QTextBoundaryFinder finder(QTextBoundaryFinder::Word, text);
    int start = -1;
    do {
        const QTextBoundaryFinder::BoundaryReasons reasons = finder.boundaryReasons();
        if (start != -1 && (reasons & QTextBoundaryFinder::EndOfItem))
            appendWords(&ranges, text, start, finder.position());
        if (reasons & QTextBoundaryFinder::StartOfItem)
            start = finder.position();
        else if (reasons & QTextBoundaryFinder::EndOfItem)
            start = -1;
    } while (finder.toNextBoundary() != -1);
    return ranges;
}

QStringList SpellChecker::suggestions(const QString &word, const QString &language) const
{
    Q_UNUSED(word)
    Q_UNUSED(language)
    return {};
}

void SpellChecker::learnWord(const QString &word, const QString &language)
{
    Q_UNUSED(word)
    Q_UNUSED(language)
}

void SpellChecker::ignoreWord(const QString &word, const QString &language)
{
    Q_UNUSED(word)
    Q_UNUSED(language)
}

QList<SpellChecker::Range> SpellChecker::check(const QString &text, const QString &language) const
{
    Q_UNUSED(text)
    Q_UNUSED(language)
    return {};
}

} // namespace Utils
