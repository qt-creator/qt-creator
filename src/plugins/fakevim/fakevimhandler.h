/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef FAKEVIM_HANDLER_H
#define FAKEVIM_HANDLER_H

#include "fakevimactions.h"

#include <QObject>
#include <QTextEdit>

namespace FakeVim {
namespace Internal {

enum RangeMode
{
    // Reordering first three enum items here will break
    // compatibility with clipboard format stored by Vim.
    RangeCharMode,         // v
    RangeLineMode,         // V
    RangeBlockMode,        // Ctrl-v
    RangeLineModeExclusive,
    RangeBlockAndTailMode // Ctrl-v for D and X
};

struct Range
{
    Range();
    Range(int b, int e, RangeMode m = RangeCharMode);
    QString toString() const;

    int beginPos;
    int endPos;
    RangeMode rangemode;
};

struct ExCommand
{
    ExCommand() : hasBang(false), count(1) {}
    ExCommand(const QString &cmd, const QString &args = QString(),
        const Range &range = Range());

    bool matches(const QString &min, const QString &full) const;
    void setContentsFromLine(const QString &line);

    // set cmd to next subcommand and return false only if no subcommand is left
    bool nextSubcommand();

    QString printCommand() const;

    QString cmd;
    QStringList subCommands;
    bool hasBang;
    QString args;
    Range range;
    int count;
};

// message levels sorted by severity
enum MessageLevel
{
    MessageMode,    // show current mode (format "-- %1 --")
    MessageCommand, // show last Ex command or search
    MessageInfo,    // result of a command
    MessageWarning, // warning
    MessageError    // error
};

class FakeVimHandler : public QObject
{
    Q_OBJECT

public:
    explicit FakeVimHandler(QWidget *widget, QObject *parent = 0);
    ~FakeVimHandler();

    QWidget *widget();

    // call before widget is deleted
    void disconnectFromEditor();

public slots:
    void setCurrentFileName(const QString &fileName);
    QString currentFileName() const;

    void showMessage(MessageLevel level, const QString &msg);

    // This executes an "ex" style command taking context
    // information from the current widget.
    void handleCommand(const QString &cmd);
    void handleReplay(const QString &keys);
    void handleInput(const QString &keys);

    void installEventFilter();

    // Convenience
    void setupWidget();
    void restoreWidget(int tabSize);

    // Test only
    int physicalIndentation(const QString &line) const;
    int logicalIndentation(const QString &line) const;
    QString tabExpand(int n) const;

    void miniBufferTextEdited(const QString &text, int cursorPos);

signals:
    void commandBufferChanged(const QString &msg, int pos, int messageLevel, QObject *eventFilter);
    void statusDataChanged(const QString &msg);
    void extraInformationChanged(const QString &msg);
    void selectionChanged(const QList<QTextEdit::ExtraSelection> &selection);
    void highlightMatches(const QString &needle);
    void writeAllRequested(QString *error);
    void moveToMatchingParenthesis(bool *moved, bool *forward, QTextCursor *cursor);
    void checkForElectricCharacter(bool *result, QChar c);
    void indentRegion(int beginLine, int endLine, QChar typedChar);
    void completionRequested();
    void simpleCompletionRequested(const QString &needle, bool forward);
    void windowCommandRequested(int key);
    void findRequested(bool reverse);
    void findNextRequested(bool reverse);
    void handleExCommandRequested(bool *handled, const ExCommand &cmd);
    void requestSetBlockSelection(bool on);
    void requestHasBlockSelection(bool *on);

public:
    class Private;

private:
    bool eventFilter(QObject *ob, QEvent *ev);

    Private *d;
};

} // namespace Internal
} // namespace FakeVim

Q_DECLARE_METATYPE(FakeVim::Internal::ExCommand)


#endif // FAKEVIM_HANDLER_H
