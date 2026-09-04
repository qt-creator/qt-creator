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
    void testMisspelledWords_data();
    void testMisspelledWords();
    void testSuggestions();

private:
    QStringList misspelledWords(const QString &text) const;

    QString m_language;
};

void tst_SpellChecker::initTestCase()
{
    SpellChecker *checker = SpellChecker::instance();
    if (!checker->isAvailable())
        QSKIP("This operating system provides no spell checking service");

    m_language = Utils::findOrDefault(checker->availableLanguages(), [](const QString &language) {
        return language.startsWith("en");
    });
    if (m_language.isEmpty())
        QSKIP("No English dictionary is installed");
}

QStringList tst_SpellChecker::misspelledWords(const QString &text) const
{
    const QList<SpellChecker::Range> ranges
        = SpellChecker::instance()->misspelledRanges(text, m_language);
    return Utils::transform<QStringList>(ranges, [&text](const SpellChecker::Range &range) {
        return text.mid(range.start, range.length);
    });
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
    QFETCH(QString, text);
    QFETCH(QStringList, expected);

    QCOMPARE(misspelledWords(text), expected);
}

void tst_SpellChecker::testSuggestions()
{
    const QStringList suggestions = SpellChecker::instance()->suggestions("mispelled", m_language);
    QVERIFY(!suggestions.isEmpty());
}

QTEST_GUILESS_MAIN(tst_SpellChecker)

#include "tst_spellchecker.moc"
