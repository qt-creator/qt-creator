/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "fakevimhandler.h"

#include <QApplication>
#include <QFontMetrics>
#include <QMainWindow>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTextEdit>

using namespace FakeVim::Internal;

/**
 * Simple editor widget.
 * @tparam TextEdit QTextEdit or QPlainTextEdit as base class
 */
template <typename TextEdit>
class Editor : public TextEdit
{
public:
    Editor()
    {
        TextEdit::setCursorWidth(0);
        TextEdit::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    void paintEvent(QPaintEvent *e)
    {
        TextEdit::paintEvent(e);

        // Draw text cursor.
        QRect rect = TextEdit::cursorRect();
        if ( e->rect().contains(rect) ) {
            QPainter painter(TextEdit::viewport());

            if ( TextEdit::overwriteMode() ) {
                QFontMetrics fm(TextEdit::font());
                rect.setWidth(fm.horizontalAdvance('m'));
                painter.setPen(Qt::NoPen);
                painter.setBrush(TextEdit::palette().color(QPalette::Base));
                painter.setCompositionMode(QPainter::CompositionMode_Difference);
            } else {
                rect.setWidth(TextEdit::cursorWidth());
                painter.setPen(TextEdit::palette().color(QPalette::Text));
            }
            painter.drawRect(rect);
        }
    }
};

static void highlightMatches(QWidget *widget, const QString &pattern)
{
    auto ed = qobject_cast<QTextEdit *>(widget);
    if (!ed)
        return;

    // Clear previous highlights.
    ed->selectAll();
    QTextCursor cur = ed->textCursor();
    QTextCharFormat fmt = cur.charFormat();
    fmt.setBackground(Qt::transparent);
    cur.setCharFormat(fmt);

    // Highlight matches.
    QTextDocument *doc = ed->document();
    const QRegularExpression re(pattern);
    cur = doc->find(re);

    int a = cur.position();
    while ( !cur.isNull() ) {
        if ( cur.hasSelection() ) {
            fmt.setBackground(Qt::yellow);
            cur.setCharFormat(fmt);
        } else {
            cur.movePosition(QTextCursor::NextCharacter);
        }
        cur = doc->find(re, cur);
        int b = cur.position();
        if (a == b) {
            cur.movePosition(QTextCursor::NextCharacter);
            cur = doc->find(re, cur);
            b = cur.position();
            if (a == b) break;
        }
        a = b;
    }
}

class StatusData
{
public:
    void setStatusMessage(const QString &msg, int pos)
    {
        m_statusMessage = pos == -1 ? msg : msg.left(pos) + QChar(10073) + msg.mid(pos);
    }

    void setStatusInfo(const QString &info)
    {
        m_statusData = info;
    }

    QString currentStatusLine() const
    {
        const int slack = 80 - m_statusMessage.size() - m_statusData.size();
        return m_statusMessage + QString(slack, ' ') + m_statusData;
    }

private:
    QString m_statusMessage;
    QString m_statusData;
};

static QWidget *createEditorWidget(bool usePlainTextEdit)
{
    QWidget *editor = 0;
    if (usePlainTextEdit)
        editor = new Editor<QPlainTextEdit>;
    else
        editor = new Editor<QTextEdit>;
    editor->setObjectName("Editor");
    editor->setFocus();

    return editor;
}

static void initHandler(FakeVimHandler &handler)
{
    // Set some Vim options.
    handler.handleCommand("set expandtab");
    handler.handleCommand("set shiftwidth=8");
    handler.handleCommand("set tabstop=16");
    handler.handleCommand("set autoindent");

    // Try to source file "fakevimrc" from current directory.
    handler.handleCommand("source fakevimrc");

    handler.installEventFilter();
    handler.setupWidget();
}

static void initMainWindow(QMainWindow &mainWindow, QWidget *centralWidget, const QString &title)
{
    mainWindow.setWindowTitle(QString("FakeVim (%1)").arg(title));
    mainWindow.setCentralWidget(centralWidget);
    mainWindow.resize(600, 650);
    mainWindow.move(0, 0);
    mainWindow.show();

    // Set monospace font for editor and status bar.
    QFont font = QApplication::font();
    font.setFamily("Monospace");
    centralWidget->setFont(font);
    mainWindow.statusBar()->setFont(font);
}

void readFile(FakeVimHandler &handler, const QString &editFileName)
{
    handler.handleCommand("r " + editFileName);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QStringList args = app.arguments();

    // If first argument is present use QPlainTextEdit instead on QTextEdit;
    bool usePlainTextEdit = args.size() > 1;
    // Second argument is path to file to edit.
    const QString editFileName = args.value(2, "/usr/share/vim/vim73/tutor/tutor");

    // Create editor widget.
    QWidget *editor = createEditorWidget(usePlainTextEdit);

    // Create main window.
    QMainWindow mainWindow;
    initMainWindow(mainWindow, editor, usePlainTextEdit ? "QPlainTextEdit" : "QTextEdit");

    // Keep track of status line related data.
    StatusData statusData;

    // Create FakeVimHandler instance which will emulate Vim behavior in editor widget.
    FakeVimHandler handler(editor, nullptr);

    handler.commandBufferChanged.connect([&](const QString &msg, int cursorPos, int, int) {
        statusData.setStatusMessage(msg, cursorPos);
        mainWindow.statusBar()->showMessage(statusData.currentStatusLine());
    });

    handler.selectionChanged.connect([&handler](const QList<QTextEdit::ExtraSelection> &s) {
        QWidget *widget = handler.widget();
        if (auto ed = qobject_cast<QPlainTextEdit *>(widget))
            ed->setExtraSelections(s);
        else if (auto ed = qobject_cast<QTextEdit *>(widget))
            ed->setExtraSelections(s);
    });

    handler.extraInformationChanged.connect([&](const QString &info) {
        statusData.setStatusInfo(info);
        mainWindow.statusBar()->showMessage(statusData.currentStatusLine());
    });

    handler.statusDataChanged.connect([&](const QString &info) {
        statusData.setStatusInfo(info);
        mainWindow.statusBar()->showMessage(statusData.currentStatusLine());
    });

    handler.highlightMatches.connect([&](const QString &needle) {
        highlightMatches(handler.widget(), needle);
    });

    handler.handleExCommandRequested.connect([](bool *handled, const ExCommand &cmd) {
        if (cmd.matches("q", "quit") || cmd.matches("qa", "qall")) {
            QApplication::quit();
            *handled = true;
        } else {
            *handled = false;
        }
    });

    // Initialize FakeVimHandler.
    initHandler(handler);

    // Read file content to editor.
    readFile(handler, editFileName);

    return app.exec();
}
