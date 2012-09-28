/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Lukas Holecek <hluk@email.cz>
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

/*!
 * Tests for FakeVim plugin.
 * All test are based on Vim behaviour.
 */

#include "fakevimplugin.h"
#include "fakevimhandler.h"

#include <QtTest>
#include <QTextEdit>
#include <QTextDocument>

/*!
 * Tests after this macro will be skipped and warning printed.
 * Uncomment it to test a feature -- if tests succeeds it should be removed from the test.
 */
#define NOT_IMPLEMENTED return;

//   QTest::qSkip("Not fully implemented!", QTest::SkipSingle, __FILE__, __LINE__);
//   return;

// text cursor representation in comparisons (set empty to disable cursor position checking)
#define X "|"
static const QString cursorString(X);

// a more distinct line separator in code
#define N "\n"

// document line start and end string in error text
#define LINE_START "\t\t<"
#define LINE_END ">\n"

// Compare document contents with a expectedText.
// Also check cursor position if the expectedText contains cursorString.
#define COMPARE(beforeText, beforePosition, afterText, afterPosition, expectedText, cmd) \
    do { \
        QString before(beforeText); \
        QString actual(afterText); \
        QString expected(expectedText); \
        if (!cursorString.isEmpty() && expected.contains(cursorString)) {\
            before.insert(beforePosition, cursorString); \
            actual.insert(afterPosition, cursorString); \
        } \
        QString help = "\n\tBefore command [" + QString(cmd) + "]:\n" LINE_START \
            + (before.replace('\n', LINE_END LINE_START)) \
            + LINE_END "\n\tAfter the command:\n" LINE_START \
            + actual.replace('\n', LINE_END LINE_START) \
            + LINE_END "\n\tShould be:\n" LINE_START \
            + expected.replace('\n', LINE_END LINE_START) + LINE_END; \
        QVERIFY2(actual == expected, help.toLatin1().constData()); \
    } while (false)

// Send keys and check if the expected result is same as document contents.
// Escape is always prepended to keys so that previous command is cancled.
#define KEYS(keys, expected) \
    do { \
        QString beforeText(data.text()); \
        int beforePosition = data.position(); \
        data.doKeys("<ESC>" keys); \
        COMPARE(beforeText, beforePosition, data.text(), data.position(), (expected), (keys)); \
    } while (false);

// Run Ex command and check if the expected result is same as document contents.
#define COMMAND(cmd, expected) \
    do { \
        QString beforeText(data.text()); \
        int beforePosition = data.position(); \
        data.doCommand(cmd); \
        COMPARE(beforeText, beforePosition, data.text(), data.position(), (expected), (":" cmd)); \
    } while (false);

using namespace FakeVim::Internal;

namespace {

struct TestData
{
    FakeVimHandler *handler;
    QTextEdit *edit;
    QWidget parent;

    QTextCursor cursor() const { return edit->textCursor(); }

    int position() const
    {
        int pos = cursor().position();
        // text cursor position is never behind last character in document
        return qMax(0, qMin(pos, edit->document()->characterCount() - 2));
    }

    QString text() const { return edit->toPlainText(); }

    void setText(const QString &text)
    {
        QString str = text;
        int i = str.indexOf(cursorString);
        if (!cursorString.isEmpty() && i != -1)
            str.remove(i, 1);
        edit->document()->setPlainText(str);
        handler->setTextCursorPosition(i);
    }

    void doCommand(const QString &cmd) { handler->handleCommand(cmd); }
    void doKeys(const QString &keys) { handler->handleInput(keys); }
};

static void setup(TestData *data)
{
    data->edit = new QTextEdit(&data->parent);
    data->handler = new FakeVimHandler(data->edit, &data->parent);
    data->handler->handleCommand("set startofline");
}

} // namespace

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

void FakeVimPlugin::test_vim_delete()
{
    TestData data;
    setup(&data);

    data.setText("123" N "456");
    KEYS("x",  "23" N "456");
    KEYS("dd", "456");
    KEYS("2x", "6");
    KEYS("dd", "");

    data.setText("void main()");
    KEYS("dt(", "()");

    data.setText("void main()");
    KEYS("df(", ")");

    data.setText("void main()");
    KEYS("wD", "void ");
    KEYS("ggd$", "");

    data.setText("abc def ghi");
    KEYS("2dw", X "ghi");
    data.setText("abc def ghi");
    KEYS("d2w", X "ghi");

    data.setText("abc  " N "  def" N "  ghi" N "jkl");
    KEYS("3dw", X "jkl");
    data.setText("abc  " N "  def" N "  ghi" N "jkl");
    KEYS("d3w", X "jkl");
}

