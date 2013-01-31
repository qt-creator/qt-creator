/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "fakevimhandler.h"

#include <QSet>

#include <QTextEdit>
#include <QPlainTextEdit>

#include <QtTest>

//TESTED_COMPONENT=src/plugins/fakevim
using namespace FakeVim;
using namespace FakeVim::Internal;

#define EDITOR(s) (m_textedit ? m_textedit->s : m_plaintextedit->s)

class tst_FakeVim : public QObject
{
    Q_OBJECT

public:
    tst_FakeVim(bool);
    ~tst_FakeVim();

public slots:
    void changeStatusData(const QString &info) { m_statusData = info; }
    void changeStatusMessage(const QString &info, int) { m_statusMessage = info; }
    void changeExtraInformation(const QString &info) { m_infoMessage = info; }

private slots:
    // functional tests
    void indentation();

    // command mode
    void command_oO();
    void command_put_at_eol();
    void command_Cxx_down_dot();
    void command_Gyyp();
    void command_J();
    void command_Yp();
    void command_cc();
    void command_cw();
    void command_cj();
    void command_ck();
    void command_c_dollar();
    void command_C();
    void command_dd();
    void command_dd_2();
    void command_d_dollar();
    void command_dgg();
    void command_dG();
    void command_dj();
    void command_dk();
    void command_D();
    void command_dfx_down();
    void command_dollar();
    void command_down();
    void command_dw();
    void command_e();
    void command_i();
    void command_left();
    void command_ma_yank();
    void command_r();
    void command_right();
    void command_up();
    void command_w();
    void command_x();
    void command_yyp();
    void command_y_dollar();

    void visual_d();
    void Visual_d();

    // special tests
    void test_i_cw_i();

private:
    void setup();
    void send(const QString &command) { sendEx("normal " + command); }
    void sendEx(const QString &command); // send an ex command

    bool checkContentsHelper(QString expected, const char* file, int line);
    bool checkHelper(bool isExCommand, QString cmd, QString expected,
        const char* file, int line);
    QString insertCursor(const QString &needle0);
    QString cursor(const QString &line, int pos); // insert @ at cursor pos, negative counts from back

    QString lmid(int i, int n = -1) const
        { return QStringList(l.mid(i, n)).join("\n"); }

    QTextEdit *m_textedit;
    QPlainTextEdit *m_plaintextedit;
    FakeVimHandler *m_handler;
    QList<QTextEdit::ExtraSelection> m_selection;

    QString m_statusMessage;
    QString m_statusData;
    QString m_infoMessage;

    // the individual lines
    static const QStringList l; // identifier intentionally kept short
    static const QString lines;
    static const QString escape;
};

const QString tst_FakeVim::lines =
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

const QStringList tst_FakeVim::l = tst_FakeVim::lines.split('\n');

const QString tst_FakeVim::escape = QChar(27);


tst_FakeVim::tst_FakeVim(bool usePlainTextEdit)
{
    if (usePlainTextEdit) {
        m_textedit = 0;
        m_plaintextedit = new QPlainTextEdit;
    } else {
        m_textedit = new QTextEdit;
        m_plaintextedit = 0;
    }
    m_handler = 0;
}

tst_FakeVim::~tst_FakeVim()
{
    delete m_handler;
    delete m_textedit;
    delete m_plaintextedit;
}

void tst_FakeVim::setup()
{
    delete m_handler;
    m_handler = 0;
    m_statusMessage.clear();
    m_statusData.clear();
    m_infoMessage.clear();
    if (m_textedit) {
        m_textedit->setPlainText(lines);
        QTextCursor tc = m_textedit->textCursor();
        tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        m_textedit->setTextCursor(tc);
        m_textedit->setPlainText(lines);
        m_handler = new FakeVimHandler(m_textedit);
    } else {
        m_plaintextedit->setPlainText(lines);
        QTextCursor tc = m_plaintextedit->textCursor();
        tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        m_plaintextedit->setTextCursor(tc);
        m_plaintextedit->setPlainText(lines);
        m_handler = new FakeVimHandler(m_plaintextedit);
    }

    QObject::connect(m_handler, SIGNAL(commandBufferChanged(QString,int)),
        this, SLOT(changeStatusMessage(QString,int)));
    QObject::connect(m_handler, SIGNAL(extraInformationChanged(QString)),
        this, SLOT(changeExtraInformation(QString)));
    QObject::connect(m_handler, SIGNAL(statusDataChanged(QString)),
        this, SLOT(changeStatusData(QString)));

    QCOMPARE(EDITOR(toPlainText()), lines);

    sendEx("set iskeyword=@,48-57,_,192-255,a-z,A-Z");
}

