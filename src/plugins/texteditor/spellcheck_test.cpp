// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "spellcheck_test.h"

#include "fontsettings.h"
#include "syntaxhighlighter.h"

#include <utils/algorithm.h>
#include <utils/spellchecker.h>

#include <QTest>
#include <QTextBlock>
#include <QTextDocument>

#include <memory>

namespace TextEditor::Internal {

class SpellCheckingHighlighter final : public SyntaxHighlighter
{
public:
    explicit SpellCheckingHighlighter(QTextDocument *document)
        : SyntaxHighlighter(document)
    {}

    void highlightBlock(const QString &text) final { spellCheck(text); }
};

// Each test function guards itself instead of initTestCase() doing it once for all of
// them: a QSKIP in initTestCase() cancels every suite that qExec() runs after this one.
#define REQUIRE_SPELL_CHECKING() \
    do { \
        const QString reason = setUpSpellChecking(); \
        if (!reason.isEmpty()) \
            QSKIP(qPrintable(reason)); \
        QVERIFY(spellErrorFormat().underlineStyle() != QTextCharFormat::NoUnderline); \
    } while (false)

// Exercises SyntaxHighlighter::spellCheck() against the spell checking service
// of the operating system.
class SpellCheckTest final : public QObject
{
    Q_OBJECT

private slots:
    void testMisspelledWordIsMarked()
    {
        REQUIRE_SPELL_CHECKING();
        setUpDocument("This sentence has a mispelled word");
        QCOMPARE(underlinedTexts(), QStringList{"mispelled"});
    }

    void testCodeIsNotMarked()
    {
        REQUIRE_SPELL_CHECKING();
        setUpDocument("Rename mispelledFunction in src/libs/utils/spellcheckr.cpp");
        QCOMPARE(underlinedTexts(), QStringList());
    }

    void testWordUnderCursorIsNotMarked()
    {
        REQUIRE_SPELL_CHECKING();
        const QString text = "This sentence has a mispelled word";
        setUpDocument(text);
        m_highlighter->setSpellCheckCursorPosition(text.indexOf("mispelled") + 3);
        QCOMPARE(underlinedTexts(), QStringList());

        m_highlighter->setSpellCheckCursorPosition(-1);
        QCOMPARE(underlinedTexts(), QStringList{"mispelled"});
    }

    void cleanup() { m_document.reset(); }

private:
    static QTextCharFormat spellErrorFormat()
    {
        return globalFontSettings().data().toTextCharFormat(C_SPELL_ERROR);
    }

    // The reason there is nothing to test against, empty when there is something.
    QString setUpSpellChecking()
    {
        Utils::SpellChecker *checker = Utils::SpellChecker::instance();
        if (!checker->isAvailable())
            return "This operating system provides no spell checking service";

        m_language = Utils::findOrDefault(checker->availableLanguages(),
                                          [](const QString &language) {
                                              return language.startsWith("en");
                                          });
        if (m_language.isEmpty())
            return "No English dictionary is installed";
        return {};
    }

    void setUpDocument(const QString &text)
    {
        m_document = std::make_unique<QTextDocument>(text);
        m_highlighter = new SpellCheckingHighlighter(m_document.get());
        m_highlighter->setFontSettings(globalFontSettings().data());
        m_highlighter->setSpellCheckLanguage(m_language);
    }

    QStringList underlinedTexts() const
    {
        const QTextCharFormat format = spellErrorFormat();
        QStringList texts;
        for (QTextBlock block = m_document->firstBlock(); block.isValid(); block = block.next()) {
            const QList<QTextLayout::FormatRange> ranges = block.layout()->formats();
            for (const QTextLayout::FormatRange &range : ranges) {
                if (range.format.underlineStyle() == format.underlineStyle()
                    && range.format.underlineColor() == format.underlineColor()) {
                    texts.append(block.text().mid(range.start, range.length));
                }
            }
        }
        return texts;
    }

    QString m_language;
    std::unique_ptr<QTextDocument> m_document;
    SpellCheckingHighlighter *m_highlighter = nullptr;
};

#undef REQUIRE_SPELL_CHECKING

QObject *createSpellCheckTest()
{
    return new SpellCheckTest;
}

} // namespace TextEditor::Internal

#include "spellcheck_test.moc"
