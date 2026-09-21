// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/algorithm.h>
#include <utils/spellchecker.h>

#include <QTest>

//TESTED_COMPONENT=src/libs/utils

using namespace Utils;

class tst_SpellChecker : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testWordRanges_data();
    void testWordRanges();
    void testMisspelledWords_data();
    void testMisspelledWords();
    void testMisspelledWordsInProse_data();
    void testMisspelledWordsInProse();
    void testOnlyProseReachesTheDictionary_data();
    void testOnlyProseReachesTheDictionary();
    void testSuggestions();

private:
    QStringList misspelledWords(const QString &text) const;

    QString m_language;
};

void tst_SpellChecker::initTestCase()
{
    const QStringList languages = SpellChecker::instance()->availableLanguages();
    m_language = Utils::findOrDefault(languages, [](const QString &language) {
        return language.startsWith("en");
    });
}

static QStringList wordsAt(const QString &text, const QList<SpellChecker::Range> &ranges)
{
    return Utils::transform<QStringList>(ranges, [&text](const SpellChecker::Range &range) {
        return text.mid(range.start, range.length);
    });
}

QStringList tst_SpellChecker::misspelledWords(const QString &text) const
{
    return wordsAt(text, SpellChecker::instance()->misspelledRanges(text, m_language));
}

void tst_SpellChecker::testWordRanges_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("empty") << QString() << QStringList();
    QTest::newRow("punctuation") << "-> ()" << QStringList();
    QTest::newRow("sentence") << "A mispelled word" << QStringList{"A", "mispelled", "word"};
    QTest::newRow("start of text") << "mispelled at the start"
                                   << QStringList{"mispelled", "at", "the", "start"};
    QTest::newRow("sentence end") << "A mispelled word." << QStringList{"A", "mispelled", "word"};
    QTest::newRow("apostrophe") << "It doesn't matter" << QStringList{"It", "doesn't", "matter"};
    QTest::newRow("identifier") << "The mispelled_word and camelCase"
                                << QStringList{"The", "mispelled", "word", "and", "camelCase"};
    QTest::newRow("file name") << "Add mainwindow.cpp here"
                               << QStringList{"Add", "mainwindow.cpp", "here"};
    QTest::newRow("path") << "src/libs/spellcheckr.cpp"
                          << QStringList{"src", "libs", "spellcheckr.cpp"};
    QTest::newRow("numbers") << "Qt6 and 12345" << QStringList{"Qt6", "and"};
}

void tst_SpellChecker::testWordRanges()
{
    QFETCH(QString, text);
    QFETCH(QStringList, expected);

    QCOMPARE(wordsAt(text, SpellChecker::wordRanges(text)), expected);
}

void tst_SpellChecker::testMisspelledWords_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("correct") << "This sentence is spelled correctly" << QStringList();
    QTest::newRow("typo") << "This sentence has a mispelled word" << QStringList{"mispelled"};
    QTest::newRow("two typos") << "A mispelled and anoter word"
                               << QStringList{"mispelled", "anoter"};
    QTest::newRow("identifier") << "Call mispelledWord to fix it" << QStringList();
    QTest::newRow("acronym") << "The ABI and MISPELLED word" << QStringList();
    QTest::newRow("path") << "Fix src/libs/utils/spellcheckr.cpp" << QStringList();
    QTest::newRow("file name") << "Add mainwindow.cpp and widget.ui" << QStringList();
    QTest::newRow("file names") << "Rename oldfile.txt to newfile.txt" << QStringList();
    QTest::newRow("sentence end") << "This sentence has a mispelled word."
                                  << QStringList{"mispelled"};
    QTest::newRow("url") << "See https://example.com/nonsensz for details" << QStringList();
    QTest::newRow("trailer") << "Task-number: QTCREATORBUG-12345" << QStringList();
}

void tst_SpellChecker::testMisspelledWords()
{
    if (m_language.isEmpty())
        QSKIP("No spell checking service or no English dictionary is installed");

    QFETCH(QString, text);
    QFETCH(QStringList, expected);

    QCOMPARE(misspelledWords(text), expected);
}

