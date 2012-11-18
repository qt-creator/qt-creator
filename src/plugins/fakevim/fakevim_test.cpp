/**************************************************************************
**
** Copyright (C) 2012 Lukas Holecek <hluk@email.cz>
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

/*!
 * Tests for FakeVim plugin.
 * All test are based on Vim behaviour.
 */

#include "fakevimplugin.h"
#include "fakevimhandler.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/basetexteditor.h>

#include <QtTest>
#include <QTextEdit>
#include <QTextDocument>
#include <QTextBlock>

//TESTED_COMPONENT=src/plugins/fakevim

/*!
 * Tests after this macro will be skipped and warning printed.
 * Uncomment it to test a feature -- if tests succeeds it should be removed from the test.
 */
#define NOT_IMPLEMENTED return;

//   QTest::qSkip("Not fully implemented!", QTest::SkipSingle, __FILE__, __LINE__);
//   return;

// Text cursor representation in comparisons (set empty to disable cursor position checking).
#define X "|"
const QString cursorString(X);

const QString testLines =
  /* 0         1         2         3        4 */
  /* 0123456789012345678901234567890123457890 */
    "\n"
    "#include <QtCore>\n"
    "#include <QtGui>\n"
    "\n"
    "int main(int argc, char *argv[])\n"
    "{\n"
    "    QApplication app(argc, argv);\n"
    "\n"
    "    return app.exec();\n"
    "}\n";

// More distinct line separator in code.
#define N "\n"

// Document line start and end string in error text.
#define LINE_START "\t\t<"
#define LINE_END ">\n"

// Format of message after comparison fails (used by KEYS, COMMAND).
static const QString helpFormat =
    "\n\tBefore command [%1]:\n" \
    LINE_START "%2" LINE_END \
    "\n\tAfter the command:\n" \
    LINE_START "%3" LINE_END \
    "\n\tShould be:\n" \
    LINE_START "%4" LINE_END;

// Compare document contents with a expectedText.
// Also check cursor position if the expectedText contains cursorString.
#define COMPARE(beforeText, beforePosition, afterText, afterPosition, expectedText, cmd) \
    do { \
        QString before(beforeText); \
        QString actual(afterText); \
        QString expected(expectedText); \
        data.oldPosition = beforePosition; \
        data.oldText = before; \
        if (!cursorString.isEmpty() && expected.contains(cursorString)) {\
            before = textWithCursor(before, beforePosition); \
            actual = textWithCursor(actual, afterPosition); \
        } \
        QString help = helpFormat \
            .arg(QString(cmd)) \
            .arg(before.replace('\n', LINE_END LINE_START)) \
            .arg(actual.replace('\n', LINE_END LINE_START)) \
            .arg(expected.replace('\n', LINE_END LINE_START)); \
        QVERIFY2(actual == expected, help.toLatin1().constData()); \
    } while (false)

// Send keys and check if the expected result is same as document contents.
// Escape is always prepended to keys so that previous command is cancelled.
#define KEYS(keys, expected) \
    do { \
        QString beforeText(data.text()); \
        int beforePosition = data.position(); \
        data.doKeys("<ESC>"); \
        data.doKeys(keys); \
        COMPARE(beforeText, beforePosition, data.text(), data.position(), (expected), (keys)); \
    } while (false)

// Run Ex command and check if the expected result is same as document contents.
#define COMMAND(cmd, expected) \
    do { \
        QString beforeText(data.text()); \
        int beforePosition = data.position(); \
        data.doCommand(cmd); \
        COMPARE(beforeText, beforePosition, data.text(), data.position(), (expected), (":" cmd)); \
    } while (false)

// Test undo, redo and repeat of last single command. This doesn't test cursor position.
// Set afterEnd to true if cursor position after undo and redo differs at the end of line
// (e.g. undoing 'A' operation moves cursor at the end of line and redo moves it one char right).
#define INTEGRITY(afterEnd) \
    do { \
        data.doKeys("<ESC>"); \
        const int newPosition = data.position(); \
        const int oldPosition = data.oldPosition; \
        const QString redo = data.text(); \
        KEYS("u", data.oldText); \
        const QTextCursor tc = data.cursor(); \
        const int pos = tc.position(); \
        const int col = tc.positionInBlock() \
            + ((afterEnd && tc.positionInBlock() + 2 == tc.block().length()) ? 1 : 0); \
        const int line = tc.block().blockNumber(); \
        const QTextDocument *doc = data.editor()->document(); \
        KEYS("<c-r>", textWithCursor(redo, doc->findBlockByNumber(line), col)); \
        KEYS("u", textWithCursor(data.oldText, pos)); \
        data.setPosition(oldPosition); \
        KEYS(".", textWithCursor(redo, newPosition)); \
    } while (false)

using namespace FakeVim::Internal;
using namespace TextEditor;

static QString textWithCursor(const QString &text, int position)
{
    return (position == -1) ? text : (text.left(position) + cursorString + text.mid(position));
}

static QString textWithCursor(const QString &text, const QTextBlock &block, int column)
{
    const int pos = block.position() + qMin(column, qMax(0, block.length() - 2));
    return text.left(pos) + cursorString + text.mid(pos);
}


// Data for tests containing BaseTextEditorWidget and FakeVimHAndler.
struct FakeVimPlugin::TestData
{
    FakeVimHandler *handler;
    QWidget *edit;
    QString title;

    int oldPosition;
    QString oldText;

    BaseTextEditorWidget *editor() const { return qobject_cast<BaseTextEditorWidget *>(edit); }

    QTextCursor cursor() const { return editor()->textCursor(); }

    int position() const
    {
        return cursor().position();
    }

    void setPosition(int position)
    {
        handler->setTextCursorPosition(position);
    }

    QString text() const { return editor()->toPlainText(); }

    void doCommand(const QString &cmd) { handler->handleCommand(cmd); }
    void doKeys(const QString &keys) { handler->handleInput(keys); }

    void setText(const QString &text)
    {
        doKeys("<ESC>");
        QString str = text;
        int i = str.indexOf(cursorString);
        if (!cursorString.isEmpty() && i != -1)
            str.remove(i, 1);
        editor()->document()->setPlainText(str);
        setPosition(i);
    }

    int lines() const
    {
        QTextDocument *doc = editor()->document();
        Q_ASSERT(doc != 0);
        return doc->lineCount();
    }
};

void FakeVimPlugin::setup(TestData *data)
{
    setupTest(&data->title, &data->handler, &data->edit);
}


void FakeVimPlugin::cleanup()
{
    Core::EditorManager::instance()->closeAllEditors(false);
}


void FakeVimPlugin::test_vim_indentation()
{
    TestData data;
    setup(&data);

    data.doCommand("set expandtab");
    data.doCommand("set tabstop=4");
    data.doCommand("set shiftwidth=4");
    QCOMPARE(data.handler->physicalIndentation("      \t\t\tx"), 6 + 3);
    QCOMPARE(data.handler->logicalIndentation ("      \t\t\tx"), 4 + 3 * 4);
    QCOMPARE(data.handler->physicalIndentation("     \t\t\tx"), 5 + 3);
    QCOMPARE(data.handler->logicalIndentation ("     \t\t\tx"), 4 + 3 * 4);

    QCOMPARE(data.handler->tabExpand(3), QLatin1String("   "));
    QCOMPARE(data.handler->tabExpand(4), QLatin1String("    "));
    QCOMPARE(data.handler->tabExpand(5), QLatin1String("     "));
    QCOMPARE(data.handler->tabExpand(6), QLatin1String("      "));
    QCOMPARE(data.handler->tabExpand(7), QLatin1String("       "));
    QCOMPARE(data.handler->tabExpand(8), QLatin1String("        "));
    QCOMPARE(data.handler->tabExpand(9), QLatin1String("         "));

    data.doCommand("set expandtab");
    data.doCommand("set tabstop=8");
    data.doCommand("set shiftwidth=4");
    QCOMPARE(data.handler->physicalIndentation("      \t\t\tx"), 6 + 3);
    QCOMPARE(data.handler->logicalIndentation ("      \t\t\tx"), 0 + 3 * 8);
    QCOMPARE(data.handler->physicalIndentation("     \t\t\tx"), 5 + 3);
    QCOMPARE(data.handler->logicalIndentation ("     \t\t\tx"), 0 + 3 * 8);

    QCOMPARE(data.handler->tabExpand(3), QLatin1String("   "));
    QCOMPARE(data.handler->tabExpand(4), QLatin1String("    "));
    QCOMPARE(data.handler->tabExpand(5), QLatin1String("     "));
    QCOMPARE(data.handler->tabExpand(6), QLatin1String("      "));
    QCOMPARE(data.handler->tabExpand(7), QLatin1String("       "));
    QCOMPARE(data.handler->tabExpand(8), QLatin1String("        "));
    QCOMPARE(data.handler->tabExpand(9), QLatin1String("         "));

    data.doCommand("set noexpandtab");
    data.doCommand("set tabstop=4");
    data.doCommand("set shiftwidth=4");
    QCOMPARE(data.handler->physicalIndentation("      \t\t\tx"), 6 + 3);
    QCOMPARE(data.handler->logicalIndentation ("      \t\t\tx"), 4 + 3 * 4);
    QCOMPARE(data.handler->physicalIndentation("     \t\t\tx"), 5 + 3);
    QCOMPARE(data.handler->logicalIndentation ("     \t\t\tx"), 4 + 3 * 4);

    QCOMPARE(data.handler->tabExpand(3), QLatin1String("   "));
    QCOMPARE(data.handler->tabExpand(4), QLatin1String("\t"));
    QCOMPARE(data.handler->tabExpand(5), QLatin1String("\t "));
    QCOMPARE(data.handler->tabExpand(6), QLatin1String("\t  "));
    QCOMPARE(data.handler->tabExpand(7), QLatin1String("\t   "));
    QCOMPARE(data.handler->tabExpand(8), QLatin1String("\t\t"));
    QCOMPARE(data.handler->tabExpand(9), QLatin1String("\t\t "));

    data.doCommand("set noexpandtab");
    data.doCommand("set tabstop=8");
    data.doCommand("set shiftwidth=4");
    QCOMPARE(data.handler->physicalIndentation("      \t\t\tx"), 6 + 3);
    QCOMPARE(data.handler->logicalIndentation ("      \t\t\tx"), 0 + 3 * 8);
    QCOMPARE(data.handler->physicalIndentation("     \t\t\tx"), 5 + 3);
    QCOMPARE(data.handler->logicalIndentation ("     \t\t\tx"), 0 + 3 * 8);

    QCOMPARE(data.handler->tabExpand(3), QLatin1String("   "));
    QCOMPARE(data.handler->tabExpand(4), QLatin1String("    "));
    QCOMPARE(data.handler->tabExpand(5), QLatin1String("     "));
    QCOMPARE(data.handler->tabExpand(6), QLatin1String("      "));
    QCOMPARE(data.handler->tabExpand(7), QLatin1String("       "));
    QCOMPARE(data.handler->tabExpand(8), QLatin1String("\t"));
    QCOMPARE(data.handler->tabExpand(9), QLatin1String("\t "));
}

