/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "fakevimhandler.h"

#include <QApplication>
#include <QFontMetrics>
#include <QMainWindow>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QTextEdit>

using namespace FakeVim::Internal;

typedef QLatin1String _;

/**
 * Simple editor widget.
 * @tparam TextEdit QTextEdit or QPlainTextEdit as base class
 */
template <typename TextEdit>
class Editor : public TextEdit
{
public:
    Editor(QWidget *parent = 0) : TextEdit(parent)
    {
        TextEdit::setCursorWidth(0);
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
                rect.setWidth(fm.width(QLatin1Char('m')));
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

class Proxy : public QObject
{
    Q_OBJECT

public:
    Proxy(QWidget *widget, QMainWindow *mw, QObject *parent = 0)
      : QObject(parent), m_widget(widget), m_mainWindow(mw)
    {}

public slots:
    void changeSelection(const QList<QTextEdit::ExtraSelection> &s)
    {
        if (QPlainTextEdit *ed = qobject_cast<QPlainTextEdit *>(m_widget))
            ed->setExtraSelections(s);
        else if (QTextEdit *ed = qobject_cast<QTextEdit *>(m_widget))
            ed->setExtraSelections(s);
    }

    void changeStatusData(const QString &info)
    {
        m_statusData = info;
        updateStatusBar();
    }

    void highlightMatches(const QString &pattern)
    {
        QTextEdit *ed = qobject_cast<QTextEdit *>(m_widget);
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
        QRegExp re(pattern);
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

    void changeStatusMessage(const QString &contents, int cursorPos)
    {
        m_statusMessage = cursorPos == -1 ? contents
            : contents.left(cursorPos) + QChar(10073) + contents.mid(cursorPos);
        updateStatusBar();
    }

    void changeExtraInformation(const QString &info)
    {
        QMessageBox::information(m_widget, tr("Information"), info);
    }

    void updateStatusBar()
    {
        int slack = 80 - m_statusMessage.size() - m_statusData.size();
        QString msg = m_statusMessage + QString(slack, QLatin1Char(' ')) + m_statusData;
        m_mainWindow->statusBar()->showMessage(msg);
    }

    void handleExCommand(bool *handled, const ExCommand &cmd)
    {
        if (cmd.matches(_("q"), _("quit")) || cmd.matches(_("qa"), _("qall"))) {
            QApplication::quit();
        } else {
            *handled = false;
            return;
        }

        *handled = true;
    }

private:
    QWidget *m_widget;
    QMainWindow *m_mainWindow;
    QString m_statusMessage;
    QString m_statusData;
};

QWidget *createEditorWidget(bool usePlainTextEdit)
{
    QWidget *editor = 0;
    if (usePlainTextEdit) {
        Editor<QPlainTextEdit> *w = new Editor<QPlainTextEdit>;
        w->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        editor = w;
    } else {
        Editor<QTextEdit> *w = new Editor<QTextEdit>;
        w->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        editor = w;
    }
    editor->setObjectName(_("Editor"));
    editor->setFocus();

    return editor;
}

void initHandler(FakeVimHandler &handler)
{
    // Set some Vim options.
    handler.handleCommand(_("set expandtab"));
    handler.handleCommand(_("set shiftwidth=8"));
    handler.handleCommand(_("set tabstop=16"));
    handler.handleCommand(_("set autoindent"));

    // Try to source file "fakevimrc" from current directory.
    handler.handleCommand(_("source fakevimrc"));

    handler.installEventFilter();
    handler.setupWidget();
}

void initMainWindow(QMainWindow &mainWindow, QWidget *centralWidget, const QString &title)
{
    mainWindow.setWindowTitle(QString(_("FakeVim (%1)")).arg(title));
    mainWindow.setCentralWidget(centralWidget);
    mainWindow.resize(600, 650);
    mainWindow.move(0, 0);
    mainWindow.show();

    // Set monospace font for editor and status bar.
    QFont font = QApplication::font();
    font.setFamily(_("Monospace"));
    centralWidget->setFont(font);
    mainWindow.statusBar()->setFont(font);
}

void readFile(FakeVimHandler &handler, const QString &editFileName)
{
    handler.handleCommand(QString(_("r %1")).arg(editFileName));
}

void connectSignals(FakeVimHandler &handler, Proxy &proxy)
{
    QObject::connect(&handler, SIGNAL(commandBufferChanged(QString,int,int,int,QObject*)),
        &proxy, SLOT(changeStatusMessage(QString,int)));
    QObject::connect(&handler,
        SIGNAL(selectionChanged(QList<QTextEdit::ExtraSelection>)),
        &proxy, SLOT(changeSelection(QList<QTextEdit::ExtraSelection>)));
    QObject::connect(&handler, SIGNAL(extraInformationChanged(QString)),
        &proxy, SLOT(changeExtraInformation(QString)));
    QObject::connect(&handler, SIGNAL(statusDataChanged(QString)),
        &proxy, SLOT(changeStatusData(QString)));
    QObject::connect(&handler, SIGNAL(highlightMatches(QString)),
        &proxy, SLOT(highlightMatches(QString)));
    QObject::connect(&handler, SIGNAL(handleExCommandRequested(bool*,ExCommand)),
        &proxy, SLOT(handleExCommand(bool*,ExCommand)));
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QStringList args = app.arguments();

    // If first argument is present use QPlainTextEdit instead on QTextEdit;
    bool usePlainTextEdit = args.size() > 1;
    // Second argument is path to file to edit.
    const QString editFileName = args.value(2, QString(_("/usr/share/vim/vim73/tutor/tutor")));

    // Create editor widget.
    QWidget *editor = createEditorWidget(usePlainTextEdit);

    // Create FakeVimHandler instance which will emulate Vim behavior in editor widget.
    FakeVimHandler handler(editor, 0);

    // Create main window.
    QMainWindow mainWindow;
    initMainWindow(mainWindow, editor, usePlainTextEdit ? _("QPlainTextEdit") : _("QTextEdit"));

    // Connect slots to FakeVimHandler signals.
    Proxy proxy(editor, &mainWindow);
    connectSignals(handler, proxy);

    // Initialize FakeVimHandler.
    initHandler(handler);

    // Read file content to editor.
    readFile(handler, editFileName);

    return app.exec();
}

#include "main.moc"
