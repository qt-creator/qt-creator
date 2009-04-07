/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "fakevimhandler.h"

#include <QtCore/QSet>
#include <QtGui/QPlainTextEdit>

#include <QtTest/QtTest>

using namespace FakeVim;
using namespace FakeVim::Internal;


class tst_FakeVim : public QObject
{
    Q_OBJECT

public:
    tst_FakeVim();

    void setup();    
    void send(const QString &command); // send a normal command
    void sendEx(const QString &command); // send an ex command

    QString cleaned(QString wanted) { wanted.remove('$'); return wanted; }

public slots:
    void changeStatusData(const QString &info) { m_statusData = info; }
    void changeStatusMessage(const QString &info) { m_statusMessage = info; }
    void changeExtraInformation(const QString &info) { m_infoMessage = info; }
    
public:
    QString m_statusMessage;
    QString m_statusData;
    QString m_infoMessage;

private slots:
    void commandI();
    void commandDollar();

private:
    QPlainTextEdit m_editor;
    FakeVimHandler m_handler;
    QList<QTextEdit::ExtraSelection> m_selection;

    static const QString lines;
    static const QString escape;
};

const QString tst_FakeVim::lines = 
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

const QString tst_FakeVim::escape = QChar(27);

tst_FakeVim::tst_FakeVim()
    : m_handler(&m_editor, this)
{

    QObject::connect(&m_handler, SIGNAL(commandBufferChanged(QString)),
        this, SLOT(changeStatusMessage(QString)));
    QObject::connect(&m_handler, SIGNAL(extraInformationChanged(QString)),
        this, SLOT(changeExtraInformation(QString)));
    QObject::connect(&m_handler, SIGNAL(statusDataChanged(QString)),
        this, SLOT(changeStatusData(QString)));
}

void tst_FakeVim::setup()
{
    m_statusMessage.clear();
    m_statusData.clear();
    m_infoMessage.clear();
    m_editor.setPlainText(lines);
    QCOMPARE(m_editor.toPlainText(), lines);
}

void tst_FakeVim::send(const QString &command)
{
    m_handler.handleCommand("normal " + command);
}

void tst_FakeVim::sendEx(const QString &command)
{
    m_handler.handleCommand(command);
}

#define checkContents(wanted) \
    do { QString want = cleaned(wanted); \
        QString got = m_editor.toPlainText(); \
        QStringList wantlist = want.split('\n'); \
        QStringList gotlist = got.split('\n'); \
        QCOMPARE(gotlist.size(), wantlist.size()); \
        for (int i = 0; i < wantlist.size() && i < gotlist.size(); ++i) { \
           QString g = QString("line %1: %2").arg(i + 1).arg(gotlist.at(i)); \
           QString w = QString("line %1: %2").arg(i + 1).arg(wantlist.at(i)); \
           QCOMPARE(g, w); \
        } \
    } while (0)

#define checkText(cmd, wanted) \
    do { \
        send(cmd); \
        checkContents(wanted); \
        int p = (wanted).indexOf('$'); \
        QCOMPARE(m_editor.textCursor().position(), p); \
    } while (0)

#define checkTextEx(cmd, wanted) \
    do { \
        sendEx(cmd); \
        checkContents(wanted); \
        int p = (wanted).indexOf('$'); \
        QCOMPARE(m_editor.textCursor().position(), p); \
    } while (0)

#define checkPosition(cmd, pos) \
    do { \
        send(cmd); \
        QCOMPARE(m_editor.textCursor().position(), pos); \
    } while (0)

void tst_FakeVim::commandI()
{
    setup();

    // empty insertion at start of document
    checkText("i" + escape, "$" + lines);
    checkText("u", "$" + lines);

    // small insertion at start of document
    checkText("ix" + escape, "$x" + lines);
    checkText("u", "$" + lines);

    // small insertion at start of document
    checkText("ixxx" + escape, "xx$x" + lines);
    checkText("u", "$" + lines);

    // combine insertions
    checkText("ia" + escape, "$a" + lines);
    checkText("ibx" + escape, "b$xa" + lines);
    checkText("icyy" + escape, "bcy$yxa" + lines);
    checkText("u", "b$xa" + lines);
    checkText("u", "$a" + lines);   // undo broken
    checkTextEx("redo", "b$xa" + lines);
    checkText("u", "$a" + lines);
    checkText("u", "$" + lines);
}

void tst_FakeVim::commandDollar()
{
    setup();
    checkPosition("$", 0);
    checkPosition("j", 2);
}


QTEST_MAIN(tst_FakeVim)

#include "main.moc"