void FakeVimPlugin::test_vim_movement()
{
    TestData data;
    setup(&data);

    // vertical movement
    data.setText("123" N   "456" N   "789" N   "abc");
    KEYS("",   X "123" N   "456" N   "789" N   "abc");
    KEYS("j",    "123" N X "456" N   "789" N   "abc");
    KEYS("G",    "123" N   "456" N   "789" N X "abc");
    KEYS("k",    "123" N   "456" N X "789" N   "abc");
    KEYS("2k", X "123" N   "456" N   "789" N   "abc");
    KEYS("k",  X "123" N   "456" N   "789" N   "abc");
    KEYS("jj",   "123" N   "456" N X "789" N   "abc");
    KEYS("gg", X "123" N   "456" N   "789" N   "abc");

    // horizontal movement
    data.setText(" " X "x"   "x"   "x"   "x");
    KEYS("",     " " X "x"   "x"   "x"   "x");
    KEYS("h",  X " "   "x"   "x"   "x"   "x");
    KEYS("l",    " " X "x"   "x"   "x"   "x");
    KEYS("3l",   " "   "x"   "x"   "x" X "x");
    KEYS("2h",   " "   "x" X "x"   "x"   "x");
    KEYS("$",    " "   "x"   "x"   "x" X "x");
    KEYS("^",    " " X "x"   "x"   "x"   "x");
    KEYS("0",  X " "   "x"   "x"   "x"   "x");

    // skip words
    data.setText("123 "   "456"   "."   "789 "   "abc");
    KEYS("b",  X "123 "   "456"   "."   "789 "   "abc");
    KEYS("w",    "123 " X "456"   "."   "789 "   "abc");
    KEYS("2w",   "123 "   "456"   "." X "789 "   "abc");
    KEYS("3w",   "123 "   "456"   "."   "789 "   "ab" X "c");
    KEYS("3b",   "123 "   "456" X "."   "789 "   "abc");

    data.setText("123 "   "456.789 "   "abc "   "def");
    KEYS("B",  X "123 "   "456.789 "   "abc "   "def");
    KEYS("W",    "123 " X "456.789 "   "abc "   "def");
    KEYS("2W",   "123 "   "456.789 "   "abc " X "def");
    KEYS("B",    "123 "   "456.789 " X "abc "   "def");
    KEYS("2B", X "123 "   "456.789 "   "abc "   "def");
    KEYS("4W",   "123 "   "456.789 "   "abc "   "de" X "f");

    data.setText("123" N   "45."   "6" N   "" N " " N   "789");
    KEYS("3w",   "123" N   "45." X "6" N   "" N " " N   "789");
    // From Vim help (motion.txt): An empty line is also considered to be a word.
    KEYS("w",    "123" N   "45."   "6" N X "" N " " N   "789");
    KEYS("w",    "123" N   "45."   "6" N   "" N " " N X "789");

    KEYS("b",    "123" N   "45."   "6" N X "" N " " N   "789");
    KEYS("4b", X "123" N   "45."   "6" N   "" N " " N   "789");

    KEYS("3e",    "123" N "45" X "."   "6" N "" N " " N "789");
    KEYS("e",     "123" N "45"   "." X "6" N "" N " " N "789");
    // Command "e" does not stop on empty lines ("ge" does).
    KEYS("e",     "123" N "45"   "."   "6" N "" N " " N "78" X "9");
    KEYS("ge",    "123" N "45"   "."   "6" N X "" N " " N "789");
    KEYS("2ge",   "123" N "45" X "."   "6" N   "" N " " N "789");

    // do not move behind end of line in normal mode
    data.setText("abc def" N "ghi");
    KEYS("$h", "abc d" X "ef" N "ghi");
    data.setText("abc def" N "ghi");
    KEYS("4e", "abc def" N "gh" X "i");
    data.setText("abc def" N "ghi");
    KEYS("$i", "abc de" X "f" N "ghi");

    // move behind end of line in insert mode
    data.setText("abc def" N "ghi");
    KEYS("i<end>", "abc def" X N "ghi");
    data.setText("abc def" N "ghi");
    KEYS("A", "abc def" X N "ghi");
    data.setText("abc def" N "ghi");
    KEYS("$a", "abc def" X N "ghi");
}

void FakeVimPlugin::test_vim_insert()
{
    TestData data;
    setup(&data);

    // basic insert text
    data.setText("ab" X "c" N "def");
    KEYS("i 123", "ab 123" X "c" N "def");
    INTEGRITY(false);

    data.setText("ab" X "c" N "def");
    KEYS("a 123", "abc 123" X N "def");
    INTEGRITY(true);

    data.setText("ab" X "c" N "def");
    KEYS("I 123", " 123" X "abc" N "def");
    INTEGRITY(false);

    data.setText("abc" N "def");
    KEYS("A 123", "abc 123" X N "def");
    INTEGRITY(true);

    data.setText("abc" N "def");
    KEYS("o 123", "abc" N " 123" X N "def");
    INTEGRITY(false);

    data.setText("abc" N "def");
    KEYS("O 123", " 123" X N "abc" N "def");
    INTEGRITY(false);

    // insert text [count] times
    data.setText("ab" X "c" N "def");
    KEYS("3i 123<esc>", "ab 123 123 12" X "3c" N "def");
    INTEGRITY(false);

    data.setText("ab" X "c" N "def");
    KEYS("3a 123<esc>", "abc 123 123 12" X "3" N "def");
    INTEGRITY(true);

    data.setText("ab" X "c" N "def");
    KEYS("3I 123<esc>", " 123 123 12" X "3abc" N "def");
    INTEGRITY(false);

    data.setText("abc" N "def");
    KEYS("3A 123<esc>", "abc 123 123 12" X "3" N "def");
    INTEGRITY(true);

    data.setText("abc" N "def");
    KEYS("3o 123<esc>", "abc" N " 123" N " 123" N " 12" X "3" N "def");
    INTEGRITY(false);

    data.setText("abc" N "def");
    KEYS("3O 123<esc>", " 123" N " 123" N " 12" X "3" N "abc" N "def");
    INTEGRITY(false);
}

void FakeVimPlugin::test_vim_fFtT()
{
    TestData data;
    setup(&data);

    data.setText("123()456" N "a(b(c)d)e");
    KEYS("t(", "12" X "3()456" N "a(b(c)d)e");
    KEYS("lt(", "123" X "()456" N "a(b(c)d)e");
    KEYS("0j2t(", "123()456" N "a(" X "b(c)d)e");
    KEYS("l2T(", "123()456" N "a(b" X "(c)d)e");
    KEYS("l2T(", "123()456" N "a(" X "b(c)d)e");
    KEYS("T(", "123()456" N "a(" X "b(c)d)e");

    KEYS("ggf(", "123" X "()456" N "a(b(c)d)e");
    KEYS("lf(", "123(" X ")456" N "a(b(c)d)e");
    KEYS("0j2f(", "123()456" N "a(b" X "(c)d)e");
    KEYS("2F(", "123()456" N "a(b" X "(c)d)e");
    KEYS("l2F(", "123()456" N "a" X "(b(c)d)e");
    KEYS("F(", "123()456" N "a" X "(b(c)d)e");
}

void FakeVimPlugin::test_vim_transform_numbers()
{
    TestData data;
    setup(&data);

    data.setText("8");
    KEYS("<c-a>", X "9");
    INTEGRITY(false);
    KEYS("<c-x>", X "8");
    INTEGRITY(false);
    KEYS("<c-a>", X "9");
    KEYS("<c-a>", "1" X "0");
    KEYS("<c-a>", "1" X "1");
    KEYS("5<c-a>", "1" X "6");
    INTEGRITY(false);
    KEYS("10<c-a>", "2" X "6");
    KEYS("h100<c-a>", "12" X "6");
    KEYS("100<c-x>", "2" X "6");
    INTEGRITY(false);
    KEYS("10<c-x>", "1" X "6");
    KEYS("5<c-x>", "1" X "1");
    KEYS("5<c-x>", X "6");
    KEYS("6<c-x>", X "0");
    KEYS("<c-x>", "-" X "1");
    KEYS("h10<c-x>", "-1" X "1");
    KEYS("h100<c-x>", "-11" X "1");
    KEYS("h889<c-x>", "-100" X "0");

    // increase nearest number
    data.setText("x-x+x: 1 2 3 -4 5");
    KEYS("8<c-a>", "x-x+x: " X "9 2 3 -4 5");
    KEYS("l8<c-a>", "x-x+x: 9 1" X "0 3 -4 5");
    KEYS("l8<c-a>", "x-x+x: 9 10 1" X "1 -4 5");
    KEYS("l16<c-a>", "x-x+x: 9 10 11 1" X "2 5");
    KEYS("w18<c-x>", "x-x+x: 9 10 11 12 -1" X "3");
    KEYS("hh13<c-a>", "x-x+x: 9 10 11 12 " X "0");
    KEYS("B12<c-x>", "x-x+x: 9 10 11 " X "0 0");
    KEYS("B11<c-x>", "x-x+x: 9 10 " X "0 0 0");
    KEYS("B10<c-x>", "x-x+x: 9 " X "0 0 0 0");
    KEYS("B9<c-x>", "x-x+x: " X "0 0 0 0 0");
    KEYS("B9<c-x>", "x-x+x: -" X "9 0 0 0 0");

    data.setText("-- 1 --");
    KEYS("<c-x>", "-- " X "0 --");
    KEYS("<c-x><c-x>", "-- -" X "2 --");
    KEYS("2<c-a><c-a>", "-- " X "1 --");
    KEYS("<c-a>2<c-a>", "-- " X "4 --");
    KEYS(".", "-- " X "6 --");
}

void FakeVimPlugin::test_vim_delete()
{
    TestData data;
    setup(&data);

    data.setText("123" N "456");
    KEYS("x",  "23" N "456");
    INTEGRITY(false);
    KEYS("dd", "456");
    INTEGRITY(false);
    KEYS("2x", "6");
    INTEGRITY(false);
    KEYS("dd", "");
    INTEGRITY(false);

    data.setText("void main()");
    KEYS("dt(", "()");
    INTEGRITY(false);

    data.setText("void main()");
    KEYS("df(", ")");
    INTEGRITY(false);

    data.setText("void " X "main()");
    KEYS("D", "void ");
    INTEGRITY(false);
    KEYS("ggd$", "");

    data.setText("abc def ghi");
    KEYS("2dw", X "ghi");
    INTEGRITY(false);
    data.setText("abc def ghi");
    KEYS("d2w", X "ghi");
    INTEGRITY(false);

    data.setText("abc  " N "  def" N "  ghi" N "jkl");
    KEYS("3dw", X "jkl");
    data.setText("abc  " N "  def" N "  ghi" N "jkl");
    KEYS("d3w", X "jkl");

    // delete empty line
    data.setText("a" N X "" N "  b");
    KEYS("dd", "a" N "  " X "b");

    // delete on an empty line
    data.setText("a" N X "" N "  b");
    KEYS("d$", "a" N X "" N "  b");
    INTEGRITY(false);

    // delete in empty document
    data.setText("");
    KEYS("dd", X);

    // delete to start of line
    data.setText("  abc" N "  de" X "f" N "  ghi");
    KEYS("d0", "  abc" N X "f" N "  ghi");
    INTEGRITY(false);
    data.setText("  abc" N "  de" X "f" N "  ghi");
    KEYS("d^", "  abc" N "  " X "f" N "  ghi");
    INTEGRITY(false);

    // delete to mark
    data.setText("abc " X "def ghi");
    KEYS("ma" "3l" "d`a", "abc " X " ghi");
    KEYS("u" "gg" "d`a", X "def ghi");

    // delete lines
    data.setText("  abc" N "  de" X "f" N "  ghi" N "  jkl");
    KEYS("dj", "  abc" N "  " X "jkl");
    INTEGRITY(false);
    data.setText("  abc" N "  def" N "  gh" X "i" N "  jkl");
    KEYS("dk", "  abc" N "  " X "jkl");
    INTEGRITY(false);
}

void FakeVimPlugin::test_vim_delete_inner_word()
{
    TestData data;
    setup(&data);

    data.setText("abc def ghi");
    KEYS("wlldiw", "abc " X " ghi");

    data.setText("abc def ghi jkl");
    KEYS("3diw", X  " ghi jkl");
    INTEGRITY(false);

    data.setText("abc " X "  def");
    KEYS("diw", "abc" X "def");
    INTEGRITY(false);
    KEYS("diw", "");

    data.setText("abc  " N "  def");
    KEYS("3diw", X "def");

    data.setText("abc  " N "  def" N "  ghi");
    KEYS("4diw", "  " X "ghi");
    data.setText("ab" X "c  " N "  def" N "  ghi");
    KEYS("4diw", "  " X "ghi");
    data.setText("a b" X "c  " N "  def" N "  ghi");
    KEYS("4diw", "a" X " " N "  ghi");

    data.setText("abc def" N "ghi");
    KEYS("2diw", X "def" N "ghi");
    data.setText("abc def" N "ghi");
    KEYS("3diw", X "" N "ghi");

    data.setText("x" N X "" N "" N "  ");
    KEYS("diw", "x" N X "" N "" N "  ");
    data.setText("x" N X "" N "" N "  ");
    KEYS("2diw", "x" N " " X " ");
    data.setText("x" N X "" N "" N "" N "" N "  ");
    KEYS("3diw", "x" N " " X " ");
    data.setText("x" N X "" N "" N "" N "" N "" N "  ");
    KEYS("3diw", "x" N X "" N "  ");
    data.setText("x" N X "" N "" N "" N "" N "" N "" N "  ");
    KEYS("4diw", "x" N X "" N "  ");

    // delete single-character-word
    data.setText("a " X "b c");
    KEYS("diw", "a " X " c");
}