void tst_FakeVim::sendEx(const QString &command)
{
    if (m_handler)
        m_handler->handleCommand(command);
    else
        qDebug() << "NO HANDLER YET";
}

bool tst_FakeVim::checkContentsHelper(QString want, const char* file, int line)
{
    QString got = EDITOR(toPlainText());
    int pos = EDITOR(textCursor().position());
    got = got.left(pos) + "@" + got.mid(pos);
    QStringList wantlist = want.split('\n');
    QStringList gotlist = got.split('\n');
    if (!QTest::qCompare(gotlist.size(), wantlist.size(), "", "", file, line)) {
        qDebug() << "0 WANT: " << want;
        qDebug() << "0 GOT: " << got;
        return false;
    }
    for (int i = 0; i < wantlist.size() && i < gotlist.size(); ++i) {
        QString g = QString("line %1: %2").arg(i + 1).arg(gotlist.at(i));
        QString w = QString("line %1: %2").arg(i + 1).arg(wantlist.at(i));
        if (!QTest::qCompare(g, w, "", "", file, line)) {
            qDebug() << "1 WANT: " << want;
            qDebug() << "1 GOT: " << got;
            return false;
        }
    }
    return true;
}

bool tst_FakeVim::checkHelper(bool ex, QString cmd, QString expected,
    const char *file, int line)
{
    if (ex)
        sendEx(cmd);
    else
        send(cmd);
    return checkContentsHelper(expected, file, line);
}


#define checkContents(expected) \
    do { if (!checkContentsHelper(expected, __FILE__, __LINE__)) return; } while (0)

// Runs a "normal" command and checks the result.
// Cursor position is marked by a '@' in the expected contents.
#define check(cmd, expected) \
    do { if (!checkHelper(false, cmd, expected, __FILE__, __LINE__)) \
            return; } while (0)

#define move(cmd, expected) \
    do { if (!checkHelper(false, cmd, insertCursor(expected), __FILE__, __LINE__)) \
            return; } while (0)

// Runs an ex command and checks the result.
// Cursor position is marked by a '@' in the expected contents.
#define checkEx(cmd, expected) \
    do { if (!checkHelper(true, cmd, expected, __FILE__, __LINE__)) \
            return; } while (0)

QString tst_FakeVim::insertCursor(const QString &needle0)
{
    QString needle = needle0;
    needle.remove('@');
    QString lines0 = lines;
    int pos = lines0.indexOf(needle);
    if (pos == -1)
        qDebug() << "Cannot find: \n----\n" + needle + "\n----\n";
    lines0.replace(pos, needle.size(), needle0);
    return lines0;
}

QString tst_FakeVim::cursor(const QString &line, int pos)
{
    if (pos < 0)
        pos = line.length() + pos;

    return line.left(pos) + "@" + line.mid(pos);
}


//////////////////////////////////////////////////////////////////////////
//
// Command mode
//
//////////////////////////////////////////////////////////////////////////

