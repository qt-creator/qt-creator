// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "completingtextedit.h"
#include "qtcassert.h"
#include "filepath.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTimer>


static bool isEndOfWordChar(const QChar &c)
{
    return !c.isLetterOrNumber() && c.category() != QChar::Punctuation_Connector;
}

static int findPrefixStart(const QString &text, int cursorPos)
{
    int start = cursorPos;
    while (start > 0) {
        const QChar prevChar = text[start - 1];
        if (prevChar == '\n' || prevChar == ' ')
            break;
        start--;
    }
    return start;
}

/*!
  \class Utils::CompletingTextEdit
  \inmodule QtCreator

  \brief The CompletingTextEdit class is a QTextEdit with auto-completion
  support.

  Excerpted from Qt examples/tools/customcompleter.
*/

namespace Utils {

class CompletingTextEditPrivate
{
public:
    CompletingTextEditPrivate(CompletingTextEdit *textEdit);

    void insertCompletion(const QString &completion);
    QString textUnderCursor() const;

    bool acceptsCompletionPrefix(const QString &prefix) const;

    QPointer<QCompleter> m_completer;
    int m_completionLengthThreshold = 3;

private:
    CompletingTextEdit *m_backPointer;
};

CompletingTextEditPrivate::CompletingTextEditPrivate(CompletingTextEdit *textEdit)
    : m_backPointer(textEdit)
{
}

void CompletingTextEditPrivate::insertCompletion(const QString &completion)
{
    if (!m_completer || m_completer->widget() != m_backPointer)
        return;

    QTextCursor tc = m_backPointer->textCursor();
    switch (m_backPointer->completionBehavior()) {
    case CompletingTextEdit::CompletionBehavior::OnTextChange: {
        const int pos = tc.position();
        const QString text = m_backPointer->toPlainText();
        const int start = findPrefixStart(text, pos);

        // Replace the prefix with the completion
        tc.setPosition(start);
        tc.setPosition(pos, QTextCursor::KeepAnchor);
        tc.insertText(completion);
        m_backPointer->setTextCursor(tc);
        break;
    }
    case CompletingTextEdit::CompletionBehavior::OnKeyPress: {
        int extra = completion.length() - m_completer->completionPrefix().length();
        tc.movePosition(QTextCursor::Left);
        tc.movePosition(QTextCursor::EndOfWord);
        tc.insertText(completion.right(extra));
        m_backPointer->setTextCursor(tc);
        break;
    }
    default:
        QTC_ASSERT(false, return);
    }
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
    connect(this, &QTextEdit::textChanged, this, [this]() {
        if (m_completionBehavior == CompletionBehavior::OnTextChange)
            updateCompletion();
    });
    QTimer::singleShot(0, this, [this]() { updateIconPosition(); });
}

CompletingTextEdit::~CompletingTextEdit()
{
    delete d;
}

void CompletingTextEdit::setCompleter(QCompleter *c)
{
    if (completer() == c)
        return;

    if (completer())
        disconnect(completer(), nullptr, this, nullptr);

    d->m_completer = c;

    if (!c)
        return;

    completer()->setWidget(this);
    completer()->setCompletionMode(QCompleter::PopupCompletion);
    connect(completer(),
        QOverload<const QString &>::of(&QCompleter::activated),
        this, [this](const QString &text) { d->insertCompletion(text); });
}

CompletingTextEdit::CompletionBehavior CompletingTextEdit::completionBehavior() const
{
    return m_completionBehavior;
}

void CompletingTextEdit::setCompletionBehavior(CompletionBehavior behavior)
{
    if (m_completionBehavior == behavior)
        return;

    m_completionBehavior = behavior;

    if (completer()) {
        disconnect(completer(), nullptr, this, nullptr);
        connect(completer(),
            QOverload<const QString &>::of(&QCompleter::activated),
            this, [this](const QString &text) { d->insertCompletion(text); });
    }
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

void CompletingTextEdit::setRightSideIconPath(const FilePath &path)
{
    if (!path.isEmpty()) {
        QIcon icon(path.toFSPathString());
        QTC_CHECK(!icon.isNull());
        setRightSideIcon(icon);
        return;
    }

    setRightSideIcon(QIcon());
}

void CompletingTextEdit::setRightSideIcon(const QIcon &icon)
{
    if (!m_rightButton) {
        m_rightButton = new FancyIconButton(this);
        m_rightButton->hide();
        m_rightButton->setAutoHide(false);
        connect(m_rightButton, &QAbstractButton::clicked,
            this, &CompletingTextEdit::handleIconClick);
    }

    if (icon.isNull()) {
        m_rightButton->hide();
        // Defer margin update to avoid temporary incorrect layout
        QTimer::singleShot(0, this, [this]() { updateTextMargins(); });
    } else {
        m_rightButton->setIcon(icon);
        m_rightButton->show();
        // Defer margin and position updates to avoid temporary incorrect layout
        QTimer::singleShot(0, this, [this]() {
            updateTextMargins();
            updateIconPosition();
        });
    }
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

    if (m_completionBehavior == CompletionBehavior::OnTextChange) {
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            if (e->modifiers() & Qt::ShiftModifier) {
                // Shift+Enter: insert newline
                QTextEdit::keyPressEvent(e);
            } else {
                emit returnPressed();
            }
            return;
        }

        QTextEdit::keyPressEvent(e);
        updateCompletion();
        return;
    }

    const bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_E); // CTRL+E
    if (completer() == nullptr || !isShortcut) // do not process the shortcut when we have a completer
        QTextEdit::keyPressEvent(e);