void FakeVimPlugin::test_vim_delete_a_word()
{
    TestData data;
    setup(&data);

    data.setText("abc def ghi");
    KEYS("wlldaw", "abc " X "ghi");

    data.setText("abc def ghi jkl");
    KEYS("wll2daw", "abc " X "jkl");

    data.setText("abc" X " def ghi");
    KEYS("daw", "abc" X " ghi");
    INTEGRITY(false);
    KEYS("daw", "ab" X "c");
    INTEGRITY(false);
    KEYS("daw", "");

    data.setText(X " ghi jkl");
    KEYS("daw", X " jkl");
    KEYS("ldaw", X " ");

    data.setText("abc def ghi jkl");
    KEYS("3daw", X "jkl");
    INTEGRITY(false);

    // remove trailing spaces
    data.setText("abc  " N "  def" N "  ghi" N "jkl");
    KEYS("3daw", X "jkl");

    data.setText("abc  " N "  def" N "  ghi" N "jkl");
    KEYS("3daw", X "jkl");

    data.setText("abc def" N "ghi");
    KEYS("2daw", X "" N "ghi");

    data.setText("x" N X "" N "" N "  ");
    KEYS("daw", "x" N " " X " ");
    data.setText("x" N X "" N "" N "" N "" N "  ");
    KEYS("2daw", "x" N " " X " ");
    data.setText("x" N X "" N "" N "" N "" N "" N "  ");
    KEYS("2daw", "x" N X "" N "  ");
    data.setText("x" N X "" N "" N "" N "" N "" N "" N "  ");
    KEYS("3daw", "x" N " " X " ");

    // delete single-character-word
    data.setText("a," X "b,c");
    KEYS("daw", "a," X ",c");
}

void FakeVimPlugin::test_vim_change_a_word()
{
    TestData data;
    setup(&data);

    data.setText("abc " X "def ghi");
    KEYS("caw#", "abc #" X "ghi");
    INTEGRITY(false);
    data.setText("abc d" X "ef ghi");
    KEYS("caw#", "abc #" X "ghi");
    data.setText("abc de" X "f ghi");
    KEYS("caw#", "abc #" X "ghi");

    data.setText("abc de" X "f ghi jkl");
    KEYS("2caw#", "abc #" X "jkl");
    INTEGRITY(false);

    data.setText("abc" X " def ghi jkl");
    KEYS("2caw#", "abc#" X " jkl");

    data.setText("abc " X "  def ghi jkl");
    KEYS("2caw#", "abc#" X " jkl");

    data.setText(" abc  " N "  def" N "  ghi" N " jkl");
    KEYS("3caw#", "#" X N " jkl");

    // change single-character-word
    data.setText("a " X "b c");
    KEYS("ciwX<esc>", "a " X "X c");
    KEYS("cawZ<esc>", "a " X "Zc");
}

void FakeVimPlugin::test_vim_change_replace()
{
    TestData data;
    setup(&data);

    // preserve lines in replace mode
    data.setText("abc" N "def");
    KEYS("llvjhrX", "ab" X "X" N "XXf");

    // change empty line
    data.setText("a" N X "" N "  b");
    KEYS("ccABC", "a" N "ABC" X N "  b");
    INTEGRITY(false);

    // change on empty line
    data.setText("a" N X "" N "  b");
    KEYS("c$ABC<esc>", "a" N "AB" X "C" N "  b");
    INTEGRITY(false);
    KEYS("u", "a" N X "" N "  b");
    KEYS("rA", "a" N X "" N "  b");

    // change in empty document
    data.setText("");
    KEYS("ccABC", "ABC" X);
    KEYS("u", "");
    KEYS("SABC", "ABC" X);
    KEYS("u", "");
    KEYS("sABC", "ABC" X);
    KEYS("u", "");
    KEYS("rA", "" X);

    // indentation with change
    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=2");
    data.setText("int main()" N
         "{" N
         " " X "   return 0;" N
         "}" N
         "");

    KEYS("cc" "int i = 0;",
         "int main()" N
         "{" N
         "  int i = 0;" X N
         "}" N
         "");
    INTEGRITY(false);

    KEYS("uS" "int i = 0;" N "int j = 1;",
         "int main()" N
         "{" N
         "  int i = 0;" N
         "  int j = 1;" X N
         "}" N
         "");

    // change to start of line
    data.setText("  abc" N "  de" X "f" N "  ghi");
    KEYS("c0123<ESC>", "  abc" N "12" X "3f" N "  ghi");
    INTEGRITY(false);
    data.setText("  abc" N "  de" X "f" N "  ghi");
    KEYS("c^123<ESC>", "  abc" N "  12" X "3f" N "  ghi");
    INTEGRITY(false);

    // change to mark
    data.setText("abc " X "def ghi");
    KEYS("ma" "3l" "c`a123<ESC>", "abc 12" X "3 ghi");
    KEYS("u" "gg" "c`a123<ESC>", "12" X "3def ghi");

    // change lines
    data.setText("  abc" N "  de" X "f" N "  ghi" N "  jkl");
    KEYS("cj123<ESC>", "  abc" N "  12" X "3" N "  jkl");
    INTEGRITY(false);
    data.setText("  abc" N "  def" N "  gh" X "i" N "  jkl");
    KEYS("ck123<ESC>", "  abc" N "  12" X "3" N "  jkl");
    INTEGRITY(false);
}

void FakeVimPlugin::test_vim_block_selection()
{
    TestData data;
    setup(&data);

    data.setText("int main(int /* (unused) */, char *argv[]);");
    KEYS("f(", "int main" X "(int /* (unused) */, char *argv[]);");
    KEYS("da(", "int main" X ";");
    INTEGRITY(false);

    data.setText("int main(int /* (unused) */, char *argv[]);");
    KEYS("f(", "int main" X "(int /* (unused) */, char *argv[]);");
    KEYS("di(", "int main(" X ");");
    INTEGRITY(false);

    data.setText("int main(int /* (unused) */, char *argv[]);");
    KEYS("2f)", "int main(int /* (unused) */, char *argv[]" X ");");
    KEYS("da(", "int main" X ";");

    data.setText("int main(int /* (unused) */, char *argv[]);");
    KEYS("2f)", "int main(int /* (unused) */, char *argv[]" X ");");
    KEYS("di(", "int main(" X ");");

    data.setText("{ { { } } }");
    KEYS("2f{l", "{ { {" X " } } }");
    KEYS("da{", "{ { " X " } }");
    KEYS("da{", "{ " X " }");
    INTEGRITY(false);

    data.setText("{ { { } } }");
    KEYS("2f{l", "{ { {" X " } } }");
    KEYS("2da{", "{ " X " }");
    INTEGRITY(false);

    data.setText("{" N " { " N " } " N "}");
    KEYS("di{", "{" N "}");

    data.setText("(" X "())");
    KEYS("di(", "((" X "))");
    data.setText("\"\"");
    KEYS("di\"", "\"" X "\"");

    // repeat change inner
    data.setText("(abc)" N "def" N "(ghi)");
    KEYS("ci(xyz<esc>", "(xy" X "z)" N "def" N "(ghi)");
    KEYS("j.", "(xyz)" N "de" X "f" N "(ghi)");
    KEYS("j.", "(xyz)" N "def" N "(xy" X "z)");
}

void FakeVimPlugin::test_vim_repeat()
{
    TestData data;
    setup(&data);

    // delete line
    data.setText("abc" N "def" N "ghi");
    KEYS("dd", X "def" N "ghi");
    KEYS(".", X "ghi");
    INTEGRITY(false);

    // delete to next word
    data.setText("abc def ghi jkl");
    KEYS("dw", X "def ghi jkl");
    KEYS("w.", "def " X "jkl");
    KEYS("gg.", X "jkl");

    // change in word
    data.setText("WORD text");
    KEYS("ciwWORD<esc>", "WOR" X "D text");
    KEYS("w.", "WORD WOR" X "D");

    /* QTCREATORBUG-7248 */
    data.setText("test tex" X "t");
    KEYS("vbcWORD<esc>", "test " "WOR" X "D");
    KEYS("bb.", "WOR" X "D WORD");

    // delete selected range
    data.setText("abc def ghi jkl");
    KEYS("viwd", X " def ghi jkl");
    KEYS(".", X "f ghi jkl");
    KEYS(".", X "hi jkl");

    // delete two lines
    data.setText("abc" N "def" N "ghi" N "jkl" N "mno");
    KEYS("Vjx", X "ghi" N "jkl" N "mno");
    KEYS(".", X "mno");

    // delete three lines
    data.setText("abc" N "def" N "ghi" N "jkl" N "mno" N "pqr" N "stu");
    KEYS("d2j", X "jkl" N "mno" N "pqr" N "stu");
    KEYS(".", X "stu");

    // replace block selection
    data.setText("abcd" N "d" X "efg" N "ghij" N "jklm");
    KEYS("<c-v>jlrX", "abcd" N "d" X "XXg" N "gXXj" N "jklm");
    KEYS("gg.", "XXcd" N "XXXg" N "gXXj" N "jklm");
}

void FakeVimPlugin::test_vim_search()
{
    TestData data;
    setup(&data);

    data.setText("abc" N "def" N "ghi");
    KEYS("/ghi<CR>", "abc" N "def" N X "ghi");
    KEYS("gg/\\w\\{3}<CR>", "abc" N X "def" N "ghi");
    KEYS("n", "abc" N "def" N X "ghi");
    KEYS("N", "abc" N X "def" N "ghi");
    KEYS("N", X "abc" N "def" N "ghi");

    // return to search-start position on escape or not found
    KEYS("/def<ESC>", X "abc" N "def" N "ghi");
    KEYS("/x", X "abc" N "def" N "ghi");
    KEYS("/x<CR>", X "abc" N "def" N "ghi");
    KEYS("/x<ESC>", X "abc" N "def" N "ghi");
    KEYS("/ghX", X "abc" N "def" N "ghi");

    KEYS("?def<ESC>", X "abc" N "def" N "ghi");
    KEYS("?x", X "abc" N "def" N "ghi");
    KEYS("?x<CR>", X "abc" N "def" N "ghi");
    KEYS("?x<ESC>", X "abc" N "def" N "ghi");

    // search [count] times
    data.setText("abc" N "def" N "ghi");
    KEYS("/\\w\\{3}<CR>", "abc" N X "def" N "ghi");
    KEYS("2n", X "abc" N "def" N "ghi");
    KEYS("2N", "abc" N X "def" N "ghi");
    KEYS("2/\\w\\{3}<CR>", X "abc" N "def" N "ghi");

    // set wrapscan (search wraps at end of file)
    data.doCommand("set ws");
    data.setText("abc" N "def" N "abc" N "ghi abc jkl");
    KEYS("*", "abc" N "def" N X "abc" N "ghi abc jkl");
    KEYS("*", "abc" N "def" N "abc" N "ghi " X "abc jkl");
    KEYS("2*", "abc" N "def" N X "abc" N "ghi abc jkl");
    KEYS("#", X "abc" N "def" N "abc" N "ghi abc jkl");
    KEYS("#", "abc" N "def" N "abc" N "ghi " X "abc jkl");
    KEYS("#", "abc" N "def" N X "abc" N "ghi abc jkl");
    KEYS("2#", "abc" N "def" N "abc" N "ghi " X "abc jkl");

    data.doCommand("set nows");
    data.setText("abc" N "def" N "abc" N "ghi abc jkl");
    KEYS("*", "abc" N "def" N X "abc" N "ghi abc jkl");
    KEYS("*", "abc" N "def" N "abc" N "ghi " X "abc jkl");
    KEYS("*", "abc" N "def" N "abc" N "ghi " X "abc jkl");
    KEYS("#", "abc" N "def" N X "abc" N "ghi abc jkl");
    KEYS("#", X "abc" N "def" N "abc" N "ghi abc jkl");
    KEYS("#", X "abc" N "def" N "abc" N "ghi abc jkl");

    data.setText("abc" N "def" N "ab" X "c" N "ghi abc jkl");
    KEYS("#", X "abc" N "def" N "abc" N "ghi abc jkl");

    // search with g* and g#
    data.doCommand("set nows");
    data.setText("bc" N "abc" N "abcd" N "bc" N "b");
    KEYS("g*", "bc" N "a" X "bc" N "abcd" N "bc" N "b");
    KEYS("n", "bc" N "abc" N "a" X "bcd" N "bc" N "b");
    KEYS("n", "bc" N "abc" N "abcd" N X "bc" N "b");
    KEYS("n", "bc" N "abc" N "abcd" N X "bc" N "b");
    KEYS("g#", "bc" N "abc" N "a" X "bcd" N "bc" N "b");
    KEYS("n", "bc" N "a" X "bc" N "abcd" N "bc" N "b");
    KEYS("N", "bc" N "abc" N "a" X "bcd" N "bc" N "b");
    KEYS("3n", "bc" N "abc" N "a" X "bcd" N "bc" N "b");
    KEYS("2n", X "bc" N "abc" N "abcd" N "bc" N "b");

    /* QTCREATORBUG-7251 */
    data.setText("abc abc abc abc");
    KEYS("$?abc<CR>", "abc abc abc " X "abc");
    KEYS("2?abc<CR>", "abc " X "abc abc abc");
    KEYS("n", X "abc abc abc abc");
    KEYS("N", "abc " X "abc abc abc");

    // search is greedy
    data.doCommand("set ws");
    data.setText("abc" N "def" N "abc" N "ghi abc jkl");
    KEYS("/[a-z]*<CR>", "abc" N X "def" N "abc" N "ghi abc jkl");
    KEYS("2n", "abc" N "def" N "abc" N X "ghi abc jkl");
    KEYS("3n", "abc" N "def" N "abc" N "ghi abc" X " jkl");
    KEYS("3N", "abc" N "def" N "abc" N X "ghi abc jkl");
    KEYS("2N", "abc" N X "def" N "abc" N "ghi abc jkl");

    data.setText("a.b.c" N "" N "d.e.f");
    KEYS("/[a-z]*<CR>", "a" X ".b.c" N "" N "d.e.f");
    KEYS("n", "a." X "b.c" N "" N "d.e.f");
    KEYS("2n", "a.b." X "c" N "" N "d.e.f");
    KEYS("n", "a.b.c" N X "" N "d.e.f");
    KEYS("n", "a.b.c" N "" N X "d.e.f");
    KEYS("2N", "a.b." X "c" N "" N "d.e.f");
    KEYS("2n", "a.b.c" N "" N X "d.e.f");

    // find same stuff forward and backward,
    // i.e. '<ab>c' forward but not 'a<bc>' backward
    data.setText("abc" N "def" N "ghi");
    KEYS("/\\w\\{2}<CR>", "abc" N X "def" N "ghi");
    KEYS("n", "abc" N "def" N X "ghi");
    KEYS("N", "abc" N X "def" N "ghi");
    KEYS("N", X "abc" N "def" N "ghi");
    KEYS("2n2N", X "abc" N "def" N "ghi");
}

