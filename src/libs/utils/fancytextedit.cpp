// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fancytextedit.h"

#include "fancyiconbutton.h"
#include "hostosinfo.h"
#include "icon.h"
#include "theme/theme.h"
#include "utilstr.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCompleter>
#include <QCoreApplication>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QStyleOptionFrame>
#include <QStylePainter>
#include <QTextCursor>

/*!
    \class Utils::FancyTextEdit
    \inmodule QtCreator

    \brief The FancyTextEdit class is a line edit that can hold several lines.

    It looks like a FancyLineEdit and offers the subset of its features that
    the find tool bar needs: an icon button on each side, a clear button,
    a validation function that colors the text, and a completer.

    The field is as tall as a line edit as long as the text has one line,
    grows with the number of lines up to maximumVisibleLines(), and scrolls
    beyond that.

    Return emits returnPressed(). Ctrl+Return (Cmd+Return on \macos) inserts
    a line break. Shift+Return is left to shortcuts or actions that the user
    of the widget has registered.
*/

namespace Utils {

static const int horizontalMargin = 2;

class FancyTextEditPrivate : public QObject
{
public:
    FancyTextEditPrivate(FancyTextEdit *parent);

    bool eventFilter(QObject *obj, QEvent *event) override;

    bool handleKeyPress(QKeyEvent *event);
    bool handleShortcutOverride(QKeyEvent *event);
    void textChanged();
    void updateCompletion();
    void completeFromCursor(bool useWholeText);
    bool completionPopupVisible() const;

    int sideMargin(FancyTextEdit::Side side) const;
    int singleLineHeight() const;
    int heightForLines(int lines) const;
    void updateLayout();
    void updateClearButton();