void tst_FakeVim::indentation()
{
    setup();
    sendEx("set expandtab");
    sendEx("set tabstop=4");
    sendEx("set shiftwidth=4");
    QCOMPARE(m_handler->physicalIndentation("      \t\t\tx"), 6 + 3);
    QCOMPARE(m_handler->logicalIndentation ("      \t\t\tx"), 4 + 3 * 4);
    QCOMPARE(m_handler->physicalIndentation("     \t\t\tx"), 5 + 3);
    QCOMPARE(m_handler->logicalIndentation ("     \t\t\tx"), 4 + 3 * 4);

    QCOMPARE(m_handler->tabExpand(3), QLatin1String("   "));
    QCOMPARE(m_handler->tabExpand(4), QLatin1String("    "));
    QCOMPARE(m_handler->tabExpand(5), QLatin1String("     "));
    QCOMPARE(m_handler->tabExpand(6), QLatin1String("      "));
    QCOMPARE(m_handler->tabExpand(7), QLatin1String("       "));
    QCOMPARE(m_handler->tabExpand(8), QLatin1String("        "));
    QCOMPARE(m_handler->tabExpand(9), QLatin1String("         "));

    sendEx("set expandtab");
    sendEx("set tabstop=8");
    sendEx("set shiftwidth=4");
    QCOMPARE(m_handler->physicalIndentation("      \t\t\tx"), 6 + 3);
    QCOMPARE(m_handler->logicalIndentation ("      \t\t\tx"), 0 + 3 * 8);
    QCOMPARE(m_handler->physicalIndentation("     \t\t\tx"), 5 + 3);
    QCOMPARE(m_handler->logicalIndentation ("     \t\t\tx"), 0 + 3 * 8);

    QCOMPARE(m_handler->tabExpand(3), QLatin1String("   "));
    QCOMPARE(m_handler->tabExpand(4), QLatin1String("    "));
    QCOMPARE(m_handler->tabExpand(5), QLatin1String("     "));
    QCOMPARE(m_handler->tabExpand(6), QLatin1String("      "));
    QCOMPARE(m_handler->tabExpand(7), QLatin1String("       "));
    QCOMPARE(m_handler->tabExpand(8), QLatin1String("        "));
    QCOMPARE(m_handler->tabExpand(9), QLatin1String("         "));

    sendEx("set noexpandtab");
    sendEx("set tabstop=4");
    sendEx("set shiftwidth=4");
    QCOMPARE(m_handler->physicalIndentation("      \t\t\tx"), 6 + 3);
    QCOMPARE(m_handler->logicalIndentation ("      \t\t\tx"), 4 + 3 * 4);
    QCOMPARE(m_handler->physicalIndentation("     \t\t\tx"), 5 + 3);
    QCOMPARE(m_handler->logicalIndentation ("     \t\t\tx"), 4 + 3 * 4);

    QCOMPARE(m_handler->tabExpand(3), QLatin1String("   "));
    QCOMPARE(m_handler->tabExpand(4), QLatin1String("\t"));
    QCOMPARE(m_handler->tabExpand(5), QLatin1String("\t "));
    QCOMPARE(m_handler->tabExpand(6), QLatin1String("\t  "));
    QCOMPARE(m_handler->tabExpand(7), QLatin1String("\t   "));
    QCOMPARE(m_handler->tabExpand(8), QLatin1String("\t\t"));
    QCOMPARE(m_handler->tabExpand(9), QLatin1String("\t\t "));

    sendEx("set noexpandtab");
    sendEx("set tabstop=8");
    sendEx("set shiftwidth=4");
    QCOMPARE(m_handler->physicalIndentation("      \t\t\tx"), 6 + 3);
    QCOMPARE(m_handler->logicalIndentation ("      \t\t\tx"), 0 + 3 * 8);
    QCOMPARE(m_handler->physicalIndentation("     \t\t\tx"), 5 + 3);
    QCOMPARE(m_handler->logicalIndentation ("     \t\t\tx"), 0 + 3 * 8);

    QCOMPARE(m_handler->tabExpand(3), QLatin1String("   "));
    QCOMPARE(m_handler->tabExpand(4), QLatin1String("    "));
    QCOMPARE(m_handler->tabExpand(5), QLatin1String("     "));
    QCOMPARE(m_handler->tabExpand(6), QLatin1String("      "));
    QCOMPARE(m_handler->tabExpand(7), QLatin1String("       "));
    QCOMPARE(m_handler->tabExpand(8), QLatin1String("\t"));
    QCOMPARE(m_handler->tabExpand(9), QLatin1String("\t "));
}


//////////////////////////////////////////////////////////////////////////
//
// Command mode
//
//////////////////////////////////////////////////////////////////////////

void tst_FakeVim::command_cc()
{
    setup();
    move("j",                "@" + l[1]);
    check("ccabc" + escape,  l[0] + "\nab@c\n" + lmid(2));
    check("ccabc" + escape,  l[0] + "\nab@c\n" + lmid(2));
    check(".",               l[0] + "\nab@c\n" + lmid(2));
    check("j",               l[0] + "\nabc\n#i@nclude <QtGui>\n" + lmid(3));
    check("3ccxyz" + escape, l[0] + "\nabc\nxy@z\n" + lmid(5));
}