void FakeVimPlugin::test_vim_indent()
{
    TestData data;
    setup(&data);

    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=4");

    data.setText(
        "abc" N
        "def" N
        "ghi" N
        "jkl" N
        "mno");
    KEYS("j3>>",
        "abc" N
        "    " X "def" N
        "    ghi" N
        "    jkl" N
        "mno");
    KEYS("j2>>",
        "abc" N
        "    def" N
        "        " X "ghi" N
        "        jkl" N
        "mno");

    KEYS("2<<",
        "abc" N
        "    def" N
        "    " X "ghi" N
        "    jkl" N
        "mno");
    INTEGRITY(false);
    KEYS("k3<<",
        "abc" N
        X "def" N
        "ghi" N
        "jkl" N
        "mno");

    data.setText(
        "abc" N
        "def" N
        "ghi" N
        "jkl" N
        "mno");
    KEYS("jj>j",
        "abc" N
        "def" N
        "    " X "ghi" N
        "    jkl" N
        "mno");

    data.setText("abc");
    KEYS(">>", "    " X "abc");
    INTEGRITY(false);

    data.setText("abc");
    data.doCommand("set shiftwidth=2");
    KEYS(">>", "  " X "abc");

    data.setText("abc");
    data.doCommand("set noexpandtab");
    data.doCommand("set tabstop=2");
    data.doCommand("set shiftwidth=7");
    // shiftwidth = TABS * tabstop + SPACES
    //          7 = 3    * 2       + 1
    KEYS(">>", "\t\t\t abc");

    data.doCommand("set tabstop=3");
    data.doCommand("set shiftwidth=7");
    data.setText("abc");
    KEYS(">>", "\t\t abc");
    INTEGRITY(false);
}

void FakeVimPlugin::test_vim_marks()
{
    TestData data;
    setup(&data);

    data.setText("  abc" N "  def" N "  ghi");
    data.doKeys("ma");
    data.doKeys("ma");
    data.doKeys("jmb");
    data.doKeys("j^mc");
    KEYS("'a",   "  " X "abc" N   "  "   "def" N   "  "   "ghi");
    KEYS("`a", X "  "   "abc" N   "  "   "def" N   "  "   "ghi");
    KEYS("`b",   "  "   "abc" N X "  "   "def" N   "  "   "ghi");
    KEYS("'b",   "  "   "abc" N   "  " X "def" N   "  "   "ghi");
    KEYS("`c",   "  "   "abc" N   "  "   "def" N   "  " X "ghi");
    KEYS("'c",   "  "   "abc" N   "  "   "def" N   "  " X "ghi");

    KEYS("`b",   "  "   "abc" N X "  "   "def" N   "  "   "ghi");
    KEYS("'c",   "  "   "abc" N   "  "   "def" N   "  " X "ghi");

    KEYS("`'",   "  "   "abc" N X "  "   "def" N   "  "   "ghi");
    KEYS("`a", X "  "   "abc" N   "  "   "def" N   "  "   "ghi");
    KEYS("''",   "  "   "abc" N   "  " X "def" N   "  "   "ghi");
    KEYS("`'", X "  "   "abc" N   "  "   "def" N   "  "   "ghi");
    KEYS("`'",   "  "   "abc" N   "  " X "def" N   "  "   "ghi");
}

void FakeVimPlugin::test_vim_jumps()
{
    TestData data;
    setup(&data);

    // last position
    data.setText("  abc" N "  def" N "  ghi");
    KEYS("G", "  abc" N "  def" N "  " X "ghi");
    KEYS("`'", X "  abc" N "  def" N "  ghi");
    KEYS("`'", "  abc" N "  def" N "  " X "ghi");
    KEYS("''", "  " X "abc" N "  def" N "  ghi");
    KEYS("<C-O>", "  abc" N "  def" N "  " X "ghi");
    KEYS("<C-I>", "  " X "abc" N "  def" N "  ghi");

    KEYS("lgUlhj", "  aBc" N "  " X "def" N "  ghi");
    KEYS("`.", "  a" X "Bc" N "  def" N "  ghi");
    KEYS("`'", "  aBc" N "  " X "def" N "  ghi");
    KEYS("'.", "  " X "aBc" N "  def" N "  ghi");
    KEYS("G", "  aBc" N "  def" N "  " X "ghi");
    KEYS("u", "  a" X "bc" N "  def" N "  ghi");
    KEYS("`'", "  abc" N "  def" N "  " X "ghi");
    KEYS("<c-r>", "  a" X "Bc" N "  def" N "  ghi");
    KEYS("jd$", "  aBc" N "  " X "d" N "  ghi");
    KEYS("''", "  aBc" N "  d" N "  " X "ghi");
    KEYS("`'", "  aBc" N "  " X "d" N "  ghi");
    KEYS("u", "  aBc" N "  d" X "ef" N "  ghi");
    KEYS("''", "  aBc" N "  " X "def" N "  ghi");
    KEYS("`'", "  aBc" N "  d" X "ef" N "  ghi");
}

void FakeVimPlugin::test_vim_copy_paste()
{
    TestData data;
    setup(&data);

    data.setText("123" N "456");
    KEYS("llyy2P", X "123" N "123" N "123" N "456");

    data.setText("123" N "456");
    KEYS("yyp", "123" N X "123" N "456");
    KEYS("2p", "123" N "123" N X "123" N "123" N "456");
    INTEGRITY(false);

    data.setText("123 456");
    KEYS("yw2P", "123 123" X " 123 456");
    KEYS("2p", "123 123 123 123" X " 123 456");

    data.setText("123" N "456");
    KEYS("2yyp", "123" N X "123" N "456" N "456");

    data.setText("123" N "456");
    KEYS("2yyP", X "123" N "456" N "123" N "456");

    data.setText("123" N "456" N "789");
    KEYS("ddp", "456" N X "123" N "789");

    // block-select middle column, copy and paste twice
    data.setText("123" N "456");
    KEYS("l<C-v>j\"xy2\"xp", "12" X "223" N "45556");

    data.setText("123" N "456" N "789");
    KEYS("wyiw" "wviwp", "123" N "456" N "45" X "6");

    // QTCREATORBUG-8148
    data.setText("abc");
    KEYS("yyp", "abc" N X "abc");
    KEYS("4p", "abc" N "abc" N X "abc" N "abc" N "abc" N "abc");
}

void FakeVimPlugin::test_vim_undo_redo()
{
    TestData data;
    setup(&data);

    data.setText("abc def" N "xyz" N "123");
    KEYS("ddu", X "abc def" N "xyz" N "123");
    COMMAND("redo", X "xyz" N "123");
    COMMAND("undo", X "abc def" N "xyz" N "123");
    COMMAND("redo", X "xyz" N "123");
    KEYS("dd", X "123");
    KEYS("3x", X "");
    KEYS("uuu", X "abc def" N "xyz" N "123");
    KEYS("<C-r>", X "xyz" N "123");
    KEYS("2<C-r>", X "");
    KEYS("3u", X "abc def" N "xyz" N "123");

    KEYS("wved", "abc" X " " N "xyz" N "123");
    KEYS("2w", "abc " N "xyz" N X "123");
    KEYS("u", "abc " X "def" N "xyz" N "123");
    KEYS("<C-r>", "abc" X " " N "xyz" N "123");
    KEYS("10ugg", X "abc def" N "xyz" N "123");

    KEYS("A xxx<ESC>", "abc def xx" X "x" N "xyz" N "123");
    KEYS("A yyy<ESC>", "abc def xxx yy" X "y" N "xyz" N "123");
    KEYS("u", "abc def xx" X "x" N "xyz" N "123");
    KEYS("u", "abc de" X "f" N "xyz" N "123");
    KEYS("<C-r>", "abc def" X " xxx" N "xyz" N "123");
    KEYS("<C-r>", "abc def xxx" X " yyy" N "xyz" N "123");

    KEYS("izzz<ESC>", "abc def xxxzz" X "z yyy" N "xyz" N "123");
    KEYS("<C-r>", "abc def xxxzz" X "z yyy" N "xyz" N "123");
    KEYS("u", "abc def xxx" X " yyy" N "xyz" N "123");

    data.setText("abc" N X "def");
    KEYS("oxyz<ESC>", "abc" N "def" N "xy" X "z");
    KEYS("u", "abc" N X "def");

    // undo paste lines
    data.setText("abc" N);
    KEYS("yy2p", "abc" N X "abc" N "abc" N);
    KEYS("yy3p", "abc" N "abc" N X "abc" N "abc" N "abc" N "abc" N);
    KEYS("u", "abc" N X "abc" N "abc" N);
    KEYS("u", X "abc" N);
    KEYS("<C-r>", X "abc" N "abc" N "abc" N);
    KEYS("<C-r>", "abc" N X "abc" N "abc" N "abc" N "abc" N "abc" N);
    KEYS("u", "abc" N X "abc" N "abc" N);
    KEYS("u", X "abc" N);

    // undo paste block
    data.setText("abc" N "def" N "ghi");
    KEYS("<C-v>jyp", "a" X "abc" N "ddef" N "ghi");
    KEYS("2p", "aa" X "aabc" N "ddddef" N "ghi");
    KEYS("3p", "aaa" X "aaaabc" N "dddddddef" N "ghi");
    KEYS("u", "aa" X "aabc" N "ddddef" N "ghi");
    KEYS("u", "a" X "abc" N "ddef" N "ghi");

    // undo indent
    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=4");
    data.setText("abc" N "def");
    KEYS(">>", "    " X "abc" N "def");
    KEYS(">>", "        " X "abc" N "def");
    KEYS("<<", "    " X "abc" N "def");
    KEYS("<<", X "abc" N "def");
    KEYS("u", "    " X "abc" N "def");
    KEYS("u", "        " X "abc" N "def");
    KEYS("u", "    " X "abc" N "def");
    KEYS("u", X "abc" N "def");
    KEYS("<C-r>", X "    abc" N "def");
    KEYS("<C-r>", "    " X "    abc" N "def");
    KEYS("<C-r>", "    ab" X "c" N "def");
    KEYS("<C-r>", "ab" X "c" N "def");
    KEYS("<C-r>", "ab" X "c" N "def");

    data.setText("abc" N "def");
    KEYS("2>>", "    " X "abc" N "    def");
    KEYS("u", X "abc" N "def");
    KEYS("<c-r>", X "    abc" N "    def");
    KEYS("u", X "abc" N "def");
    KEYS(">j", "    " X "abc" N "    def");
    KEYS("u", X "abc" N "def");
    KEYS("<c-r>", X "    abc" N "    def");

    // undo replace line
    data.setText("abc" N "  def" N "ghi");
    KEYS("jlllSxyz<ESC>", "abc" N "xyz" N "ghi");
    KEYS("u", "abc" N "  " X "def" N "ghi");
}