void FakeVimPlugin::test_vim_delete_inner_word()
{
    TestData data;
    setup(&data);

    data.setText("abc def ghi");
    KEYS("wlldiw", "abc " X " ghi");

    data.setText("abc def ghi jkl");
    KEYS("3diw", X  " ghi jkl");

    data.setText("abc " X "  def");
    KEYS("diw", "abc" X "def");
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
    KEYS("daw", "ab" X "c");
    KEYS("daw", "");

    data.setText(X " ghi jkl");
    KEYS("daw", X " jkl");
    KEYS("ldaw", X " ");

    data.setText("abc def ghi jkl");
    KEYS("3daw", X "jkl");

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
}

void FakeVimPlugin::test_vim_change_a_word()
{
    TestData data;
    setup(&data);

    data.setText("abc " X "def ghi");
    KEYS("caw#", "abc #" X "ghi");
    data.setText("abc d" X "ef ghi");
    KEYS("caw#", "abc #" X "ghi");
    data.setText("abc de" X "f ghi");
    KEYS("caw#", "abc #" X "ghi");

    data.setText("abc de" X "f ghi jkl");
    KEYS("2caw#", "abc #" X "jkl");

    data.setText("abc" X " def ghi jkl");
    KEYS("2caw#", "abc#" X " jkl");

    data.setText("abc " X "  def ghi jkl");
    KEYS("2caw#", "abc#" X " jkl");

    data.setText(" abc  " N "  def" N "  ghi" N " jkl");
    KEYS("3caw#", "#" X N " jkl");
}

void FakeVimPlugin::test_vim_block_selection()
{
    TestData data;
    setup(&data);

    data.setText("int main(int /* (unused) */, char *argv[]);");
    KEYS("f(", "int main" X "(int /* (unused) */, char *argv[]);");
    KEYS("da(", "int main" X ";")

    data.setText("int main(int /* (unused) */, char *argv[]);");
    KEYS("f(", "int main" X "(int /* (unused) */, char *argv[]);");
    KEYS("di(", "int main(" X ");")

    data.setText("int main(int /* (unused) */, char *argv[]);");
    KEYS("2f)", "int main(int /* (unused) */, char *argv[]" X ");");
    KEYS("da(", "int main" X ";")

    data.setText("int main(int /* (unused) */, char *argv[]);");
    KEYS("2f)", "int main(int /* (unused) */, char *argv[]" X ");");
    KEYS("di(", "int main(" X ");")

    data.setText("{ { { } } }");
    KEYS("2f{l", "{ { {" X " } } }")
    KEYS("da{", "{ { " X " } }")
    KEYS("da{", "{ " X " }")

    data.setText("{ { { } } }");
    KEYS("2f{l", "{ { {" X " } } }")
    KEYS("2da{", "{ " X " }")

    data.setText("{" N " { " N " } " N "}");
    KEYS("di{", "{" N "}")
}

void FakeVimPlugin::test_vim_repeat()
{
    TestData data;
    setup(&data);

    // delete line
    data.setText("abc" N "def" N "ghi");
    KEYS("dd", X "def" N "ghi");
    KEYS(".", X "ghi");

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

    /* QTCREATORBUG-7251 */
    data.setText("abc abc abc abc");
    KEYS("$?abc<CR>", "abc abc abc " X "abc");
    KEYS("2?abc<CR>", "abc " X "abc abc abc");
    KEYS("n", X "abc abc abc abc");
    KEYS("N", "abc " X "abc abc abc");

    NOT_IMPLEMENTED
    // find same stuff forward and backward,
    // i.e. '<ab>c' forward but not 'a<bc>' backward
    data.setText("abc" N "def" N "ghi");
    KEYS("/\\w\\{2}<CR>", X "abc" N "def" N "ghi");
    KEYS("2n", "abc" N "def" N X "ghi");
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

void FakeVimPlugin::test_vim_copy_paste()
{
    TestData data;
    setup(&data);

    data.setText("123" N "456");
    KEYS("llyy2P", X "123" N "123" N "123" N "456");

    data.setText("123" N "456");
    KEYS("yyp", "123" N X "123" N "456");
    KEYS("2p", "123" N "123" N X "123" N "123" N "456");

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
    COMMAND(":::   %s/\\S\\S\\S/ZZZ/g   |   ::::   %s/ZZZ/XXX/g ", "XXX" N "  XXX" N "  XXX" N X "XXX");
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

    NOT_IMPLEMENTED
    // <C-o>
    data.setText("abc def");
    data.doCommand("imap X <c-o>:%s/def/xxx/<cr>");
    KEYS("iX", "abc xxx");
}
