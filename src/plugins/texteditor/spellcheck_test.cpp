// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "spellcheck_test.h"

#include "fontsettings.h"
#include "spellcheckmenu.h"
#include "spellchecksettings.h"
#include "syntaxhighlighter.h"
#include "textdocument.h"
#include "texteditortr.h"
#include "texteditor.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/algorithm.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/plaintextedit/texteditorlayout.h>
#include <utils/spellchecker.h>

#include <QAction>
#include <QMenu>
#include <QScopeGuard>
#include <QTest>
#include <QTextBlock>
#include <QTextDocument>

#include <memory>
#include <optional>

namespace TextEditor::Internal {

class SpellCheckingHighlighter final : public SyntaxHighlighter
{
public:
    explicit SpellCheckingHighlighter(QTextDocument *document)
        : SyntaxHighlighter(document)
    {}

    // The whole of a block is prose as long as no test says which parts of it are.
    void setProseRanges(const QList<Utils::SpellChecker::Range> &ranges)
    {
        m_proseRanges = ranges;
    }

    void highlightBlock(const QString &text) final
    {
        if (m_proseRanges) {
            for (const Utils::SpellChecker::Range &range : *m_proseRanges)
                addProseRange(range.start, range.length);
        } else {
            addProseRange(0, text.size());
        }
        spellCheck(text);
    }

private:
    std::optional<QList<Utils::SpellChecker::Range>> m_proseRanges;
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

    void testOnlyProseIsMarked()
    {
        REQUIRE_SPELL_CHECKING();
        const QString text = "A mispelled word and a mistaeken one";
        setUpDocument(text);
        const int prose = text.indexOf("and");
        m_highlighter->setProseRanges({{prose, int(text.size()) - prose}});
        m_highlighter->rehighlight();
        QCOMPARE(underlinedTexts(), QStringList{"mistaeken"});
    }

    void testWordReachingOutOfProseIsNotMarked()
    {
        REQUIRE_SPELL_CHECKING();
        const QString text = "A mispelled word";
        setUpDocument(text);
        const int prose = text.indexOf("spelled");
        m_highlighter->setProseRanges({{prose, int(text.size()) - prose}});
        m_highlighter->rehighlight();
        QCOMPARE(underlinedTexts(), QStringList());
    }

    // Asking the dictionary costs a call into the spell checking service of the
    // platform, too much to spend on a block nobody is looking at.
    void testOnlyBlocksAViewerShowsAreMarked()
    {
        REQUIRE_SPELL_CHECKING();
        setUpDocument("A mispelled word\nAnd a mistaeken one\n");

        QObject viewer;
        m_highlighter->setVisibleBlocks(&viewer, {{0, 0}});
        m_highlighter->rehighlight();
        QCOMPARE(underlinedTexts(), QStringList{"mispelled"});

        // Scrolling the second block into view is what brings its mark out. The first
        // one keeps the mark it has: a block is checked again when it is highlighted
        // again, and scrolling out of view is no reason to highlight one.
        m_highlighter->setVisibleBlocks(&viewer, {{1, 1}});
        QCOMPARE(underlinedTexts(), (QStringList{"mispelled", "mistaeken"}));
    }

    // A fold spans the numbers of the blocks it hides without taking up a row of the
    // viewport, so the range a viewer reports covers blocks that nobody is looking at.
    void testBlocksAFoldHidesAreNotMarked()
    {
        REQUIRE_SPELL_CHECKING();
        setUpDocument("A mispelled word\nAnd a mistaeken one\nAnd a mistuken one\n");

        QTextBlock folded = m_checkedDocument->findBlockByNumber(1);
        folded.setVisible(false);

        QObject viewer;
        m_highlighter->setVisibleBlocks(&viewer, {{0, 2}});
        m_highlighter->rehighlight();
        QCOMPARE(underlinedTexts(), (QStringList{"mispelled", "mistuken"}));

        // Opening the fold is what brings the mark of the block out, with the range the
        // viewer reports the same one as before.
        folded.setVisible(true);
        m_highlighter->setVisibleBlocks(&viewer, {{0, 2}});
        QCOMPARE(underlinedTexts(), (QStringList{"mispelled", "mistaeken", "mistuken"}));
    }

