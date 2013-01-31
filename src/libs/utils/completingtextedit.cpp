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

#include "completingtextedit.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QKeyEvent>
#include <QScrollBar>

static bool isEndOfWordChar(const QChar &c)
{
    return !c.isLetterOrNumber() && c.category() != QChar::Punctuation_Connector;
}

/*! \class Utils::CompletingTextEdit

  \brief A QTextEdit with auto-completion support

  Excerpted from Qt examples/tools/customcompleter
*/

namespace Utils {

class CompletingTextEditPrivate
{
public:
    CompletingTextEditPrivate(CompletingTextEdit *textEdit);

    void insertCompletion(const QString &completion);
    QString textUnderCursor() const;

    bool acceptsCompletionPrefix(const QString &prefix) const;

    QCompleter *m_completer;
    int m_completionLengthThreshold;

private:
    CompletingTextEdit *m_backPointer;
};

CompletingTextEditPrivate::CompletingTextEditPrivate(CompletingTextEdit *textEdit)
    : m_completer(0),
      m_completionLengthThreshold(3),
      m_backPointer(textEdit)
{
}

void CompletingTextEditPrivate::insertCompletion(const QString &completion)
{
    if (m_completer->widget() != m_backPointer)
        return;
    QTextCursor tc = m_backPointer->textCursor();
    int extra = completion.length() - m_completer->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    m_backPointer->setTextCursor(tc);
}

QString CompletingTextEditPrivate::textUnderCursor() const
{
    QTextCursor tc = m_backPointer->textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

bool CompletingTextEditPrivate::acceptsCompletionPrefix(const QString &prefix) const
{
    return prefix.length() >= m_completionLengthThreshold;
}

CompletingTextEdit::CompletingTextEdit(QWidget *parent)
    : QTextEdit(parent),
      d(new CompletingTextEditPrivate(this))
{
}

CompletingTextEdit::~CompletingTextEdit()
{
    delete d;
}

void CompletingTextEdit::setCompleter(QCompleter *c)
{
    if (completer())
        disconnect(completer(), 0, this, 0);

    d->m_completer = c;

    if (!c)
        return;

    completer()->setWidget(this);
    completer()->setCompletionMode(QCompleter::PopupCompletion);
    connect(completer(), SIGNAL(activated(QString)), this, SLOT(insertCompletion(QString)));
}

QCompleter *CompletingTextEdit::completer() const
{
    return d->m_completer;
}

int CompletingTextEdit::completionLengthThreshold() const
{
    return d->m_completionLengthThreshold;
}

void CompletingTextEdit::setCompletionLengthThreshold(int len)
{
    d->m_completionLengthThreshold = len;
}

void CompletingTextEdit::keyPressEvent(QKeyEvent *e)
{
    if (completer() && completer()->popup()->isVisible()) {
        // The following keys are forwarded by the completer to the widget
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return; // let the completer do default behavior
        default:
            break;
        }
    }

    const bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_E); // CTRL+E
    if (completer() == 0 || !isShortcut) // do not process the shortcut when we have a completer
        QTextEdit::keyPressEvent(e);

    const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    if (completer() == 0 || (ctrlOrShift && e->text().isEmpty()))
        return;

    const bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
    const QString newCompletionPrefix = d->textUnderCursor();
    const QChar lastChar = e->text().isEmpty() ? QChar() : e->text().right(1).at(0);

    if (!isShortcut && (hasModifier || e->text().isEmpty() || isEndOfWordChar(lastChar)
                        || !d->acceptsCompletionPrefix(newCompletionPrefix))) {
        completer()->popup()->hide();
        return;
    }

    if (newCompletionPrefix != completer()->completionPrefix()) {
        completer()->setCompletionPrefix(newCompletionPrefix);
        completer()->popup()->setCurrentIndex(completer()->completionModel()->index(0, 0));
    }
    QRect cr = cursorRect();
    cr.setWidth(completer()->popup()->sizeHintForColumn(0)
                + completer()->popup()->verticalScrollBar()->sizeHint().width());
    completer()->complete(cr); // popup it up!
}

void CompletingTextEdit::focusInEvent(QFocusEvent *e)
{
    if (completer() != 0)
        completer()->setWidget(this);
    QTextEdit::focusInEvent(e);
}

} // namespace Utils

#include "moc_completingtextedit.cpp"