    const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    const QString text = e->text();
    if (completer() == nullptr || (ctrlOrShift && text.isEmpty()))
        return;

    const bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
    const QString newCompletionPrefix = d->textUnderCursor();
    const QChar lastChar = text.isEmpty() ? QChar() : text.right(1).at(0);

    if (!isShortcut && (hasModifier || text.isEmpty() || isEndOfWordChar(lastChar)
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
    if (completer() != nullptr)
        completer()->setWidget(this);
    QTextEdit::focusInEvent(e);
}

bool CompletingTextEdit::event(QEvent *e)
{
    // workaround for QTCREATORBUG-9453
    if (e->type() == QEvent::ShortcutOverride && completer()
            && completer()->popup() && completer()->popup()->isVisible()) {
        auto ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            ke->accept();
            return true;
        }
    }
    return QTextEdit::event(e);
}

void CompletingTextEdit::resizeEvent(QResizeEvent *e)
{
    QTextEdit::resizeEvent(e);
    updateTextMargins();
    updateIconPosition();
}

void CompletingTextEdit::updateTextMargins()
{
    if (m_rightButton && m_rightButton->isVisible()) {
        const QSize buttonSize = m_rightButton->sizeHint();
        const int rightMargin = buttonSize.width() + 8;
        setViewportMargins(0, 0, rightMargin, 0);
        return;
    }

    setViewportMargins(0, 0, 0, 0);
}

void CompletingTextEdit::updateIconPosition()
{
    if (!m_rightButton || !m_rightButton->isVisible())
        return;

    const QRect rect = this->rect();
    const int frameWidth = this->frameWidth();
    const int scrollBarWidth = verticalScrollBar()->isVisible() ? verticalScrollBar()->width() : 0;

    // Position on the right side, vertically centered
    const QSize buttonSize = m_rightButton->sizeHint();
    const int x = rect.right() - frameWidth - scrollBarWidth - buttonSize.width() - 4;
    const int y = rect.top() + frameWidth
                  + (rect.height() - 2 * frameWidth - buttonSize.height()) / 2;

    m_rightButton->setGeometry(x, y, buttonSize.width(), buttonSize.height());
}

void CompletingTextEdit::handleIconClick()
{
    emit rightSideIconClicked();
}

QString CompletingTextEdit::getCompletionPrefix() const
{
    const QTextCursor tc = textCursor();
    const int pos = tc.position();
    const QString text = toPlainText();
    const int start = findPrefixStart(text, pos);
    return text.mid(start, pos - start);
}

void CompletingTextEdit::updateCompletion()
{
    if (!completer())
        return;

    const QString prefix = getCompletionPrefix();

    if (prefix.isEmpty()) {
        completer()->popup()->hide();
        return;
    }

    if (prefix != completer()->completionPrefix()) {
        completer()->setCompletionPrefix(prefix);
        completer()->popup()->setCurrentIndex(completer()->completionModel()->index(0, 0));
    }

    const int matchCount = completer()->completionCount();
    if (matchCount > 0) {
        showCompleterPopup();
    } else {
        completer()->popup()->hide();
    }
}

void CompletingTextEdit::showCompleterPopup()
{
    QRect cr = cursorRect();

    // Position popup below the cursor line, at the start of line
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::StartOfLine);
    const QRect lineStartRect = cursorRect(tc);

    // Set position: start of line horizontally, below cursor line vertically
    cr.setX(lineStartRect.x());
    cr.setY(cr.y() + cr.height());
    cr.setWidth(
        completer()->popup()->sizeHintForColumn(0)
        + completer()->popup()->verticalScrollBar()->sizeHint().width());

    // Show popup and let Qt initialize it, then override position
    completer()->complete(cr);

    // Force the popup to appear at the correct position
    if (completer()->popup()->isVisible()) {
        const QPoint globalPos = mapToGlobal(cr.topLeft());
        completer()->popup()->move(globalPos);
    }
}

} // namespace Utils

#include "moc_completingtextedit.cpp"