    // A viewer that leaves blocks out in the middle of what it shows reports a range
    // per run of the blocks it does show, so that the numbers of the ones in between
    // fall into no range of its.
    void testBlocksBetweenTheRunsAViewerShowsAreNotMarked()
    {
        REQUIRE_SPELL_CHECKING();
        setUpDocument("A mispelled word\nAnd a mistaeken one\nAnd a mistuken one\n");

        QObject viewer;
        m_highlighter->setVisibleBlocks(&viewer, {{0, 0}, {2, 2}});
        m_highlighter->rehighlight();
        QCOMPARE(underlinedTexts(), (QStringList{"mispelled", "mistuken"}));

        // Showing the block in between is what brings its mark out, which a viewer
        // says by reporting the one run its blocks now make up.
        m_highlighter->setVisibleBlocks(&viewer, {{0, 2}});
        QCOMPARE(underlinedTexts(), (QStringList{"mispelled", "mistaeken", "mistuken"}));
    }

    // The unchanged lines an inline diff collapses are hidden in the layout of the
    // editor that collapsed them and nowhere else, so the document other editors share
    // keeps showing them and QTextBlock::isVisible() calls such a block visible. What
    // the editor reports has to leave it out all the same.
    void testBlocksTheEditorCollapsesAreNotMarked()
    {
        REQUIRE_SPELL_CHECKING();
        QVERIFY(openEditor("A mispelled word\nAnd a mistaeken one\nAnd a mistuken one\n"));

        Utils::TextEditorLayout *layout = m_editor->editorWidget()->editorLayout();
        QVERIFY(layout);
        layout->setBlockVisibleInEditor(m_checkedDocument->findBlockByNumber(1), false);

        // Configuring a highlighter is what asks the editor which blocks it shows.
        m_editor->editorWidget()->configureGenericHighlighter(
            Utils::mimeTypeForName("text/plain"));
        SyntaxHighlighter *highlighter = m_editor->textDocument()->syntaxHighlighter();
        QVERIFY(highlighter);
        highlighter->setSpellCheckLanguage(m_language);
        QTRY_COMPARE(underlinedTexts(), (QStringList{"mispelled", "mistuken"}));
    }

    // Highlighting a block a viewer brought into view changes none of its text, so the
    // formats a highlighter set for its semantics stay over the words they were set on.
    void testExtraFormatsStayWhereTheyAreWhenABlockIsChecked()
    {
        REQUIRE_SPELL_CHECKING();
        setUpDocument("A mispelled word\nAnd a mistaeken one\n");

        QObject viewer;
        m_highlighter->setVisibleBlocks(&viewer, {{0, 0}});
        m_highlighter->rehighlight();
        QCOMPARE(underlinedTexts(), QStringList{"mispelled"});

        const QTextBlock unchecked = m_checkedDocument->findBlockByNumber(1);
        const QString word = "mistaeken";
        m_highlighter->setExtraFormats(unchecked,
                                       {{int(unchecked.text().indexOf(word)),
                                         int(word.size()),
                                         extraFormat()}});
        QCOMPARE(extraFormattedTexts(unchecked), QStringList{word});

        m_highlighter->setVisibleBlocks(&viewer, {{0, 1}});
        QCOMPARE(underlinedTexts(), (QStringList{"mispelled", "mistaeken"}));
        QCOMPARE(extraFormattedTexts(unchecked), QStringList{word});
    }

    // Between the moment an editor is handed a document and the moment it lays out its
    // viewport it shows nothing, which is not the same as there being no editor.
    void testNothingIsMarkedWhileAViewerShowsNothing()
    {
        REQUIRE_SPELL_CHECKING();
        setUpDocument("A mispelled word\n");

        QObject viewer;
        m_highlighter->setVisibleBlocks(&viewer, {});
        m_highlighter->rehighlight();
        QCOMPARE(underlinedTexts(), QStringList());
    }