void FakeVimPlugin::test_vim_letter_case()
{
    TestData data;
    setup(&data);

    // upper- and lower-case
    data.setText("abc DEF");
    KEYS("lv3l~", "a" X "BC dEF");
    KEYS("v4lU", "a" X "BC DEF");
    KEYS("v4$u", "a" X "bc def");
    KEYS("v4$gU", "a" X "BC DEF");
    KEYS("gu$", "a" X "bc def");
    KEYS("lg~~", X "ABC DEF");
    KEYS(".", X "abc def");
    KEYS("gUiw", X "ABC def");

    data.setText("  ab" X "c" N "def");
    KEYS("2gUU", "  " X "ABC" N "DEF");
    KEYS("u", "  " X "abc" N "def");
    KEYS("<c-r>", "  " X "ABC" N "DEF");
}

void FakeVimPlugin::test_vim_code_autoindent()
{
    TestData data;
    setup(&data);

    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=3");

    data.setText("int main()" N
         X "{" N
         "}" N
         "");
    KEYS("o" "return 0;",
         "int main()" N
         "{" N
         "   return 0;" X N
         "}" N
         "");
    INTEGRITY(false);
    KEYS("O" "int i = 0;",
         "int main()" N
         "{" N
         "   int i = 0;" X N
         "   return 0;" N
         "}" N
         "");
    INTEGRITY(false);
    KEYS("ddO" "int i = 0;" N "int j = 0;",
         "int main()" N
         "{" N
         "   int i = 0;" N
         "   int j = 0;" X N
         "   return 0;" N
         "}" N
         "");
    KEYS("^i" "int x = 1;" N,
         "int main()" N
         "{" N
         "   int i = 0;" N
         "   int x = 1;" N
         "   " X "int j = 0;" N
         "   return 0;" N
         "}" N
         "");
    KEYS("c2k" "if (true) {" N ";" N "}",
         "int main()" N
         "{" N
         "   if (true) {" N
         "      ;" N
         "   }" X N
         "   return 0;" N
         "}" N
         "");
    KEYS("jci{" "return 1;",
         "int main()" N
         "{" N
         "   return 1;" X N
         "}" N
         "");
    KEYS("di{",
         "int main()" N
         "{" N
         X "}" N
         "");
    INTEGRITY(false);

    // autoindent
    data.doCommand("set nosmartindent");
    data.setText("abc" N "def");
    KEYS("3o 123<esc>", "abc" N " 123" N "  123" N "   12" X "3" N "def");
    INTEGRITY(false);

    data.setText("abc" N "def");
    KEYS("3O 123<esc>", " 123" N "  123" N "   12" X "3" N "abc" N "def");
    INTEGRITY(false);
    data.doCommand("set smartindent");
}

void FakeVimPlugin::test_vim_code_folding()
{
    TestData data;
    setup(&data);

    data.setText("int main()" N "{" N "    return 0;" N "}" N "");

    // fold/unfold function block
    data.doKeys("zc");
    QCOMPARE(data.lines(), 2);
    data.doKeys("zo");
    QCOMPARE(data.lines(), 5);
    data.doKeys("za");
    QCOMPARE(data.lines(), 2);

    // delete whole block
    KEYS("dd", "");

    // undo/redo
    KEYS("u", "int main()" N "{" N "    return 0;" N "}" N "");
    KEYS("<c-r>", "");

    // change block
    KEYS("uggzo", X "int main()" N "{" N "    return 0;" N "}" N "");
    KEYS("ccvoid f()<esc>", "void f(" X ")" N "{" N "    return 0;" N "}" N "");
    KEYS("uzc.", "void f(" X ")" N "");

    // open/close folds recursively
    data.setText("int main()" N
         "{" N
         "    if (true) {" N
         "        return 0;" N
         "    } else {" N
         "        // comment" N
         "        " X "return 2" N
         "    }" N
         "}" N
         "");
    int lines = data.lines();
    // close else block
    data.doKeys("zc");
    QCOMPARE(data.lines(), lines - 3);
    // close function block
    data.doKeys("zc");
    QCOMPARE(data.lines(), lines - 8);
    // jumping to a line opens all its parent folds
    data.doKeys("6gg");
    QCOMPARE(data.lines(), lines);

    // close recursively
    data.doKeys("zC");
    QCOMPARE(data.lines(), lines - 8);
    data.doKeys("za");
    QCOMPARE(data.lines(), lines - 3);
    data.doKeys("6gg");
    QCOMPARE(data.lines(), lines);
    data.doKeys("zA");
    QCOMPARE(data.lines(), lines - 8);
    data.doKeys("za");
    QCOMPARE(data.lines(), lines - 3);

    // close all folds
    data.doKeys("zM");
    QCOMPARE(data.lines(), lines - 8);
    data.doKeys("zo");
    QCOMPARE(data.lines(), lines - 4);
    data.doKeys("zM");
    QCOMPARE(data.lines(), lines - 8);

    // open all folds
    data.doKeys("zR");
    QCOMPARE(data.lines(), lines);

    // delete folded lined if deleting to the end of the first folding line
    data.doKeys("zMgg");
    QCOMPARE(data.lines(), lines - 8);
    KEYS("wwd$", "int main" N "");

    // undo
    KEYS("u", "int main" X "()" N
         "{" N
         "    if (true) {" N
         "        return 0;" N
         "    } else {" N
         "        // comment" N
         "        return 2" N
         "    }" N
         "}" N
         "");

    NOT_IMPLEMENTED
    // Opening folds recursively isn't supported (previous position in fold isn't restored).
}

void FakeVimPlugin::test_vim_substitute()
{
    TestData data;
    setup(&data);

    data.setText("abcabc");
    COMMAND("s/abc/123/", X "123abc");
    COMMAND("u", X "abcabc");
    COMMAND("s/abc/123/g", X "123123");
    COMMAND("u", X "abcabc");

    data.setText("abc" N "def");
    COMMAND("%s/^/ -- /", " -- abc" N " " X "-- def");
    COMMAND("u", X "abc" N "def");

    data.setText("  abc" N "  def");
    COMMAND("%s/$/./", "  abc." N "  " X "def.");

    data.setText("abc" N "def");
    COMMAND("%s/.*/(&)", "(abc)" N X "(def)");
    COMMAND("u", X "abc" N "def");
    COMMAND("%s/.*/X/g", "X" N X "X");

    data.setText("abc" N "" N "def");
    COMMAND("%s/^\\|$/--", "--abc" N "--" N X "--def");
    COMMAND("u", X "abc" N "" N "def");
    COMMAND("%s/^\\|$/--/g", "--abc--" N "--" N X "--def--");

    // captures
    data.setText("abc def ghi");
    COMMAND("s/\\w\\+/'&'/g", X "'abc' 'def' 'ghi'");
    COMMAND("u", X "abc def ghi");
    COMMAND("s/\\w\\+/'\\&'/g", X "'&' '&' '&'");
    COMMAND("u", X "abc def ghi");
    COMMAND("s/\\(\\w\\{3}\\)/(\\1)/g", X "(abc) (def) (ghi)");
    COMMAND("u", X "abc def ghi");
    COMMAND("s/\\(\\w\\{3}\\) \\(\\w\\{3\\}\\)/\\2 \\1 \\\\1/g", X "def abc \\1 ghi");

    // case-insensitive
    data.setText("abc ABC abc");
    COMMAND("s/ABC/123/gi", X "123 123 123");

    // replace on a line
    data.setText("abc" N "def" N "ghi");
    COMMAND("2s/^/ + /", "abc" N " " X "+ def" N "ghi");
    COMMAND("1s/^/ * /", " " X "* abc" N " + def" N "ghi");
    COMMAND("$s/^/ - /", " * abc" N " + def" N " " X "- ghi");

    // replace on lines
    data.setText("abc" N "def" N "ghi");
    COMMAND("2,$s/^/ + /", "abc" N " + def" N " " X "+ ghi");
    COMMAND("1,2s/^/ * /", " * abc" N " " X "*  + def" N " + ghi");
    COMMAND("3,3s/^/ - /", " * abc" N " *  + def" N " " X "-  + ghi");
    COMMAND("%s/\\( \\S \\)*//g", "abc" N "def" N X "ghi");

    // last substitution
    data.setText("abc" N "def" N "ghi");
    COMMAND("%s/DEF/+&/i", "abc" N X "+def" N "ghi");
    COMMAND("&&", "abc" N X "++def" N "ghi");
    COMMAND("&", "abc" N X "++def" N "ghi");
    COMMAND("&&", "abc" N X "++def" N "ghi");
    COMMAND("&i", "abc" N X "+++def" N "ghi");
    COMMAND("s", "abc" N X "+++def" N "ghi");
    COMMAND("&&i", "abc" N X "++++def" N "ghi");

    // search for last substitute pattern
    data.setText("abc" N "def" N "ghi");
    COMMAND("%s/def/def", "abc" N X "def" N "ghi");
    KEYS("gg", X "abc" N "def" N "ghi");
    COMMAND("\\&", "abc" N X "def" N "ghi");

    // substitute last selection
    data.setText("abc" N "def" N "ghi" N "jkl");
    KEYS("jVj:s/^/*<CR>", "abc" N "*def" N X "*ghi" N "jkl");
    COMMAND("'<,'>s/^/*", "abc" N "**def" N X "**ghi" N "jkl");
    KEYS("ugv:s/^/+<CR>", "abc" N "+*def" N X "+*ghi" N "jkl");
}

void FakeVimPlugin::test_vim_ex_yank()
{
    TestData data;
    setup(&data);

    data.setText("abc" N "def");
    COMMAND("y x", X "abc" N "def");
    KEYS("\"xp", "abc" N X "abc" N "def");
    COMMAND("u", X "abc" N "def");
    COMMAND("redo", X "abc" N "abc" N "def");

    KEYS("uw", "abc" N X "def");
    COMMAND("1y y", "abc" N X "def");
    KEYS("\"yP", "abc" N X "abc" N "def");
    COMMAND("u", "abc" N X "def");

    COMMAND("-1,$y x", "abc" N X "def");
    KEYS("\"xP", "abc" N X "abc" N "def" N "def");
    COMMAND("u", "abc" N X "def");

    COMMAND("$-1y", "abc" N X "def");
    KEYS("P", "abc" N X "abc" N "def");
    COMMAND("u", "abc" N X "def");
}

void FakeVimPlugin::test_vim_ex_delete()
{
    TestData data;
    setup(&data);

    data.setText("abc" N X "def" N "ghi" N "jkl");
    COMMAND("d", "abc" N X "ghi" N "jkl");
    COMMAND("1,2d", X "jkl");
    COMMAND("u", X "abc" N "ghi" N "jkl");
    COMMAND("u", "abc" N X "def" N "ghi" N "jkl");
    KEYS("p", "abc" N "def" N X "abc" N "ghi" N "ghi" N "jkl");
    COMMAND("set ws|" "/abc/,/ghi/d|" "set nows", X "ghi" N "jkl");
    COMMAND("u", X "abc" N "def" N "abc" N "ghi" N "ghi" N "jkl");
    COMMAND("2,/abc/d3", "abc" N "def" N X "jkl");
    COMMAND("u", "abc" N "def" N X "abc" N "ghi" N "ghi" N "jkl");
    COMMAND("5,.+1d", "abc" N "def" N "abc" N X "jkl");
}

void FakeVimPlugin::test_vim_ex_change()
{
    TestData data;
    setup(&data);

    data.setText("abc" N X "def" N "ghi" N "jkl");
    KEYS(":c<CR>xxx<ESC>0", "abc" N X "xxx" N "ghi" N "jkl");
    KEYS(":-1,+1c<CR>XXX<ESC>0", X "XXX" N "jkl");
}