    FancyTextEdit *q;
    QPlainTextEdit *m_edit;
    FancyIconButton *m_buttons[2];
    bool m_buttonEnabled[2] = {false, false};
    QCompleter *m_completer = nullptr;
    FancyTextEdit::ValidationFunction m_validationFunction;
    int m_maximumVisibleLines = 6;
    mutable int m_singleLineHeight = -1;
    mutable bool m_measuring = false;
    bool m_clearButtonEnabled = false;
    bool m_changingProgrammatically = false;
    bool m_completionPending = false;
    bool m_hadText = false;
    bool m_forwardingKey = false;
    const QKeyEvent *m_ignoredKeyEvent = nullptr;
};

FancyTextEditPrivate::FancyTextEditPrivate(FancyTextEdit *parent)
    : QObject(parent)
    , q(parent)
    , m_edit(new QPlainTextEdit(parent))
{
    m_edit->setFrameShape(QFrame::NoFrame);
    m_edit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_edit->setTabChangesFocus(true);
    m_edit->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_edit->viewport()->setAutoFillBackground(false);
    m_edit->document()->setDocumentMargin(horizontalMargin);
    m_edit->installEventFilter(this);
    q->setFocusProxy(m_edit);

    for (int i = 0; i < 2; ++i) {
        m_buttons[i] = new FancyIconButton(parent);
        m_buttons[i]->hide();
        m_buttons[i]->setAutoHide(false);
    }

    connect(m_edit, &QPlainTextEdit::textChanged, this, &FancyTextEditPrivate::textChanged);
    connect(m_edit->document(), &QTextDocument::blockCountChanged, this, [this] {
        q->updateGeometry();
        updateLayout();
    });
    connect(m_buttons[FancyTextEdit::Left], &QAbstractButton::clicked,
            q, &FancyTextEdit::leftButtonClicked);
    connect(m_buttons[FancyTextEdit::Right], &QAbstractButton::clicked,
            q, &FancyTextEdit::rightButtonClicked);
}

int FancyTextEditPrivate::sideMargin(FancyTextEdit::Side side) const
{
    return m_buttonEnabled[side] ? m_buttons[side]->sizeHint().width() + 8 : 0;
}

// A line edit in a panel is forced to a fixed height by the style, so ask one.
int FancyTextEditPrivate::singleLineHeight() const
{
    const int textHeight = q->fontMetrics().lineSpacing()
                           + 2 * int(m_edit->document()->documentMargin());
    // Polishing the probe makes this widget see style and font changes.
    if (m_measuring)
        return textHeight;
    if (m_singleLineHeight < 0) {
        m_measuring = true;
        QLineEdit probe(q);
        probe.hide();
        probe.ensurePolished();
        const int probeHeight = probe.minimumHeight() > 0 ? probe.minimumHeight()
                                                          : probe.sizeHint().height();
        m_measuring = false;
        m_singleLineHeight = qMax(probeHeight, textHeight);
    }
    return m_singleLineHeight;
}

int FancyTextEditPrivate::heightForLines(int lines) const
{
    const int visibleLines = qBound(1, lines, m_maximumVisibleLines);
    return singleLineHeight() + (visibleLines - 1) * q->fontMetrics().lineSpacing();
}

void FancyTextEditPrivate::updateLayout()
{
    const int height = q->height();
    const int width = q->width();
    const int leftMargin = sideMargin(FancyTextEdit::Left);
    const int rightMargin = sideMargin(FancyTextEdit::Right);

    const int editHeight = heightForLines(m_edit->blockCount()) - singleLineHeight()
                           + q->fontMetrics().lineSpacing()
                           + 2 * int(m_edit->document()->documentMargin());
    const int editTop = qMax(0, (singleLineHeight() - q->fontMetrics().lineSpacing()
                                 - 2 * int(m_edit->document()->documentMargin())) / 2);
    // The scroll bar would show up for lines that fit, as the margin counts into the document size.
    m_edit->setVerticalScrollBarPolicy(m_edit->blockCount() > m_maximumVisibleLines
                                           ? Qt::ScrollBarAsNeeded
                                           : Qt::ScrollBarAlwaysOff);
    const int frame = 1;
    m_edit->setGeometry(frame + leftMargin, editTop,
                        qMax(0, width - 2 * frame - leftMargin - rightMargin),
                        qMin(editHeight, height - editTop));

    const int buttonHeight = singleLineHeight();
    m_buttons[FancyTextEdit::Left]->setGeometry(0, 0, leftMargin + 4, buttonHeight);
    m_buttons[FancyTextEdit::Right]->setGeometry(width - rightMargin - 4, 0,
                                                 rightMargin + 4, buttonHeight);
}

void FancyTextEditPrivate::updateClearButton()
{
    if (!m_clearButtonEnabled)
        return;
    const bool hasText = m_edit->blockCount() > 1 || !m_edit->document()->isEmpty();
    if (hasText != m_hadText) {
        m_buttons[FancyTextEdit::Right]->animateShow(hasText);
        m_hadText = hasText;
    }
}

bool FancyTextEditPrivate::completionPopupVisible() const
{
    return m_completer && m_completer->popup() && m_completer->popup()->isVisible();
}

void FancyTextEditPrivate::completeFromCursor(bool useWholeText)
{
    if (!m_completer)
        return;
    QString prefix = m_edit->toPlainText();
    if (!useWholeText)
        prefix.truncate(m_edit->textCursor().position());
    m_completer->setCompletionPrefix(prefix);
    m_completer->complete();
}

void FancyTextEditPrivate::updateCompletion()
{
    m_completionPending = false;
    if (!m_completer)
        return;
    const QString text = m_edit->toPlainText();
    if (text.isEmpty()) {
        m_completer->popup()->hide();
        return;
    }
    m_completer->setCompletionPrefix(text);
    if (m_completer->completionCount() == 0)
        m_completer->popup()->hide();
    else
        m_completer->complete();
}

void FancyTextEditPrivate::textChanged()
{
    updateClearButton();
    q->validate();
    emit q->textChanged();
    if (m_completer && !m_changingProgrammatically && !m_completionPending) {
        // The completer hides its popup after the widget accepted a key event,
        // so only update it once the key event has been fully handled.
        m_completionPending = true;
        QMetaObject::invokeMethod(this, &FancyTextEditPrivate::updateCompletion,
                                  Qt::QueuedConnection);
    }
}

bool FancyTextEditPrivate::handleShortcutOverride(QKeyEvent *event)
{
    const Qt::KeyboardModifiers modifiers = event->modifiers() & ~Qt::KeypadModifier;
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        // Only the keys handled in handleKeyPress() are taken from the shortcuts. Let a
        // shortcut or action bound to Shift+Return or any other combination win.
        if (modifiers == Qt::NoModifier || modifiers == Qt::ControlModifier) {
            event->accept();
            return true;
        }
        if (modifiers == Qt::ShiftModifier) {
            event->ignore();
            return true;
        }
        return false;
    case Qt::Key_Space:
        if (modifiers == HostOsInfo::controlModifier()) {
            event->accept();
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

bool FancyTextEditPrivate::handleKeyPress(QKeyEvent *event)
{
    m_ignoredKeyEvent = nullptr; // The delivery of an earlier key is over
    const Qt::KeyboardModifiers modifiers = event->modifiers() & ~Qt::KeypadModifier;
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (modifiers == Qt::ControlModifier) {
            m_edit->insertPlainText("\n");
            event->accept();
            return true;
        }
        if (modifiers != Qt::NoModifier)
            return false;
        emit q->returnPressed();
        emit q->editingFinished();
        // Let the completer see the key, too, if it is showing.
        if (completionPopupVisible()) {
            event->ignore();
            m_ignoredKeyEvent = event;
        } else {
            event->accept();
        }
        return true;
    case Qt::Key_Down:
        if (modifiers == Qt::NoModifier && m_completer && !completionPopupVisible()
            && m_edit->textCursor().blockNumber() == m_edit->blockCount() - 1) {
            completeFromCursor(true);
            event->accept();
            return true;
        }
        break;
    case Qt::Key_Space:
        if (modifiers == HostOsInfo::controlModifier() && m_completer) {
            completeFromCursor(false);
            event->accept();
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

bool FancyTextEditPrivate::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_edit)
        return QObject::eventFilter(obj, event);

    switch (event->type()) {
    case QEvent::KeyPress:
        return handleKeyPress(static_cast<QKeyEvent *>(event));
    case QEvent::ShortcutOverride:
        return handleShortcutOverride(static_cast<QKeyEvent *>(event));
    case QEvent::FocusIn:
        q->update();
        break;
    case QEvent::FocusOut: {
        q->update();
        const Qt::FocusReason reason = static_cast<QFocusEvent *>(event)->reason();
        if (reason != Qt::PopupFocusReason) {
            if (m_completer)
                m_completer->popup()->hide();
            emit q->editingFinished();
        }
        break;
    }
    default:
        break;
    }
    return false;
}

FancyTextEdit::FancyTextEdit(QWidget *parent)
    : QWidget(parent)
    , d(new FancyTextEditPrivate(this))
{
    setAttribute(Qt::WA_Hover);
    // QWidget::setTabOrder() ignores widgets without a focus policy, also if they have a focus proxy.
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    validate();
}

FancyTextEdit::~FancyTextEdit() = default;

QString FancyTextEdit::text() const
{
    return d->m_edit->toPlainText();
}

void FancyTextEdit::setText(const QString &text)
{
    const bool changing = d->m_changingProgrammatically;
    d->m_changingProgrammatically = true;
    d->m_edit->setPlainText(text);
    d->m_edit->moveCursor(QTextCursor::End);
    d->m_changingProgrammatically = changing;
}

void FancyTextEdit::selectAll()
{
    d->m_edit->selectAll();
}

QString FancyTextEdit::placeholderText() const
{
    return d->m_edit->placeholderText();
}

void FancyTextEdit::setPlaceholderText(const QString &text)
{
    d->m_edit->setPlaceholderText(text);
}

int FancyTextEdit::lineCount() const
{
    return d->m_edit->blockCount();
}

int FancyTextEdit::maximumVisibleLines() const
{
    return d->m_maximumVisibleLines;
}

void FancyTextEdit::setMaximumVisibleLines(int lines)
{
    d->m_maximumVisibleLines = qMax(1, lines);
    updateGeometry();
    d->updateLayout();
}

void FancyTextEdit::setButtonIcon(Side side, const QIcon &icon)
{
    d->m_buttons[side]->setIcon(icon);
    d->updateLayout();
    update();
}

void FancyTextEdit::setButtonVisible(Side side, bool visible)
{
    d->m_buttons[side]->setVisible(visible);
    d->m_buttonEnabled[side] = visible;
    updateGeometry();
    d->updateLayout();
}

QAbstractButton *FancyTextEdit::button(Side side) const
{
    return d->m_buttons[side];
}

void FancyTextEdit::setClearButtonEnabled(bool enabled)
{
    if (enabled == d->m_clearButtonEnabled)
        return;
    d->m_clearButtonEnabled = enabled;
    FancyIconButton *clearButton = d->m_buttons[Right];
    if (enabled) {
        // KDE has custom icons for this. The "ltr" and "rtl" suffixes describe the direction
        // into which the arrows are pointing, not the writing direction they are meant for.
        const QLatin1String pointingWest("edit-clear-locationbar-rtl");
        const QLatin1String pointingEast("edit-clear-locationbar-ltr");
        setButtonIcon(Right, Icon::fromTheme(layoutDirection() == Qt::LeftToRight ? pointingWest
                                                                                  : pointingEast));
        setButtonVisible(Right, true);
        clearButton->setToolTip(Tr::tr("Clear text"));
        clearButton->setAutoHide(true);
        d->m_hadText = !d->m_edit->document()->isEmpty();
        clearButton->setIconOpacity(d->m_hadText ? 1.0 : 0.0);
        connect(this, &FancyTextEdit::rightButtonClicked, d->m_edit, &QPlainTextEdit::clear);
    } else {
        clearButton->setAutoHide(false);
        clearButton->setIconOpacity(1.0);
        disconnect(this, &FancyTextEdit::rightButtonClicked, d->m_edit, &QPlainTextEdit::clear);
    }
}

void FancyTextEdit::setSpecialCompleter(QCompleter *completer)
{
    if (d->m_completer)
        disconnect(d->m_completer, nullptr, d, nullptr);
    d->m_completer = completer;
    if (!completer)
        return;
    completer->setWidget(this);
    connect(completer, QOverload<const QString &>::of(&QCompleter::activated), d,
            [this](const QString &text) { setText(text); });
}

void FancyTextEdit::setValidationFunction(const ValidationFunction &function)
{
    d->m_validationFunction = function;
    validate();
}

void FancyTextEdit::validate()
{
    QString errorMessage;
    bool valid = true;
    if (d->m_validationFunction) {
        if (const Result<> result = d->m_validationFunction(text()); !result) {
            valid = false;
            errorMessage = result.error();
        }
    }
    setToolTip(errorMessage);

    QPalette palette = d->m_edit->palette();
    palette.setColor(QPalette::Active, QPalette::Text,
                     creatorColor(valid ? Theme::TextColorNormal : Theme::TextColorError));
    palette.setColor(QPalette::Active, QPalette::PlaceholderText,
                     QApplication::palette().color(QPalette::PlaceholderText));
    d->m_edit->setPalette(palette);
}

/*!
    Installs \a filter on the child widget that has the keyboard focus and
    receives the key events.
*/
void FancyTextEdit::installEditFilter(QObject *filter)
{
    d->m_edit->installEventFilter(filter);
}

QSize FancyTextEdit::sizeHint() const
{
    const int width = fontMetrics().horizontalAdvance(QLatin1Char('x')) * 17 + 2 * horizontalMargin
                      + d->sideMargin(Left) + d->sideMargin(Right);
    return {width, d->heightForLines(d->m_edit->blockCount())};
}

QSize FancyTextEdit::minimumSizeHint() const
{
    return {d->sideMargin(Left) + d->sideMargin(Right) + 20, sizeHint().height()};
}

void FancyTextEdit::paintEvent(QPaintEvent *)
{
    QStylePainter painter(this);
    QStyleOptionFrame panel;
    panel.initFrom(this);
    panel.rect = rect();
    panel.lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &panel, this);
    panel.midLineWidth = 0;
    panel.state |= QStyle::State_Sunken;
    painter.drawPrimitive(QStyle::PE_PanelLineEdit, panel);
}

void FancyTextEdit::resizeEvent(QResizeEvent *)
{
    d->updateLayout();
}

// The completer forwards the keys it does not handle itself to this widget.
void FancyTextEdit::keyPressEvent(QKeyEvent *event)
{
    // A key the edit ignores propagates back here.
    if (d->m_forwardingKey || event == d->m_ignoredKeyEvent) {
        d->m_ignoredKeyEvent = nullptr;
        event->ignore();
        return;
    }
    d->m_forwardingKey = true;
    QCoreApplication::sendEvent(d->m_edit, event);
    d->m_forwardingKey = false;
    d->m_ignoredKeyEvent = nullptr; // The event does not outlive its delivery
}

void FancyTextEdit::changeEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::StyleChange:
    case QEvent::FontChange:
    case QEvent::ParentChange:
        if (!d->m_measuring) {
            d->m_singleLineHeight = -1;
            updateGeometry();
            QMetaObject::invokeMethod(d, &FancyTextEditPrivate::updateLayout, Qt::QueuedConnection);
        }
        break;
    default:
        break;
    }
    QWidget::changeEvent(event);
}

} // namespace Utils

#include "moc_fancytextedit.cpp"