    // The generic highlighter reads which parts of a file are prose off the syntax
    // definition of the file, the way the editor of any such file does. The language
    // goes to the highlighter rather than to the settings: what is under test is which
    // parts of a text a highlighter hands to the dictionary.
    void testCommentsAreMarkedInCode()
    {
        REQUIRE_SPELL_CHECKING();
        // The definition of a CMake file has its comments as prose and the arguments of
        // a command as code, so only the word in the comment is up for checking.
        SyntaxHighlighter *highlighter = setUpEditor(Utils::Constants::CMAKE_MIMETYPE,
                                                     "# A mispelled comment\nset(mispelled 1)\n");
        QVERIFY(highlighter);
        highlighter->setSpellCheckLanguage(m_language);
        QTRY_COMPARE(underlinedTexts(), QStringList{"mispelled"});
    }

    void testProseIsMarkedInTextFiles()
    {
        REQUIRE_SPELL_CHECKING();
        // The text of a Markdown file is prose, and a block it fences off as code of a
        // language is that language, down to which parts of it are prose.
        SyntaxHighlighter *highlighter
            = setUpEditor("text/markdown",
                          "A mispelled word\n\n```cmake\nset(mispelled 1)\n```\n");
        QVERIFY(highlighter);
        highlighter->setSpellCheckLanguage(m_language);
        QTRY_COMPARE(underlinedTexts(), QStringList{"mispelled"});
    }

    void testFencedCodeIsMarkedAsTheLanguageItNames()
    {
        REQUIRE_SPELL_CHECKING();
        // The definition of a Markdown file was written with spell checking in mind, the
        // one of a YAML file was not and the one of an HTML file was. What a fenced block
        // is measured against is the definition of the language it names rather than the
        // one around it, so the value of a YAML key is code and the text of an HTML
        // element is prose.
        SyntaxHighlighter *highlighter
            = setUpEditor("text/markdown",
                          "A mispelled word\n\n```yaml\nkey: a mistaeken value\n```\n"
                          "\n```html\n<p>A mistuken word</p>\n```\n");
        QVERIFY(highlighter);
        highlighter->setSpellCheckLanguage(m_language);
        QTRY_COMPARE(underlinedTexts(), QStringList({"mispelled", "mistuken"}));
    }

    void testProseIsMarkedWhereNoSyntaxDefinitionApplies()
    {
        REQUIRE_SPELL_CHECKING();
        // No syntax definition matches a plain text file, which leaves it prose from end
        // to end rather than leaving it out of the checking.
        SyntaxHighlighter *highlighter = setUpEditor("text/plain", "A mispelled word\n");
        QVERIFY(highlighter);
        highlighter->setSpellCheckLanguage(m_language);
        QTRY_COMPARE(underlinedTexts(), QStringList{"mispelled"});
    }

    void testOnlyCommentsAreMarkedWhereADefinitionNamesNoProse()
    {
        REQUIRE_SPELL_CHECKING();
        // The definition of a YAML file says of none of its formats that it holds prose,
        // so its word that all of them do counts for nothing: a key and a value are no
        // more prose than the arguments of a CMake command are. Its comments are.
        SyntaxHighlighter *highlighter
            = setUpEditor("text/yaml", "key: a mistaeken value\n# A mispelled comment\n");
        QVERIFY(highlighter);
        highlighter->setSpellCheckLanguage(m_language);
        QTRY_COMPARE(underlinedTexts(), QStringList{"mispelled"});
    }

    void testNormalTextIsNotMarkedWhereADefinitionNamesNoProse()
    {
        REQUIRE_SPELL_CHECKING();
        // The definition of a Yacc file styles its declarations as the normal text of the
        // file, which is code all the same, and says of no format that it holds prose. Its
        // comments are prose and nothing else is.
        SyntaxHighlighter *highlighter
            = setUpEditorForDefinition("Yacc/Bison", "/* A mispelled comment */\nmistaeken\n");
        QVERIFY(highlighter);
        highlighter->setSpellCheckLanguage(m_language);
        QTRY_COMPARE(underlinedTexts(), QStringList{"mispelled"});
    }