void FakeVimPlugin::test_vim_ex_shift()
{
    TestData data;
    setup(&data);

    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=2");

    data.setText("abc" N X "def" N "ghi" N "jkl");
    COMMAND(">", "abc" N "  " X "def" N "ghi" N "jkl");
    COMMAND(">>", "abc" N "      " X "def" N "ghi" N "jkl");
    COMMAND("<", "abc" N "    " X "def" N "ghi" N "jkl");
    COMMAND("<<", "abc" N X "def" N "ghi" N "jkl");
}

void FakeVimPlugin::test_vim_ex_move()
{
    TestData data;
    setup(&data);

    data.setText("abc" N "def" N "ghi" N "jkl");
    COMMAND("m +1", "def" N X "abc" N "ghi" N "jkl");
    COMMAND("u", X "abc" N "def" N "ghi" N "jkl");
    COMMAND("redo", X "def" N "abc" N "ghi" N "jkl");
    COMMAND("m -2", X "def" N "abc" N "ghi" N "jkl");
    COMMAND("2m0", X "abc" N "def" N "ghi" N "jkl");

    COMMAND("m $-2", "def" N X "abc" N "ghi" N "jkl");
    KEYS("`'", X "def" N "abc" N "ghi" N "jkl");
    KEYS("Vj:m+2<cr>", "ghi" N "def" N X "abc" N "jkl");
    KEYS("u", X "def" N "abc" N "ghi" N "jkl");

    // move visual selection with indentation
    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=2");
    data.doCommand("vnoremap <C-S-J> :m'>+<CR>gv=");
    data.doCommand("vnoremap <C-S-K> :m-2<CR>gv=");
    data.setText(
         "int x;" N
         "int y;" N
         "int main() {" N
         "  if (true) {" N
         "  }" N
         "}" N
         "");
    KEYS("Vj<C-S-J>",
         "int main() {" N
         "  int x;" N
         "  int y;" N
         "  if (true) {" N
         "  }" N
         "}" N
         "");
    KEYS("gv<C-S-J>",
         "int main() {" N
         "  if (true) {" N
         "    int x;" N
         "    int y;" N
         "  }" N
         "}" N
         "");
    KEYS("gv<C-S-K>",
         "int main() {" N
         "  int x;" N
         "  int y;" N
         "  if (true) {" N
         "  }" N
         "}" N
         "");
    data.doCommand("vunmap <C-S-K>");
    data.doCommand("vunmap <C-S-J>");
}

void FakeVimPlugin::test_vim_ex_join()
{
    TestData data;
    setup(&data);

    data.setText("  abc" N X "  def" N "  ghi" N "  jkl");
    COMMAND("j", "  abc" N "  " X "def ghi" N "  jkl");
    COMMAND("u", "  abc" N X "  def" N "  ghi" N "  jkl");
    COMMAND("1j3", "  " X "abc def ghi" N "  jkl");
    COMMAND("u", X "  abc" N "  def" N "  ghi" N "  jkl");
}

void FakeVimPlugin::test_advanced_commands()
{
    TestData data;
    setup(&data);

    // subcommands
    data.setText("abc" N "  xxx" N "  xxx" N "def");
    COMMAND("%s/xxx/ZZZ/g|%s/ZZZ/OOO/g", "abc" N "  OOO" N "  " X "OOO" N "def");

    // undo/redo all subcommands
    COMMAND(":undo", "abc" N X "  xxx" N "  xxx" N "def");
    COMMAND(":redo", "abc" N X "  OOO" N "  OOO" N "def");

    // redundant characters
    COMMAND(" :::   %s/\\S\\S\\S/ZZZ/g   |"
        "  :: :  :   %s/ZZZ/XXX/g ", "XXX" N "  XXX" N "  XXX" N X "XXX");

    // bar character in regular expression is not command separator
    data.setText("abc");
    COMMAND("%s/a\\|b\\||/X/g|%s/[^X]/Y/g", "XXY");
}

void FakeVimPlugin::test_map()
{
    TestData data;
    setup(&data);

    data.setText("abc def");
    data.doCommand("map C i<space>x<space><esc>");
    data.doCommand("map c iXXX");
    data.doCommand("imap c YYY<space>");
    KEYS("C", " x" X " abc def");
    data.doCommand("map C <nop>");
    KEYS("C", " x" X " abc def");
    data.doCommand("map C i<bs><esc><right>");
    KEYS("C", " " X " abc def");
    KEYS("ccc<esc>", " XXXYYY YYY" X "  abc def");
    // unmap
    KEYS(":unmap c<cr>ccc<esc>", "YYY" X " ");
    KEYS(":iunmap c<cr>ccc<esc>", X "c");
    data.doCommand("unmap C");

    data.setText("abc def");
    data.doCommand("imap x (((<space><right><right>)))<esc>");
    KEYS("x", X "bc def");
    KEYS("ix", "((( bc))" X ") def");
    data.doCommand("iunmap x");

    data.setText("abc def");
    data.doCommand("map <c-right> 3l");
    KEYS("<C-Right>", "abc" X " def");
    KEYS("<C-Right>", "abc de" X "f");

    // map vs. noremap
    data.setText("abc def");
    data.doCommand("map x 3l");
    data.doCommand("map X x");
    KEYS("X", "abc" X " def");
    data.doCommand("noremap X x");
    KEYS("X", "abc" X "def");
    data.doCommand("unmap X");
    data.doCommand("unmap x");

    // limit number of recursions in mappings
    data.doCommand("map X Y");
    data.doCommand("map Y Z");
    data.doCommand("map Z X");
    KEYS("X", "abc" X "def");
    data.doCommand("map Z i<space><esc>");
    KEYS("X", "abc" X " def");
    data.doCommand("unmap X");
    data.doCommand("unmap Y");
    data.doCommand("unmap Z");

    // imcomplete mapping
    data.setText("abc");
    data.doCommand("map  Xa  ia<esc>");
    data.doCommand("map  Xb  ib<esc>");
    data.doCommand("map  X   ic<esc>");
    KEYS("Xa", X "aabc");
    KEYS("Xb", X "baabc");
    KEYS("Xic<esc>", X "ccbaabc");

    // unmap
    data.doCommand("unmap  Xa");
    KEYS("Xa<esc>", X "cccbaabc");
    data.doCommand("unmap  Xb");
    KEYS("Xb", X "ccccbaabc");
    data.doCommand("unmap  X");
    KEYS("Xb", X "ccccbaabc");
    KEYS("X<esc>", X "ccccbaabc");

    // recursive mapping
    data.setText("abc");
    data.doCommand("map  X    Y");
    data.doCommand("map  XXX  i1<esc>");
    data.doCommand("map  Y    i2<esc>");
    data.doCommand("map  YZ   i3<esc>");
    data.doCommand("map  _    i <esc>");
    KEYS("_XXX_", X " 1 abc");
    KEYS("XX_0", X " 22 1 abc");
    KEYS("XXXXZ_0", X " 31 22 1 abc");
    KEYS("XXXXX_0", X " 221 31 22 1 abc");
    KEYS("XXZ", X "32 221 31 22 1 abc");
    data.doCommand("unmap  X");
    data.doCommand("unmap  XXX");
    data.doCommand("unmap  Y");
    data.doCommand("unmap  YZ");
    data.doCommand("unmap  _");

    // shift modifier
    data.setText("abc");
    data.doCommand("map  x  i1<esc>");
    data.doCommand("map  X  i2<esc>");
    KEYS("x", X "1abc");
    KEYS("X", X "21abc");
    data.doCommand("map  <S-X>  i3<esc>");
    KEYS("X", X "321abc");
    data.doCommand("map  X  i4<esc>");
    KEYS("X", X "4321abc");
    KEYS("x", X "14321abc");
    data.doCommand("unmap  x");
    data.doCommand("unmap  X");

    // undo/redo mapped input
    data.setText("abc def ghi");
    data.doCommand("map X dwea xyz<esc>3l");
    KEYS("X", "def xyz g" X "hi");
    KEYS("u", X "abc def ghi");
    KEYS("<C-r>", X "def xyz ghi");
    data.doCommand("unmap  X");

    data.setText("abc" N "  def" N "  ghi");
    data.doCommand("map X jdd");
    KEYS("X", "abc" N "  " X "ghi");
    KEYS("u", "abc" N X "  def" N "  ghi");
    KEYS("<c-r>", "abc" N X "  ghi");
    data.doCommand("unmap  X");

    data.setText("abc" N "def" N "ghi");
    data.doCommand("map X jAxxx<cr>yyy<esc>");
    KEYS("X", "abc" N "defxxx" N "yy" X "y" N "ghi");
    KEYS("u", "abc" N "de" X "f" N "ghi");
    KEYS("<c-r>", "abc" N "def" X "xxx" N "yyy" N "ghi");
    data.doCommand("unmap  X");

    /* QTCREATORBUG-7913 */
    data.setText("");
    data.doCommand("noremap l k|noremap k j|noremap j h");
    KEYS("ikkk<esc>", "kk" X "k");
    KEYS("rj", "kk" X "j");
    data.doCommand("unmap l|unmap k|unmap j");

    // bad mapping
    data.setText(X "abc" N "def");
    data.doCommand("map X Zxx");
    KEYS("X", X "abc" N "def");
    // cancelled mapping
    data.doCommand("map X fxxx");
    KEYS("X", X "abc" N "def");
    data.doCommand("map X ciXxx");
    KEYS("X", X "abc" N "def");
    data.doCommand("map Y Xxx");
    KEYS("Y", X "abc" N "def");
    data.doCommand("unmap X|unmap Y");


    NOT_IMPLEMENTED
    // <C-o>
    data.setText("abc def");
    data.doCommand("imap X <c-o>:%s/def/xxx/<cr>");
    KEYS("iX", "abc xxx");
}

void FakeVimPlugin::test_vim_command_cc()
{
    TestData data;
    setup(&data);

    data.setText("|123"  N  "456" N "789" N   "abc");
    KEYS("cc456<ESC>",  "45|6" N  "456" N   "789" N   "abc");
    KEYS("ccabc<Esc>",  "ab|c" N  "456" N   "789" N   "abc");
//    KEYS(".<Esc>",      "ab|c" N  "456" N   "789" N   "abc");
//    KEYS("j<Esc>",      "abc" N  "45|6" N   "789" N   "abc");
//    KEYS(".<Esc>",      "abc" N  "456" N   "78|9" N   "abc");
//    KEYS("kkk<Esc>",    "ab|c" N  "456" N   "789" N   "abc");
//    KEYS("3ccxyz<Esc>", "xy|z" N "abc");
}

void FakeVimPlugin::test_vim_command_cw()
{
    TestData data;
    setup(&data);
    data.setText(X "123 456");
    KEYS("cwx<Esc>", X "x 456");
}

void FakeVimPlugin::test_vim_command_cj()
{
    TestData data;
    setup(&data);
//    KEYS("j$",               cursor(l[1], -1));
//    KEYS("cj<Esc>",     l[0]+"\n@" + "\n" + lmid(3));
//    KEYS("P",               lmid(0,1)+"\n" + "@"+lmid(1,2)+"\n" + "\n" +  lmid(3));
//    KEYS("u",               l[0]+"\n@" + "\n" + lmid(3));

//    TestData data;
//    setup(&data);
//    KEYS("j$",               cursor(l[1], -1));
//    KEYS("cjabc<Esc>",  l[0]+"\nab@c\n" + lmid(3));
//    KEYS("u",               lmid(0,1)+"\n" + cursor(l[1], -1)+"\n" + lmid(2));
//    KEYS("gg",              "@" + lmid(0));
//    KEYS(".",               "ab@c\n" + lmid(2));
}

void FakeVimPlugin::test_vim_command_ck()
{
    TestData data;
    setup(&data);
//    KEYS("j$",               cursor(l[1], -1));
//    KEYS("ck<Esc>",     "@\n" + lmid(2));
//    KEYS("P",               "@" + lmid(0,2)+"\n" + "\n" + lmid(2));
}

void FakeVimPlugin::test_vim_command_c_dollar()
{
    TestData data;
    setup(&data);
//    KEYS("j",                "@" + l[1]);
//    KEYS("$",                cursor(l[1], -1));
//    KEYS("c$<Esc>",     l[0]+"\n" + l[1].left(l[1].length()-2)+"@"+l[1][l[1].length()-2]+"\n" + lmid(2));
//    KEYS("c$<Esc>",     l[0]+"\n" + l[1].left(l[1].length()-3)+"@"+l[1][l[1].length()-3]+"\n" + lmid(2));
//    KEYS("0c$abc<Esc>", l[0]+"\n" + "ab@c\n" + lmid(2));
//    KEYS("0c$abc<Esc>", l[0]+"\n" + "ab@c\n" + lmid(2));
}