void tst_SpellChecker::testMisspelledWordsInProse_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QList<SpellChecker::Range>>("prose");
    QTest::addColumn<QStringList>("expected");

    // Only the prose of the text reaches the dictionary, so the typo in the code is
    // none of its business, though nothing about the word itself says so.
    QTest::newRow("comment of a line of code")
        << "set(mispeled 1) # A mispeled comment" << QList<SpellChecker::Range>{{16, 20}}
        << QStringList{"mispeled"};

    // What makes a word a token of code is the character next to it, which sits
    // outside the prose the word was found in.
    QTest::newRow("word glued to code before the prose")
        << "path/mispeled" << QList<SpellChecker::Range>{{5, 8}} << QStringList();
    QTest::newRow("word glued to code after the prose")
        << "mispeled/path" << QList<SpellChecker::Range>{{0, 8}} << QStringList();

    // A word only half of which is prose is no word the dictionary was asked about.
    QTest::newRow("word reaching out of the prose")
        << "A mispeled word" << QList<SpellChecker::Range>{{5, 10}} << QStringList();

    QTest::newRow("no prose at all")
        << "A mispeled word" << QList<SpellChecker::Range>() << QStringList();
}

void tst_SpellChecker::testMisspelledWordsInProse()
{
    if (m_language.isEmpty())
        QSKIP("No spell checking service or no English dictionary is installed");

    QFETCH(QString, text);
    QFETCH(QList<SpellChecker::Range>, prose);
    QFETCH(QStringList, expected);

    const QList<SpellChecker::Range> ranges
        = SpellChecker::instance()->misspelledRanges(text, prose, m_language);
    QCOMPARE(wordsAt(text, ranges), expected);
}

// Records what the dictionary is asked about, which is what a text costs: a call into
// the spell checking service of the platform is the expensive part of a check.
class RecordingSpellChecker : public SpellChecker
{
public:
    mutable QStringList checked;

protected:
    QList<Range> check(const QString &text, const QString &language) const override
    {
        Q_UNUSED(language)
        checked.append(text);
        return {};
    }
};

void tst_SpellChecker::testOnlyProseReachesTheDictionary_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QList<SpellChecker::Range>>("prose");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("comment of a line of code")
        << "set(mispeled 1) # A mispeled comment" << QList<SpellChecker::Range>{{16, 20}}
        << QStringList{"# A mispeled comment"};

    // One call the dictionary answers about more than it has to beats one call per
    // range: what a call costs is barely less for a shorter text.
    QTest::newRow("several ranges of one text")
        << "a mispeled b mistaeken c" << QList<SpellChecker::Range>{{2, 8}, {13, 9}}
        << QStringList{"mispeled b mistaeken"};

    QTest::newRow("nothing but prose")
        << "A mispeled word" << QList<SpellChecker::Range>{{0, 15}}
        << QStringList{"A mispeled word"};

    QTest::newRow("no prose at all")
        << "A mispeled word" << QList<SpellChecker::Range>() << QStringList();

    QTest::newRow("half a word") << "A mispeled word" << QList<SpellChecker::Range>{{5, 4}}
                                 << QStringList();
}

void tst_SpellChecker::testOnlyProseReachesTheDictionary()
{
    QFETCH(QString, text);
    QFETCH(QList<SpellChecker::Range>, prose);
    QFETCH(QStringList, expected);

    RecordingSpellChecker checker;
    checker.misspelledRanges(text, prose, "en_US");
    QCOMPARE(checker.checked, expected);
}

void tst_SpellChecker::testSuggestions()
{
    if (m_language.isEmpty())
        QSKIP("No spell checking service or no English dictionary is installed");

    const QStringList suggestions = SpellChecker::instance()->suggestions("mispelled", m_language);
    QVERIFY(!suggestions.isEmpty());
}

QTEST_GUILESS_MAIN(tst_SpellChecker)

#include "tst_spellchecker.moc"