void tst_FakeVim::command_cw()
{
    setup();
    move("j",                "@" + l[1]);
    check("cwx" + escape,    l[0] + "\n@xinclude <QtCore>\n" + lmid(2));
}

void tst_FakeVim::command_cj()
{
    setup();
    move("j$",               cursor(l[1], -1));
    check("cj" + escape,     l[0]+"\n@" + "\n" + lmid(3));
    check("P",               lmid(0,1)+"\n" + "@"+lmid(1,2)+"\n" + "\n" +  lmid(3));
    check("u",               l[0]+"\n@" + "\n" + lmid(3));

    setup();
    move("j$",               cursor(l[1], -1));
    check("cjabc" + escape,  l[0]+"\nab@c\n" + lmid(3));
    check("u",               lmid(0,1)+"\n" + cursor(l[1], -1)+"\n" + lmid(2));
    check("gg",              "@" + lmid(0));
    check(".",               "ab@c\n" + lmid(2));
}

void tst_FakeVim::command_ck()
{
    setup();
    move("j$",               cursor(l[1], -1));
    check("ck" + escape,     "@\n" + lmid(2));
    check("P",               "@" + lmid(0,2)+"\n" + "\n" + lmid(2));
}

void tst_FakeVim::command_c_dollar()
{
    setup();
    move("j",                "@" + l[1]);
    move("$",                cursor(l[1], -1));
    check("c$" + escape,     l[0]+"\n" + l[1].left(l[1].length()-2)+"@"+l[1][l[1].length()-2]+"\n" + lmid(2));
    check("c$" + escape,     l[0]+"\n" + l[1].left(l[1].length()-3)+"@"+l[1][l[1].length()-3]+"\n" + lmid(2));
    check("0c$abc" + escape, l[0]+"\n" + "ab@c\n" + lmid(2));
    check("0c$abc" + escape, l[0]+"\n" + "ab@c\n" + lmid(2));
}

void tst_FakeVim::command_C()
{
    setup();
    move("j",                "@" + l[1]);
    check("Cabc" + escape,   l[0] + "\nab@c\n" + lmid(2));
    check("Cabc" + escape,   l[0] + "\nabab@c\n" + lmid(2));
    check("$Cabc" + escape,  l[0] + "\nababab@c\n" + lmid(2));
    check("0C" + escape,     l[0] + "\n@\n" + lmid(2));
    check("0Cabc" + escape,  l[0] + "\nab@c\n" + lmid(2));
}

void tst_FakeVim::command_dw()
{
    setup();
    check("dw",  "@#include <QtCore>\n" + lmid(2));
    check("dw",  "@include <QtCore>\n" + lmid(2));
    check("dw",  "@<QtCore>\n" + lmid(2));
    check("dw",  "@QtCore>\n" + lmid(2));
    check("dw",  "@>\n" + lmid(2));
    check("dw",  "@\n" + lmid(2)); // Real vim has this intermediate step, too
    check("dw",  "@#include <QtGui>\n" + lmid(3));
    check("dw",  "@include <QtGui>\n" + lmid(3));
    check("dw",  "@<QtGui>\n" + lmid(3));
    check("dw",  "@QtGui>\n" + lmid(3));
    check("dw",  "@>\n" + lmid(3));
}

void tst_FakeVim::command_dd()
{
    setup();
    move("j",    "@" + l[1]);
    check("dd",  l[0] + "\n@" + lmid(2));
    check(".",   l[0] + "\n@" + lmid(3));
    check("3dd", l[0] + "\n    @QApplication app(argc, argv);\n" + lmid(7));
    check("4l",  l[0] + "\n    QApp@lication app(argc, argv);\n" + lmid(7));
    check("dd",  l[0] + "\n@" + lmid(7));
    check(".",   l[0] + "\n    @return app.exec();\n" + lmid(9));
    check("dd",  l[0] + "\n@" + lmid(9));
}

void tst_FakeVim::command_dd_2()
{
    setup();
    move("j",    "@" + l[1]);
    check("dd",  l[0] + "\n@" + lmid(2));
    check("p",   l[0] + "\n" + l[2] + "\n@" + l[1] + "\n" + lmid(3));
    check("u",   l[0] + "\n@" + lmid(2));
}