    void testOnlyCommentsAreMarkedWhereADefinitionNamesNoCode()
    {
        REQUIRE_SPELL_CHECKING();
        // The definition of a Ruby file turns the flag off for the escape sequences of a
        // string and for its regular expressions, where the dictionary is kept away
        // anyhow, and for nothing else: what it leaves on for a keyword is the default it
        // was never asked about, and no keyword is prose because of it.
        SyntaxHighlighter *highlighter
            = setUpEditorForDefinition("Ruby", "# A mispelled comment\nelsif\n");
        QVERIFY(highlighter);
        highlighter->setSpellCheckLanguage(m_language);
        QTRY_COMPARE(underlinedTexts(), QStringList{"mispelled"});
    }

    // An editor takes the language to check in from the settings, with no highlighter of
    // a test having handed it one.
    void testSettingSwitchesCheckingOfEditors()
    {
        REQUIRE_SPELL_CHECKING();
        SpellCheckSettings &settings = spellCheckSettings();
        const bool wasChecking = settings.checkText();
        const QString previousLanguage = settings.language();
        const QScopeGuard restore([&settings, wasChecking, previousLanguage] {
            settings.language.setValue(previousLanguage);
            settings.checkText.setValue(wasChecking);
        });
        settings.language.setValue(m_language);
        settings.checkText.setValue(true);

        QVERIFY(setUpEditor(Utils::Constants::CMAKE_MIMETYPE, "# A mispelled comment\n"));
        QTRY_COMPARE(underlinedTexts(), QStringList{"mispelled"});

        settings.checkText.setValue(false);
        QTRY_COMPARE(underlinedTexts(), QStringList());
    }

    // The corrections on offer are those for a word the editor marks, and for no other:
    // what a menu offers and what the text shows are to be the same thing.
    void testMenuOffersCorrectionsForMarkedWords()
    {
        REQUIRE_SPELL_CHECKING();
        const QString text = "# A mispelled comment\n";
        SyntaxHighlighter *highlighter = setUpEditor(Utils::Constants::CMAKE_MIMETYPE, text);
        QVERIFY(highlighter);
        highlighter->setSpellCheckLanguage(m_language);
        QTRY_COMPARE(underlinedTexts(), QStringList{"mispelled"});

        QMenu menu;
        addSpellingActions(&menu, highlighter, cursorAt(text.indexOf("mispelled") + 3));
        QVERIFY(Utils::contains(menu.actions(), [](const QAction *action) {
            return action->text() == Tr::tr("Add \"%1\" to Dictionary").arg("mispelled");
        }));

        QMenu spelledRight;
        addSpellingActions(&spelledRight, highlighter, cursorAt(text.indexOf("comment") + 3));
        QCOMPARE(spelledRight.actions(), QList<QAction *>());
    }

    // A color scheme may underline another category the way it underlines a spelling
    // error - the grayscale one is a red away from it - and a word underlined for a
    // reason of its own is no word the dictionary had anything to say about.
    void testMenuOffersNoCorrectionsForOtherUnderlines()
    {
        REQUIRE_SPELL_CHECKING();
        const QString text = "# A mispelled comment\n";
        SyntaxHighlighter *highlighter = setUpEditor(Utils::Constants::CMAKE_MIMETYPE, text);
        QVERIFY(highlighter);
        highlighter->setSpellCheckLanguage(m_language);
        QTRY_COMPARE(underlinedTexts(), QStringList{"mispelled"});

        QTextCharFormat lookalike;
        lookalike.setUnderlineStyle(spellErrorFormat().underlineStyle());
        lookalike.setUnderlineColor(spellErrorFormat().underlineColor());
        const QString spelledRight = "comment";
        const int word = int(text.indexOf(spelledRight));
        highlighter->setExtraFormats(m_checkedDocument->firstBlock(),
                                     {{word, int(spelledRight.size()), lookalike}});

        QMenu menu;
        addSpellingActions(&menu, highlighter, cursorAt(word + 3));
        QCOMPARE(menu.actions(), QList<QAction *>());
    }

