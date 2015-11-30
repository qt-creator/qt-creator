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

#ifndef FAKEVIM_HANDLER_H
#define FAKEVIM_HANDLER_H

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
    Range() {}
    Range(int b, int e, RangeMode m = RangeCharMode);
    QString toString() const;
    bool isValid() const;

    int beginPos = -1;
    int endPos = -1;
    RangeMode rangemode = RangeCharMode;
};

struct ExCommand
{
    ExCommand() {}
    ExCommand(const QString &cmd, const QString &args = QString(),
        const Range &range = Range());

    bool matches(const QString &min, const QString &full) const;

    QString cmd;
    bool hasBang = false;
    QString args;
    Range range;
    int count = 1;
};

// message levels sorted by severity
enum MessageLevel
{
    MessageMode,    // show current mode (format "-- %1 --")
    MessageCommand, // show last Ex command or search
    MessageInfo,    // result of a command
    MessageWarning, // warning
    MessageError,   // error
    MessageShowCmd  // partial command
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

    static void updateGlobalMarksFilenames(const QString &oldFileName, const QString &newFileName);

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

    void miniBufferTextEdited(const QString &text, int cursorPos, int anchorPos);

    // Set text cursor position. Keeps anchor if in visual mode.
    void setTextCursorPosition(int position);

    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor &cursor);

    bool jumpToLocalMark(QChar mark, bool backTickMode);

    bool eventFilter(QObject *ob, QEvent *ev);

signals:
    void commandBufferChanged(const QString &msg, int cursorPos,
        int anchorPos, int messageLevel, QObject *eventFilter);
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
    void windowCommandRequested(const QString &key, int count);
    void findRequested(bool reverse);
    void findNextRequested(bool reverse);
    void handleExCommandRequested(bool *handled, const ExCommand &cmd);
    void requestDisableBlockSelection();
    void requestSetBlockSelection(const QTextCursor&);
    void requestBlockSelection(QTextCursor*);
    void requestHasBlockSelection(bool *on);
    void foldToggle(int depth);
    void foldAll(bool fold);
    void fold(int depth, bool fold);
    void foldGoTo(int count, bool current);
    void jumpToGlobalMark(QChar mark, bool backTickMode, const QString &fileName);

public:
    class Private;

private:
    Private *d;
};

} // namespace Internal
} // namespace FakeVim

Q_DECLARE_METATYPE(FakeVim::Internal::ExCommand)


#endif // FAKEVIM_HANDLER_H