void tst_FakeVim::command_d_dollar()
{
    setup();
    move("j$",               cursor(l[1], -1));
    check("$d$",             l[0]+"\n" + l[1].left(l[1].length()-2)+"@"+l[1][l[1].length()-2]+"\n" + lmid(2));
    check("0d$",             l[0] + "\n"+"@\n" + lmid(2));
}

void tst_FakeVim::command_dj()
{
    setup();
    move("j$",               cursor(l[1], -1));
    check("dj",              l[0]+"\n@" + lmid(3));
    check("P",               lmid(0,1)+"\n" + "@"+lmid(1));
    move("0",                "@" + l[1]);
    check("dj",              l[0]+"\n@" + lmid(3));
    check("P",               lmid(0,1)+"\n" + "@"+lmid(1));
    move("05l",              l[1].left(5) + "@" + l[1].mid(5));
    check("dj",              l[0]+"\n@" + lmid(3));
    check("P",               lmid(0,1)+"\n" + "@"+lmid(1));
    check("dj",              l[0]+"\n@" + lmid(3));
    check("p",               lmid(0,1)+"\n" + lmid(3,1)+"\n" + "@"+lmid(1,2)+"\n" + lmid(4));
}

void tst_FakeVim::command_dk()
{
    setup();
    move("j$",               cursor(l[1], -1));
    check("dk",              "@" + lmid(2));
    check("P",               "@" + lmid(0));
    move("j0",               "@" + l[1]);
    check("dk",              "@" + lmid(2));
    check("P",               "@" + lmid(0));
    move("j05l",             l[1].left(5) + "@" + l[1].mid(5));
    check("dk",              "@" + lmid(2));
    check("P",               "@" + lmid(0));
    move("j05l",             l[1].left(5) + "@" + l[1].mid(5));
    check("dk",              "@" + lmid(2));
    check("p",               lmid(2,1)+"\n" + "@" + lmid(0,2)+"\n" + lmid(3));
}

void tst_FakeVim::command_dgg()
{
    setup();
    check("G",               lmid(0, l.size()-2)+"\n" "@"+lmid(l.size()-2));
    check("dgg",             "@");
    check("u",               "@" + lmid(0));
}

void tst_FakeVim::command_dG()
{
    setup();
    check("dG",              "@");
    check("u",               "@" + lmid(0));
    move("j",                "@" + l[1]);
    check("dG",              lmid(0,1)+"\n" + "@");
    check("u",               l[0]+"\n" + "@" + lmid(1));
    check("G",               lmid(0, l.size()-2)+"\n" + "@"+lmid(l.size()-2));
    qWarning("FIXME");
return;
    // include movement to first column, as otherwise the result depends on the 'startofline' setting
    check("dG0",             lmid(0, l.size()-2)+"\n" + "@"+lmid(l.size()-2,1));
    check("dG0",             lmid(0, l.size()-3)+"\n" + "@"+lmid(l.size()-3,1));
}

void tst_FakeVim::command_D()
{
    setup();
    move("j",                "@" + l[1]);
    check("$D",              l[0]+"\n" + l[1].left(l[1].length()-2)+"@"+l[1][l[1].length()-2]+"\n" + lmid(2));
    check("0D",              l[0] + "\n@\n" + lmid(2));
}

void tst_FakeVim::command_dollar()
{
    setup();
    move("j$",               cursor(l[1], -1));
    move("j$",               cursor(l[2], -1));
    move("2j", "@)");
}

void tst_FakeVim::command_down()
{
    setup();
    move("j",  "@" + l[1]);
    move("3j", "@int main");
    move("4j", "@    return app.exec()");
}

void tst_FakeVim::command_dfx_down()
{
    setup();
    check("j4l",  l[0] + "\n#inc@lude <QtCore>\n" + lmid(2));
    qWarning("FIXME");
return;
    check("df ",  l[0] + "\n#inc@<QtCore>\n" + lmid(2));
    check("j",    l[0] + "\n#inc<QtCore>\n#inc@lude <QtGui>\n" + lmid(3));
    check(".",    l[0] + "\n#inc<QtCore>\n#inc@<QtGui>\n" + lmid(3));
    check("u",    l[0] + "\n#inc<QtCore>\n#inc@lude <QtGui>\n" + lmid(3));
    check("u",    l[0] + "\n#inc@lude <QtCore>\n" + lmid(2));
}

