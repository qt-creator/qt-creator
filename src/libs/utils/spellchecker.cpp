// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "spellchecker.h"

#include "algorithm.h"

namespace Utils {

static bool isTokenGlue(QChar c)
{
    static const QStringView glue = u"_/\\@#$%&*+=<>~|`\"";
    return c.isDigit() || glue.contains(c);
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

SpellChecker *SpellChecker::instance()
{
    static SpellChecker *checker = []() -> SpellChecker * {
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
        if (SpellChecker *native = createNativeSpellChecker())
            return native;
#endif
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
