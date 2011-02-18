/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef FAKEVIM_HANDLER_H
#define FAKEVIM_HANDLER_H

#include "fakevimactions.h"

#include <QtCore/QObject>
#include <QtGui/QTextEdit>

namespace FakeVim {
namespace Internal {

enum RangeMode
{
    RangeCharMode,         // v
    RangeLineMode,         // V
    RangeLineModeExclusive,
    RangeBlockMode,        // Ctrl-v
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

    QString cmd;
    bool hasBang;
    QString args;
    Range range;
    int count;
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

    void showBlackMessage(const QString &msg);
    void showRedMessage(const QString &msg);

    // This executes an "ex" style command taking context
    // information from the current widget.
    void handleCommand(const QString &cmd);
    void handleReplay(const QString &keys);

    void installEventFilter();

    // Convenience
    void setupWidget();
    void restoreWidget(int tabSize);

    // Test only
    int physicalIndentation(const QString &line) const;
    int logicalIndentation(const QString &line) const;
    QString tabExpand(int n) const;

signals:
    void commandBufferChanged(const QString &msg);
    void statusDataChanged(const QString &msg);
    void extraInformationChanged(const QString &msg);
    void selectionChanged(const QList<QTextEdit::ExtraSelection> &selection);
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