void tst_FakeVim::command_Cxx_down_dot()
{
    setup();
    check("j4l",          l[0] + "\n#inc@lude <QtCore>\n" + lmid(2));
    check("Cxx" + escape, l[0] + "\n#incx@x\n" + lmid(2));
    check("j",            l[0] + "\n#incxx\n#incl@ude <QtGui>\n" + lmid(3));
    check(".",            l[0] + "\n#incxx\n#inclx@x\n" + lmid(3));
}

void tst_FakeVim::command_e()
{
    setup();
    move("e",  "@#include <QtCore");
    move("e",  "#includ@e <QtCore");
    move("e",  "#include @<QtCore");
    move("3e", "@#include <QtGui");
    move("e",  "#includ@e <QtGui");
    move("e",  "#include @<QtGui");
    move("e",  "#include <QtGu@i");
    move("4e", "int main@(int argc, char *argv[])");
    move("e",  "int main(in@t argc, char *argv[])");
    move("e",  "int main(int arg@c, char *argv[])");
    move("e",  "int main(int argc@, char *argv[])");
    move("e",  "int main(int argc, cha@r *argv[])");
    move("e",  "int main(int argc, char @*argv[])");
    move("e",  "int main(int argc, char *arg@v[])");
    move("e",  "int main(int argc, char *argv[]@)");
    move("e",  "@{");
    move("10k","@\n"); // home.
}

void tst_FakeVim::command_i()
{
    setup();

    // empty insertion at start of document
    check("i" + escape, "@" + lines);
    check("u", "@" + lines);

    // small insertion at start of document
    check("ix" + escape, "@x" + lines);
    check("u", "@" + lines);
    checkEx("redo", "@x" + lines);
    check("u", "@" + lines);

    // small insertion at start of document
    check("ixxx" + escape, "xx@x" + lines);
    check("u", "@" + lines);

    // combine insertions
    check("i1" + escape, "@1" + lines);
    check("i2" + escape, "@21" + lines);
    check("i3" + escape, "@321" + lines);
    check("u",           "@21" + lines);
    check("u",           "@1" + lines);
    check("u",           "@" + lines);
    check("ia" + escape, "@a" + lines);
    check("ibx" + escape, "b@xa" + lines);
    check("icyy" + escape, "bcy@yxa" + lines);
    check("u", "b@xa" + lines);
    check("u", "@a" + lines);
    checkEx("redo", "b@xa" + lines);
    check("u", "@a" + lines);
}

void tst_FakeVim::command_left()
{
    setup();
    move("4j",  "@int main");
    move("h",   "@int main"); // no move over left border
    move("$",   "argv[]@)");
    move("h",   "argv[@])");
    move("3h",  "ar@gv[])");
    move("50h", "@int main");
}

void tst_FakeVim::command_r()
{
    setup();
    move("4j",   "@int main");
    move("$",    "int main(int argc, char *argv[]@)");
    check("rx",  lmid(0, 4) + "\nint main(int argc, char *argv[]@x\n" + lmid(5));
    check("2h",  lmid(0, 4) + "\nint main(int argc, char *argv@[]x\n" + lmid(5));
    check("4ra", lmid(0, 4) + "\nint main(int argc, char *argv@[]x\n" + lmid(5));
    check("3rb", lmid(0, 4) + "\nint main(int argc, char *argvbb@b\n" + lmid(5));
    check("2rc", lmid(0, 4) + "\nint main(int argc, char *argvbb@b\n" + lmid(5));
    check("h2rc",lmid(0, 4) + "\nint main(int argc, char *argvbc@c\n" + lmid(5));
}

void tst_FakeVim::command_right()
{
    setup();
    move("4j", "@int main");
    move("l", "i@nt main");
    move("3l", "int @main");
    move("50l", "argv[]@)");
}

void tst_FakeVim::command_up()
{
    setup();
    move("j", "@#include <QtCore");
    move("3j", "@int main");
    move("4j", "@    return app.exec()");
}

