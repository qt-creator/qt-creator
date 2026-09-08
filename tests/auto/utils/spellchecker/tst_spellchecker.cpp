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

void tst_SpellChecker::testSuggestions()
{
    if (m_language.isEmpty())
        QSKIP("No spell checking service or no English dictionary is installed");

    const QStringList suggestions = SpellChecker::instance()->suggestions("mispelled", m_language);
    QVERIFY(!suggestions.isEmpty());
}

QTEST_GUILESS_MAIN(tst_SpellChecker)

#include "tst_spellchecker.moc"