void FakeVimPlugin::test_vim_command_C()
{
    TestData data;
    setup(&data);
//    KEYS("j",                "@" + l[1]);
//    KEYS("Cabc<Esc>",   l[0] + "\nab@c\n" + lmid(2));
//    KEYS("Cabc<Esc>",   l[0] + "\nabab@c\n" + lmid(2));
//    KEYS("$Cabc<Esc>",  l[0] + "\nababab@c\n" + lmid(2));
//    KEYS("0C<Esc>",     l[0] + "\n@\n" + lmid(2));
//    KEYS("0Cabc<Esc>",  l[0] + "\nab@c\n" + lmid(2));
}

void FakeVimPlugin::test_vim_command_dw()
{
    TestData data;
    setup(&data);
//    KEYS("dw",  "@#include <QtCore>\n" + lmid(2));
//    KEYS("dw",  "@include <QtCore>\n" + lmid(2));
//    KEYS("dw",  "@<QtCore>\n" + lmid(2));
//    KEYS("dw",  "@QtCore>\n" + lmid(2));
//    KEYS("dw",  "@>\n" + lmid(2));
//    KEYS("dw",  "@\n" + lmid(2)); // Real vim has this intermediate step, too
//    KEYKEYS("dw",  "@#include <QtGui>\n" + lmid(3));
//    KEYS("dw",  "@include <QtGui>\n" + lmid(3));
//    KEYS("dw",  "@<QtGui>\n" + lmid(3));
//    KEYS("dw",  "@QtGui>\n" + lmid(3));
//    KEYS("dw",  "@>\n" + lmid(3));
}

void FakeVimPlugin::test_vim_command_dd()
{
    TestData data;
    setup(&data);
//    KEYS("j",    "@" + l[1]);
//    KEYS("dd",  l[0] + "\n@" + lmid(2));
//    KEYS(".",   l[0] + "\n@" + lmid(3));
//    KEYS("3dd", l[0] + "\n    @QApplication app(argc, argv);\n" + lmid(7));
//    KEYS("4l",  l[0] + "\n    QApp@lication app(argc, argv);\n" + lmid(7));
//    KEYS("dd",  l[0] + "\n@" + lmid(7));
//    KEYS(".",   l[0] + "\n    @return app.exec();\n" + lmid(9));
//    KEYS("dd",  l[0] + "\n@" + lmid(9));
}

void FakeVimPlugin::test_vim_command_dd_2()
{
    TestData data;
    setup(&data);
//    KEYS("j",    "@" + l[1]);
//    KEYS("dd",  l[0] + "\n@" + lmid(2));
//    KEYS("p",   l[0] + "\n" + l[2] + "\n@" + l[1] + "\n" + lmid(3));
//    KEYS("u",   l[0] + "\n@" + lmid(2));
}

void FakeVimPlugin::test_vim_command_d_dollar()
{
    TestData data;
    setup(&data);
//    KEYS("j$",               cursor(l[1], -1));
//    KEYS("$d$",             l[0]+"\n" + l[1].left(l[1].length()-2)+"@"+l[1][l[1].length()-2]+"\n" + lmid(2));
//    KEYS("0d$",             l[0] + "\n"+"@\n" + lmid(2));
}

void FakeVimPlugin::test_vim_command_dj()
{
    TestData data;
    setup(&data);
//    KEYS("j$",               cursor(l[1], -1));
//    KEYS("dj",              l[0]+"\n@" + lmid(3));
//    KEYS("P",               lmid(0,1)+"\n" + "@"+lmid(1));
//    KEYS("0",                "@" + l[1]);
//    KEYS("dj",              l[0]+"\n@" + lmid(3));
//    KEYS("P",               lmid(0,1)+"\n" + "@"+lmid(1));
//    KEYS("05l",              l[1].left(5) + "@" + l[1].mid(5));
//    KEYS("dj",              l[0]+"\n@" + lmid(3));
//    KEYS("P",               lmid(0,1)+"\n" + "@"+lmid(1));
//    KEYS("dj",              l[0]+"\n@" + lmid(3));
//    KEYS("p",               lmid(0,1)+"\n" + lmid(3,1)+"\n" + "@"+lmid(1,2)+"\n" + lmid(4));
}

void FakeVimPlugin::test_vim_command_dk()
{
    TestData data;
    setup(&data);
//    KEYS("j$",               cursor(l[1], -1));
//    KEYS("dk",              "@" + lmid(2));
//    KEYS("P",               "@" + lmid(0));
//    KEYS("j0",               "@" + l[1]);
//    KEYS("dk",              "@" + lmid(2));
//    KEYS("P",               "@" + lmid(0));
//    KEYS("j05l",             l[1].left(5) + "@" + l[1].mid(5));
//    KEYS("dk",              "@" + lmid(2));
//    KEYS("P",               "@" + lmid(0));
//    KEYS("j05l",             l[1].left(5) + "@" + l[1].mid(5));
//    KEYS("dk",              "@" + lmid(2));
//    KEYS("p",               lmid(2,1)+"\n" + "@" + lmid(0,2)+"\n" + lmid(3));
}

void FakeVimPlugin::test_vim_command_dgg()
{
    TestData data;
    setup(&data);
//    KEYS("G",               lmid(0, l.size()-2)+"\n" "@"+lmid(l.size()-2));
//    KEYS("dgg",             "@");
//    KEYS("u",               "@" + lmid(0));
}

void FakeVimPlugin::test_vim_command_dG()
{
    TestData data;
    setup(&data);
//    KEYS("dG",              "@");
//    KEYS("u",               "@" + lmid(0));
//    KEYS("j",                "@" + l[1]);
//    KEYS("dG",              lmid(0,1)+"\n" + "@");
//    KEYS("u",               l[0]+"\n" + "@" + lmid(1));
//    KEYS("G",               lmid(0, l.size()-2)+"\n" + "@"+lmid(l.size()-2));
//    qWarning("FIXME");
//return;
//    // include movement to first column, as otherwise the result depends on the 'startofline' setting
//    KEYS("dG0",             lmid(0, l.size()-2)+"\n" + "@"+lmid(l.size()-2,1));
//    KEYS("dG0",             lmid(0, l.size()-3)+"\n" + "@"+lmid(l.size()-3,1));
}

void FakeVimPlugin::test_vim_command_D()
{
    TestData data;
    setup(&data);
//    KEYS("j",                "@" + l[1]);
//    KEYS("$D",              l[0]+"\n" + l[1].left(l[1].length()-2)+"@"+l[1][l[1].length()-2]+"\n" + lmid(2));
//    KEYS("0D",              l[0] + "\n@\n" + lmid(2));
}

void FakeVimPlugin::test_vim_command_dollar()
{
    TestData data;
    setup(&data);
//    KEYS("j$",               cursor(l[1], -1));
//    KEYS("j$",               cursor(l[2], -1));
//    KEYS("2j", "@)");
}

void FakeVimPlugin::test_vim_command_down()
{
    TestData data;
    setup(&data);
//    KEYS("j",  "@" + l[1]);
//    KEYS("3j", "@int main");
//    KEYS("4j", "@    return app.exec()");
}

void FakeVimPlugin::test_vim_command_dfx_down()
{
    TestData data;
    setup(&data);
//    KEYS("j4l",  l[0] + "\n#inc@lude <QtCore>\n" + lmid(2));
//    qWarning("FIXME");
//return;
//    KEYS("df ",  l[0] + "\n#inc@<QtCore>\n" + lmid(2));
//    KEYS("j",    l[0] + "\n#inc<QtCore>\n#inc@lude <QtGui>\n" + lmid(3));
//    KEYS(".",    l[0] + "\n#inc<QtCore>\n#inc@<QtGui>\n" + lmid(3));
//    KEYS("u",    l[0] + "\n#inc<QtCore>\n#inc@lude <QtGui>\n" + lmid(3));
//    KEYS("u",    l[0] + "\n#inc@lude <QtCore>\n" + lmid(2));
}

void FakeVimPlugin::test_vim_command_Cxx_down_dot()
{
    TestData data;
    setup(&data);
//    KEYS("j4l",          l[0] + "\n#inc@lude <QtCore>\n" + lmid(2));
//    KEYS("Cxx<Esc>", l[0] + "\n#incx@x\n" + lmid(2));
//    KEYS("j",            l[0] + "\n#incxx\n#incl@ude <QtGui>\n" + lmid(3));
//    KEYS(".",            l[0] + "\n#incxx\n#inclx@x\n" + lmid(3));
}

void FakeVimPlugin::test_vim_command_e()
{
    TestData data;
    setup(&data);
//    KEYS("e",  "@#include <QtCore");
//    KEYS("e",  "#includ@e <QtCore");
//    KEYS("e",  "#include @<QtCore");
//    KEYS("3e", "@#include <QtGui");
//    KEYS("e",  "#includ@e <QtGui");
//    KEYS("e",  "#include @<QtGui");
//    KEYS("e",  "#include <QtGu@i");
//    KEYS("4e", "int main@(int argc, char *argv[])");
//    KEYS("e",  "int main(in@t argc, char *argv[])");
//    KEYS("e",  "int main(int arg@c, char *argv[])");
//    KEYS("e",  "int main(int argc@, char *argv[])");
//    KEYS("e",  "int main(int argc, cha@r *argv[])");
//    KEYS("e",  "int main(int argc, char @*argv[])");
//    KEYS("e",  "int main(int argc, char *arg@v[])");
//    KEYS("e",  "int main(int argc, char *argv[]@)");
//    KEYS("e",  "@{");
//    KEYS("10k","@\n"); // home.
}

void FakeVimPlugin::test_vim_command_i()
{
    TestData data;
    setup(&data);

//    // empty insertion at start of document
//    KEYS("i<Esc>", "@" + lines);
//    KEYS("u", "@" + lines);

//    // small insertion at start of document
//    KEYS("ix<Esc>", "@x" + lines);
//    KEYS("u", "@" + lines);
//    checkEx("redo", "@x" + lines);
//    KEYS("u", "@" + lines);

//    // small insertion at start of document
//    KEYS("ixxx<Esc>", "xx@x" + lines);
//    KEYS("u", "@" + lines);

//    // combine insertions
//    KEYS("i1<Esc>", "@1" + lines);
//    KEYS("i2<Esc>", "@21" + lines);
//    KEYS("i3<Esc>", "@321" + lines);
//    KEYS("u",           "@21" + lines);
//    KEYS("u",           "@1" + lines);
//    KEYS("u",           "@" + lines);
//    KEYS("ia<Esc>", "@a" + lines);
//    KEYS("ibx<Esc>", "b@xa" + lines);
//    KEYS("icyy<Esc>", "bcy@yxa" + lines);
//    KEYS("u", "b@xa" + lines);
//    KEYS("u", "@a" + lines);
//    checkEx("redo", "b@xa" + lines);
//    KEYS("u", "@a" + lines);
}

void FakeVimPlugin::test_vim_command_left()
{
    TestData data;
    setup(&data);
//    KEYS("4j",  "@int main");
//    KEYS("h",   "@int main"); // no move over left border
//    KEYS("$",   "argv[]@)");
//    KEYS("h",   "argv[@])");
//    KEYS("3h",  "ar@gv[])");
//    KEYS("50h", "@int main");
}

void FakeVimPlugin::test_vim_command_r()
{
    TestData data;
    setup(&data);
//    KEYS("4j",   "@int main");
//    KEYS("$",    "int main(int argc, char *argv[]@)");
//    KEYS("rx",  lmid(0, 4) + "\nint main(int argc, char *argv[]@x\n" + lmid(5));
//    KEYS("2h",  lmid(0, 4) + "\nint main(int argc, char *argv@[]x\n" + lmid(5));
//    KEYS("4ra", lmid(0, 4) + "\nint main(int argc, char *argv@[]x\n" + lmid(5));
//    KEYS("3rb", lmid(0, 4) + "\nint main(int argc, char *argvbb@b\n" + lmid(5));
//    KEYS("2rc", lmid(0, 4) + "\nint main(int argc, char *argvbb@b\n" + lmid(5));
//    KEYS("h2rc",lmid(0, 4) + "\nint main(int argc, char *argvbc@c\n" + lmid(5));
}