    void cleanup()
    {
        if (m_editor)
            Core::EditorManager::closeEditors({m_editor}, false);
        m_editor = nullptr;
        m_document.reset();
        m_checkedDocument = nullptr;
    }

private:
    static QTextCharFormat spellErrorFormat()
    {
        return globalFontSettings().data().toTextCharFormat(C_SPELL_ERROR);
    }

    // A format to hand setExtraFormats(), marked so that extraFormattedTexts() finds it
    // again. The property is one the highlighter itself puts on no format.
    static QTextCharFormat extraFormat()
    {
        QTextCharFormat format;
        format.setProperty(QTextFormat::UserProperty, true);
        return format;
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
        m_checkedDocument = m_document.get();
        m_highlighter = new SpellCheckingHighlighter(m_document.get());
        m_highlighter->setFontSettings(globalFontSettings().data());
        m_highlighter->setSpellCheckLanguage(m_language);
    }

    // An editor on a file of contents, with no syntax definition chosen for it yet.
    bool openEditor(const QString &contents)
    {
        QString title = "spellcheck";
        m_editor = qobject_cast<BaseTextEditor *>(Core::EditorManager::openEditorWithContents(
            Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, contents.toUtf8()));
        if (!m_editor)
            return false;
        m_checkedDocument = m_editor->textDocument()->document();
        return true;
    }

    // The highlighter of an editor on a file of mimeType, which is the generic one that
    // reads the syntax definitions. Null when there is no editor to check.
    SyntaxHighlighter *setUpEditor(const QString &mimeType, const QString &contents)
    {
        if (!openEditor(contents))
            return nullptr;
        m_editor->editorWidget()->configureGenericHighlighter(Utils::mimeTypeForName(mimeType));
        return m_editor->textDocument()->syntaxHighlighter();
    }

    // The same for a language the mime type database knows nothing of, which the editor
    // of such a file reaches over its file name instead.
    SyntaxHighlighter *setUpEditorForDefinition(const QString &definitionName,
                                                const QString &contents)
    {
        if (!openEditor(contents))
            return nullptr;
        if (!m_editor->editorWidget()->configureGenericHighlighter(definitionName))
            return nullptr;
        return m_editor->textDocument()->syntaxHighlighter();
    }

    QTextCursor cursorAt(int position) const
    {
        QTextCursor cursor(m_checkedDocument);
        cursor.setPosition(position);
        return cursor;
    }

    QStringList underlinedTexts() const
    {
        QStringList texts;
        for (QTextBlock block = m_checkedDocument->firstBlock(); block.isValid();
             block = block.next()) {
            const QList<QTextLayout::FormatRange> ranges = block.layout()->formats();
            for (const QTextLayout::FormatRange &range : ranges) {
                if (SyntaxHighlighter::isSpellingError(range.format))
                    texts.append(block.text().mid(range.start, range.length));
            }
        }
        return texts;
    }

    // The words that the extra formats of block cover, wherever those formats sit now.
    static QStringList extraFormattedTexts(const QTextBlock &block)
    {
        QStringList texts;
        const QList<QTextLayout::FormatRange> ranges = block.layout()->formats();
        for (const QTextLayout::FormatRange &range : ranges) {
            if (range.format.property(QTextFormat::UserProperty).toBool())
                texts.append(block.text().mid(range.start, range.length));
        }
        return texts;
    }

    QString m_language;
    std::unique_ptr<QTextDocument> m_document;
    SpellCheckingHighlighter *m_highlighter = nullptr;
    BaseTextEditor *m_editor = nullptr;
    QTextDocument *m_checkedDocument = nullptr;
};

#undef REQUIRE_SPELL_CHECKING

QObject *createSpellCheckTest()
{
    return new SpellCheckTest;
}

} // namespace TextEditor::Internal

#include "spellcheck_test.moc"