void tst_FakeVim::command_w()
{
    setup();
    move("w",   "@#include <QtCore");
    move("w",   "#@include <QtCore");
    move("w",   "#include @<QtCore");
    move("3w",  "@#include <QtGui");
    move("w",   "#@include <QtGui");
    move("w",   "#include @<QtGui");
    move("w",   "#include <@QtGui");
    move("4w",  "int @main(int argc, char *argv[])");
    move("w",  "int main@(int argc, char *argv[])");
    move("w",   "int main(@int argc, char *argv[])");
    move("w",   "int main(int @argc, char *argv[])");
    move("w",   "int main(int argc@, char *argv[])");
    move("w",   "int main(int argc, @char *argv[])");
    move("w",   "int main(int argc, char @*argv[])");
    move("w",   "int main(int argc, char *@argv[])");
    move("w",   "int main(int argc, char *argv@[])");
    move("w",   "@{");
}

void tst_FakeVim::command_yyp()
{
    setup();
    move("4j",   "@int main");
    check("yyp", lmid(0, 4) + "\n" + lmid(4, 1) + "\n@" + lmid(4));
}

void tst_FakeVim::command_y_dollar()
{
    setup();
    move("j",     "@" + l[1]);
    check("$y$p", l[0]+"\n"+ l[1]+"@>\n" + lmid(2));
    check("$y$p", l[0]+"\n"+ l[1]+">@>\n" + lmid(2));
    check("$y$P", l[0]+"\n"+ l[1]+">@>>\n" + lmid(2));
    check("$y$P", l[0]+"\n"+ l[1]+">>@>>\n" + lmid(2));
}

void tst_FakeVim::command_Yp()
{
    setup();
    move("4j",   "@int main");
    check("Yp", lmid(0, 4) + "\n" + lmid(4, 1) + "\n@" + lmid(4));
}

void tst_FakeVim::command_ma_yank()
{
    setup();
    move("4j",   "@int main");
    check("ygg", "@" + lmid(0));
    move("4j",   "@int main");
    check("p",    lmid(0,5) + "\n@" + lmid(0,4) +"\n" + lmid(4));

    setup();
    check("ma", "@" + lmid(0));
    move("4j",   "@int main");
    check("mb", lmid(0,4) + "\n@" + lmid(4));
    check("\"ay'a", "@" + lmid(0));
    check("'b", lmid(0,4) + "\n@" + lmid(4));
    check("\"ap", lmid(0,5) + "\n@" + lmid(0,4) +"\n" + lmid(4));
}

void tst_FakeVim::command_Gyyp()
{
    setup();
    check("G",   lmid(0, l.size()-2) + "\n@" + lmid(l.size()-2));
    check("yyp", lmid(0) + "@" + lmid(9, 1)+"\n");
}

void tst_FakeVim::test_i_cw_i()
{
    setup();
    move("j",                "@" + l[1]);
    check("ixx" + escape,    l[0] + "\nx@x" + lmid(1));
    check("cwyy" + escape,   l[0] + "\nxy@y" + lmid(1));
    check("iaa" + escape,    l[0] + "\nxya@ay" + lmid(1));
}

void tst_FakeVim::command_J()
{
    setup();
    move("4j4l",   "int @main");

    check("J", lmid(0, 5) + "@ " + lmid(5));
    check("u", lmid(0, 4) + "\nint @main(int argc, char *argv[])\n" + lmid(5));
    checkEx("redo", lmid(0, 5) + "@ " + lmid(5));

    check("3J", lmid(0, 5) + " " + lmid(5, 1) + " " + lmid(6, 1).mid(4) + "@ " + lmid(7));
    check("uu", lmid(0, 4) + "\nint @main(int argc, char *argv[])\n" + lmid(5));
    checkEx("redo", lmid(0, 5) + "@ " + lmid(5));
}

void tst_FakeVim::command_put_at_eol()
{
    setup();
    move("j$",               cursor(l[1], -1));
    check("y$",              lmid(0,1)+"\n" + cursor(l[1], -1)+"\n" + lmid(2));
    check("p",               lmid(0,2)+"@>\n" + lmid(2));
    check("p",               lmid(0,2)+">@>\n" + lmid(2));
    check("$",               lmid(0,2)+">@>\n" + lmid(2));
    check("P",               lmid(0,2)+">@>>\n" + lmid(2));
}