void FakeVimPlugin::test_vim_command_right()
{
    TestData data;
    setup(&data);
//    KEYS("4j", "@int main");
//    KEYS("l", "i@nt main");
//    KEYS("3l", "int @main");
//    KEYS("50l", "argv[]@)");
}

void FakeVimPlugin::test_vim_command_up()
{
    TestData data;
    setup(&data);
//    KEYS("j", "@#include <QtCore");
//    KEYS("3j", "@int main");
//    KEYS("4j", "@    return app.exec()");
}

void FakeVimPlugin::test_vim_command_w()
{
    TestData data;
    setup(&data);
//    KEYS("w",   "@#include <QtCore");
//    KEYS("w",   "#@include <QtCore");
//    KEYS("w",   "#include @<QtCore");
//    KEYS("3w",  "@#include <QtGui");
//    KEYS("w",   "#@include <QtGui");
//    KEYS("w",   "#include @<QtGui");
//    KEYS("w",   "#include <@QtGui");
//    KEYS("4w",  "int @main(int argc, char *argv[])");
//    KEYS("w",  "int main@(int argc, char *argv[])");
//    KEYS("w",   "int main(@int argc, char *argv[])");
//    KEYS("w",   "int main(int @argc, char *argv[])");
//    KEYS("w",   "int main(int argc@, char *argv[])");
//    KEYS("w",   "int main(int argc, @char *argv[])");
//    KEYS("w",   "int main(int argc, char @*argv[])");
//    KEYS("w",   "int main(int argc, char *@argv[])");
//    KEYS("w",   "int main(int argc, char *argv@[])");
//    KEYS("w",   "@{");
}

void FakeVimPlugin::test_vim_command_yyp()
{
    TestData data;
    setup(&data);
//    KEYS("4j",   "@int main");
//    KEYS("yyp", lmid(0, 4) + "\n" + lmid(4, 1) + "\n@" + lmid(4));
}

void FakeVimPlugin::test_vim_command_y_dollar()
{
    TestData data;
    setup(&data);
//    KEYS("j",     "@" + l[1]);
//    KEYS("$y$p", l[0]+"\n"+ l[1]+"@>\n" + lmid(2));
//    KEYS("$y$p", l[0]+"\n"+ l[1]+">@>\n" + lmid(2));
//    KEYS("$y$P", l[0]+"\n"+ l[1]+">@>>\n" + lmid(2));
//    KEYS("$y$P", l[0]+"\n"+ l[1]+">>@>>\n" + lmid(2));
}

void FakeVimPlugin::test_vim_command_Yp()
{
    TestData data;
    setup(&data);
//    KEYS("4j",   "@int main");
//    KEYS("Yp", lmid(0, 4) + "\n" + lmid(4, 1) + "\n@" + lmid(4));
}

void FakeVimPlugin::test_vim_command_ma_yank()
{
    TestData data;
    setup(&data);
//    KEYS("4j",   "@int main");
//    KEYS("ygg", "@" + lmid(0));
//    KEYS("4j",   "@int main");
//    KEYS("p",    lmid(0,5) + "\n@" + lmid(0,4) +"\n" + lmid(4));

//    TestData data;
//    setup(&data);
//    KEYS("ma", "@" + lmid(0));
//    KEYS("4j",   "@int main");
//    KEYS("mb", lmid(0,4) + "\n@" + lmid(4));
//    KEYS("\"ay'a", "@" + lmid(0));
//    KEYS("'b", lmid(0,4) + "\n@" + lmid(4));
//    KEYS("\"ap", lmid(0,5) + "\n@" + lmid(0,4) +"\n" + lmid(4));
}

void FakeVimPlugin::test_vim_command_Gyyp()
{
    TestData data;
    setup(&data);
//    KEYS("G",   lmid(0, l.size()-2) + "\n@" + lmid(l.size()-2));
//    KEYS("yyp", lmid(0) + "@" + lmid(9, 1)+"\n");
}

void FakeVimPlugin::test_i_cw_i()
{
    TestData data;
    setup(&data);
//    KEYS("j",                "@" + l[1]);
//    KEYS("ixx<Esc>",    l[0] + "\nx@x" + lmid(1));
//    KEYS("cwyy<Esc>",   l[0] + "\nxy@y" + lmid(1));
//    KEYS("iaa<Esc>",    l[0] + "\nxya@ay" + lmid(1));
}

void FakeVimPlugin::test_vim_command_J()
{
    TestData data;
    setup(&data);
//    KEYS("4j4l",   "int @main");

//    KEYS("J", lmid(0, 5) + "@ " + lmid(5));
//    KEYS("u", lmid(0, 4) + "\nint @main(int argc, char *argv[])\n" + lmid(5));
//    checkEx("redo", lmid(0, 5) + "@ " + lmid(5));

//    KEYS("3J", lmid(0, 5) + " " + lmid(5, 1) + " " + lmid(6, 1).mid(4) + "@ " + lmid(7));
//    KEYS("uu", lmid(0, 4) + "\nint @main(int argc, char *argv[])\n" + lmid(5));
//    checkEx("redo", lmid(0, 5) + "@ " + lmid(5));
}

void FakeVimPlugin::test_vim_command_put_at_eol()
{
    TestData data;
    setup(&data);
//    KEYS("j$",               cursor(l[1], -1));
//    KEYS("y$",              lmid(0,1)+"\n" + cursor(l[1], -1)+"\n" + lmid(2));
//    KEYS("p",               lmid(0,2)+"@>\n" + lmid(2));
//    KEYS("p",               lmid(0,2)+">@>\n" + lmid(2));
//    KEYS("$",               lmid(0,2)+">@>\n" + lmid(2));
//    KEYS("P",               lmid(0,2)+">@>>\n" + lmid(2));
}

void FakeVimPlugin::test_vim_command_oO()
{
    TestData data;
    setup(&data);
//    KEYS("gg",              "@" + lmid(0));
//    KEYS("Ol1<Esc>",    "l@1\n" + lmid(0));
//    KEYS("gg",              "@l1\n" + lmid(0));
//    KEYS("ol2<Esc>",    "l1\n" "l@2\n" + lmid(0));
//    KEYS("G",               "l1\n" "l2\n" + lmid(0,l.size()-2)+"\n" + "@"+lmid(l.size()-2));
//    KEYS("G$",              "l1\n" "l2\n" + lmid(0,l.size()-2)+"\n" + "@"+lmid(l.size()-2));
//    KEYS("ol-1<Esc>",   "l1\n" "l2\n" + lmid(0) + "l-@1\n");
//    KEYS("G",               "l1\n" "l2\n" + lmid(0) + "@l-1\n");
//    KEYS("Ol-2<Esc>",   "l1\n" "l2\n" + lmid(0) + "l-@2\n" + "l-1\n");
}

void FakeVimPlugin::test_vim_command_x()
{
    TestData data;
    setup(&data);

//    KEYS("x", "@" + lmid(0));
//    KEYS("j$", cursor(l[1], -1));
//    KEYS("x", lmid(0,1)+"\n" + l[1].left(l[1].length()-2)+"@"+l[1].mid(l[1].length()-2,1)+"\n" + lmid(2));
}

void FakeVimPlugin::test_vim_visual_d()
{
    TestData data;
    setup(&data);

//    KEYS("vd", "@" + lmid(0));
//    KEYS("vx", "@" + lmid(0));
//    KEYS("vjd", "@" + lmid(1).mid(1));
//    KEYS("ugg", "@" + lmid(0)); // FIXME: cursor should be at begin of doc w/o gg
//    KEYS("j", "@" + l[1]);
//    KEYS("vd", lmid(0, 1)+"\n" + "@" + lmid(1).mid(1));
//    KEYS("u", lmid(0, 1)+"\n" + "@" + lmid(1));
//    KEYS("vx", lmid(0, 1)+"\n" + "@" + lmid(1).mid(1));
//    KEYS("u", lmid(0, 1)+"\n" + "@" + lmid(1));
//    KEYS("vhx", lmid(0, 1)+"\n" + "@" + lmid(1).mid(1));
//    KEYS("u", lmid(0, 1)+"\n" + "@" + lmid(1));
//    KEYS("vlx", lmid(0, 1)+"\n" + "@" + lmid(1).mid(2));
//    KEYS("P", lmid(0, 1)+"\n" + lmid(1).left(1)+"@"+lmid(1).mid(1));
//    KEYS("vhd", lmid(0, 1)+"\n" + "@" + lmid(1).mid(2));
//    KEYS("u0", lmid(0, 1)+"\n" + "@" + lmid(1)); // FIXME: cursor should be at begin of line w/o 0
//    qWarning("FIXME");
//return;
//    KEYS("v$d", lmid(0, 1)+"\n" + "@" + lmid(2));
//    KEYS("v$od", lmid(0, 1)+"\n" + "@" + lmid(3));
//    KEYS("$v$x", lmid(0, 1)+"\n" + lmid(3,1) + "@" + lmid(4));
//    KEYS("0v$d", lmid(0, 1)+"\n" + "@" + lmid(5));
//    KEYS("$v0d", lmid(0, 1)+"\n" + "@\n" + lmid(6));
//    KEYS("v$o0k$d", lmid(0, 1)+"\n" + "@" + lmid(6).mid(1));
}

void FakeVimPlugin::test_vim_Visual_d()
{
    TestData data;
    setup(&data);

//    KEYS("Vd", "@" + lmid(1));
//    KEYS("V2kd", "@" + lmid(2));
//    KEYS("u", "@" + lmid(1));
//    KEYS("u", "@" + lmid(0));
//    KEYS("j", "@" + l[1]);
//    KEYS("V$d", lmid(0,1)+"\n" + "@" + lmid(2));
//    KEYS("$V$$d", lmid(0,1)+"\n" + "@" + lmid(3));
//    KEYS("Vkx", "@" + lmid(4));
//    KEYS("P", "@" + lmid(0,1)+"\n" + lmid(3));
}


//bool FakeVimPlugin::checkContentsHelper(QString want, const char* file, int line)
//{
//    QString got = EDITOR(toPlainText());
//    int pos = EDITOR(textCursor().position());
//    got = got.left(pos) + "@" + got.mid(pos);
//    QStringList wantlist = want.split('\n');
//    QStringList gotlist = got.split('\n');
//    if (!QTest::qCompare(gotlist.size(), wantlist.size(), "", "", file, line)) {
//        qDebug() << "0 WANT: " << want;
//        qDebug() << "0 GOT: " << got;
//        return false;
//    }
//    for (int i = 0; i < wantlist.size() && i < gotlist.size(); ++i) {
//        QString g = QString("line %1: %2").arg(i + 1).arg(gotlist.at(i));
//        QString w = QString("line %1: %2").arg(i + 1).arg(wantlist.at(i));
//        if (!QTest::qCompare(g, w, "", "", file, line)) {
//            qDebug() << "1 WANT: " << want;
//            qDebug() << "1 GOT: " << got;
//            return false;
//        }
//    }
//    return true;
//}

//bool FakeVimPlugin::checkHelper(bool ex, QString cmd, QString expected,
//    const char *file, int line)
//{
//    if (ex)
//        data.doCommand((cmd);
//    else
//        send(cmd);
//    return checkContentsHelper(expected, file, line);
//}


//#define checkContents(expected) \
//    do { if (!checkContentsHelper(expected, __FILE__, __LINE__)) return; } while (0)

//// Runs a "normal" command and checks the result.
//// Cursor position is marked by a '@' in the expected contents.
//#define check(cmd, expected) \
//    do { if (!checkHelper(false, cmd, expected, __FILE__, __LINE__)) \
//            return; } while (0)

//#define move(cmd, expected) \
//    do { if (!checkHelper(false, cmd, insertCursor(expected), __FILE__, __LINE__)) \
//            return; } while (0)

//// Runs an ex command and checks the result.
//// Cursor position is marked by a '@' in the expected contents.
//#define checkEx(cmd, expected) \
//    do { if (!checkHelper(true, cmd, expected, __FILE__, __LINE__)) \
//            return; } while (0)

//QString FakeVimPlugin::insertCursor(const QString &needle0)
//{
//    QString needle = needle0;
//    needle.remove('@');
//    QString lines0 = lines;
//    int pos = lines0.indexOf(needle);
//    if (pos == -1)
//        qDebug() << "Cannot find: \n----\n" + needle + "\n----\n";
//    lines0.replace(pos, needle.size(), needle0);
//    return lines0;
//}

//QString FakeVimPlugin::cursor(const QString &line, int pos)
//{
//    if (pos < 0)
//        pos = line.length() + pos;

//    return line.left(pos) + "@" + line.mid(pos);
//}