void tst_FakeVim::command_oO()
{
    setup();
    check("gg",              "@" + lmid(0));
    check("Ol1" + escape,    "l@1\n" + lmid(0));
    check("gg",              "@l1\n" + lmid(0));
    check("ol2" + escape,    "l1\n" "l@2\n" + lmid(0));
    check("G",               "l1\n" "l2\n" + lmid(0,l.size()-2)+"\n" + "@"+lmid(l.size()-2));
    check("G$",              "l1\n" "l2\n" + lmid(0,l.size()-2)+"\n" + "@"+lmid(l.size()-2));
    check("ol-1" + escape,   "l1\n" "l2\n" + lmid(0) + "l-@1\n");
    check("G",               "l1\n" "l2\n" + lmid(0) + "@l-1\n");
    check("Ol-2" + escape,   "l1\n" "l2\n" + lmid(0) + "l-@2\n" + "l-1\n");
}

void tst_FakeVim::command_x()
{
    setup();
    check("x", "@" + lmid(0));
    move("j$", cursor(l[1], -1));
    check("x", lmid(0,1)+"\n" + l[1].left(l[1].length()-2)+"@"+l[1].mid(l[1].length()-2,1)+"\n" + lmid(2));
}

void tst_FakeVim::visual_d()
{
    setup();
    check("vd", "@" + lmid(0));
    check("vx", "@" + lmid(0));
    check("vjd", "@" + lmid(1).mid(1));
    check("ugg", "@" + lmid(0)); // FIXME: cursor should be at begin of doc w/o gg
    move("j", "@" + l[1]);
    check("vd", lmid(0, 1)+"\n" + "@" + lmid(1).mid(1));
    check("u", lmid(0, 1)+"\n" + "@" + lmid(1));
    check("vx", lmid(0, 1)+"\n" + "@" + lmid(1).mid(1));
    check("u", lmid(0, 1)+"\n" + "@" + lmid(1));
    check("vhx", lmid(0, 1)+"\n" + "@" + lmid(1).mid(1));
    check("u", lmid(0, 1)+"\n" + "@" + lmid(1));
    check("vlx", lmid(0, 1)+"\n" + "@" + lmid(1).mid(2));
    check("P", lmid(0, 1)+"\n" + lmid(1).left(1)+"@"+lmid(1).mid(1));
    check("vhd", lmid(0, 1)+"\n" + "@" + lmid(1).mid(2));
    check("u0", lmid(0, 1)+"\n" + "@" + lmid(1)); // FIXME: cursor should be at begin of line w/o 0
    qWarning("FIXME");
return;
    check("v$d", lmid(0, 1)+"\n" + "@" + lmid(2));
    check("v$od", lmid(0, 1)+"\n" + "@" + lmid(3));
    check("$v$x", lmid(0, 1)+"\n" + lmid(3,1) + "@" + lmid(4));
    check("0v$d", lmid(0, 1)+"\n" + "@" + lmid(5));
    check("$v0d", lmid(0, 1)+"\n" + "@\n" + lmid(6));
    check("v$o0k$d", lmid(0, 1)+"\n" + "@" + lmid(6).mid(1));
}

void tst_FakeVim::Visual_d()
{
    setup();
    check("Vd", "@" + lmid(1));
    check("V2kd", "@" + lmid(2));
    check("u", "@" + lmid(1));
    check("u", "@" + lmid(0));
    move("j", "@" + l[1]);
    check("V$d", lmid(0,1)+"\n" + "@" + lmid(2));
    check("$V$$d", lmid(0,1)+"\n" + "@" + lmid(3));
    check("Vkx", "@" + lmid(4));
    check("P", "@" + lmid(0,1)+"\n" + lmid(3));
}


/*

#include <QtCore>
#include <QtWidgets>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    return app.exec();
}
*/

//////////////////////////////////////////////////////////////////////////
//
// Main
//
//////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    int res = 0;
    QApplication app(argc, argv);

    // Test with QPlainTextEdit.
    tst_FakeVim plaintextedit(true);
    res += QTest::qExec(&plaintextedit, argc, argv);

#if 0
    // Test with QTextEdit, too.
    tst_FakeVim textedit(false);
    res += QTest::qExec(&textedit, argc, argv);
#endif

    return res;
}

#include "tst_fakevim.moc"
