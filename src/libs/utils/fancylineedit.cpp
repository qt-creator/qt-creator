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

#include "execmenu.h"
#include "fancylineedit.h"
#include "historycompleter.h"
#include "hostosinfo.h"
#include "qtcassert.h"
#include "stylehelper.h"
#include "utilsicons.h"

#include <QAbstractItemView>
#include <QDebug>
#include <QKeyEvent>
#include <QMenu>
#include <QStylePainter>
#include <QPropertyAnimation>
#include <QStyle>

/*!
    \class Utils::FancyLineEdit

    \brief The FancyLineEdit class is an enhanced line edit with several
    opt-in features.

    A FancyLineEdit instance can have:

    \list
    \li An embedded pixmap on one side that is connected to a menu.

    \li A grayed hintText (like "Type Here to")
    when not focused and empty. When connecting to the changed signals and
    querying text, one has to be aware that the text is set to that hint
    text if isShowingHintText() returns true (that is, does not contain
    valid user input).

    \li A history completer.

    \li The ability to validate the contents of the text field by overriding
    virtual \c validate() function in derived clases.
    \endlist

    When invalid, the text color will turn red and a tooltip will
    contain the error message. This approach is less intrusive than a
    QValidator which will prevent the user from entering certain characters.

    A visible hint text results validation to be in state 'DisplayingInitialText',
    which is not valid, but is not marked red.

 */

enum { margin = 6 };

#define ICONBUTTON_HEIGHT 18
#define FADE_TIME 160

namespace Utils {

// --------- FancyLineEditPrivate
class FancyLineEditPrivate : public QObject
{
public:
    explicit FancyLineEditPrivate(FancyLineEdit *parent);

    virtual bool eventFilter(QObject *obj, QEvent *event);

    FancyLineEdit *m_lineEdit;
    IconButton *m_iconbutton[2];
    HistoryCompleter *m_historyCompleter = 0;
    FancyLineEdit::ValidationFunction m_validationFunction = &FancyLineEdit::validateWithValidator;
    QString m_oldText;
    QPixmap m_pixmap[2];
    QMenu *m_menu[2];
    FancyLineEdit::State m_state = FancyLineEdit::Invalid;
    bool m_menuTabFocusTrigger[2];
    bool m_iconEnabled[2];

    bool m_isFiltering = false;
    bool m_firstChange = true;

    QString m_lastFilterText;

    QColor m_okTextColor;
    QColor m_errorTextColor = Qt::red;
    QString m_errorMessage;
    QString m_initialText;
};

FancyLineEditPrivate::FancyLineEditPrivate(FancyLineEdit *parent) :
    QObject(parent),
    m_lineEdit(parent)
{
    m_okTextColor = parent->palette().color(QPalette::Active, QPalette::Text);

    for (int i = 0; i < 2; ++i) {
        m_iconbutton[i] = new IconButton(parent);
        m_iconbutton[i]->installEventFilter(this);
        m_iconbutton[i]->hide();
        m_iconbutton[i]->setAutoHide(false);

        m_menu[i] = 0;

        m_menuTabFocusTrigger[i] = false;
        m_iconEnabled[i] = false;
    }
}

bool FancyLineEditPrivate::eventFilter(QObject *obj, QEvent *event)
{
    int buttonIndex = -1;
    for (int i = 0; i < 2; ++i) {
        if (obj == m_iconbutton[i]) {
            buttonIndex = i;
            break;
        }
    }
    if (buttonIndex == -1)
        return QObject::eventFilter(obj, event);
    switch (event->type()) {
    case QEvent::FocusIn:
        if (m_menuTabFocusTrigger[buttonIndex] && m_menu[buttonIndex]) {
            m_lineEdit->setFocus();
            execMenuAtWidget(m_menu[buttonIndex], m_iconbutton[buttonIndex]);
            return true;
        }
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}


// --------- FancyLineEdit
FancyLineEdit::FancyLineEdit(QWidget *parent) :
    CompletingLineEdit(parent),
    d(new FancyLineEditPrivate(this))
{
    ensurePolished();
    updateMargins();

    connect(d->m_iconbutton[Left], &QAbstractButton::clicked, this, &FancyLineEdit::iconClicked);
    connect(d->m_iconbutton[Right], &QAbstractButton::clicked, this, &FancyLineEdit::iconClicked);
    connect(this, &QLineEdit::textChanged, this, &FancyLineEdit::validate);
}

FancyLineEdit::~FancyLineEdit()
{
    if (d->m_historyCompleter) {
        // When dialog with FancyLineEdit widget closed by <Escape>
        // the QueuedConnection don't have enough time to call slot callback
        // because edit widget and all of its connections are destroyed before
        // QCoreApplicationPrivate::sendPostedEvents dispatch our queued signal.
        d->m_historyCompleter->addEntry(text());
    }
}

void FancyLineEdit::setButtonVisible(Side side, bool visible)
{
    d->m_iconbutton[side]->setVisible(visible);
    d->m_iconEnabled[side] = visible;
    updateMargins();
}

bool FancyLineEdit::isButtonVisible(Side side) const
{
    return d->m_iconEnabled[side];
}

QAbstractButton *FancyLineEdit::button(FancyLineEdit::Side side) const
{
    return d->m_iconbutton[side];
}

void FancyLineEdit::iconClicked()
{
    IconButton *button = qobject_cast<IconButton *>(sender());
    int index = -1;
    for (int i = 0; i < 2; ++i)
        if (d->m_iconbutton[i] == button)
            index = i;
    if (index == -1)
        return;
    if (d->m_menu[index]) {
        execMenuAtWidget(d->m_menu[index], button);
    } else {
        emit buttonClicked((Side)index);
        if (index == Left)
            emit leftButtonClicked();
        else if (index == Right)
            emit rightButtonClicked();
    }
}

void FancyLineEdit::updateMargins()
{
    bool leftToRight = (layoutDirection() == Qt::LeftToRight);
    Side realLeft = (leftToRight ? Left : Right);
    Side realRight = (leftToRight ? Right : Left);

    int leftMargin = d->m_iconbutton[realLeft]->sizeHint().width() + 8;
    int rightMargin = d->m_iconbutton[realRight]->sizeHint().width() + 8;
    // Note KDE does not reserve space for the highlight color
    if (style()->inherits("OxygenStyle")) {
        leftMargin = qMax(24, leftMargin);
        rightMargin = qMax(24, rightMargin);
    }

    QMargins margins((d->m_iconEnabled[realLeft] ? leftMargin : 0), 0,
                     (d->m_iconEnabled[realRight] ? rightMargin : 0), 0);

    setTextMargins(margins);
}

void FancyLineEdit::updateButtonPositions()
{
    QRect contentRect = rect();
    for (int i = 0; i < 2; ++i) {
        Side iconpos = (Side)i;
        if (layoutDirection() == Qt::RightToLeft)
            iconpos = (iconpos == Left ? Right : Left);

        if (iconpos == FancyLineEdit::Right) {
            const int iconoffset = textMargins().right() + 4;
            d->m_iconbutton[i]->setGeometry(contentRect.adjusted(width() - iconoffset, 0, 0, 0));
        } else {
            const int iconoffset = textMargins().left() + 4;
            d->m_iconbutton[i]->setGeometry(contentRect.adjusted(0, 0, -width() + iconoffset, 0));
        }
    }
}

void FancyLineEdit::resizeEvent(QResizeEvent *)
{
    updateButtonPositions();
}

void FancyLineEdit::setButtonPixmap(Side side, const QPixmap &buttonPixmap)
{
    d->m_iconbutton[side]->setPixmap(buttonPixmap);
    updateMargins();
    updateButtonPositions();
    update();
}

QPixmap FancyLineEdit::buttonPixmap(Side side) const
{
    return d->m_pixmap[side];
}

void FancyLineEdit::setButtonMenu(Side side, QMenu *buttonMenu)
{
     d->m_menu[side] = buttonMenu;
     d->m_iconbutton[side]->setIconOpacity(1.0);
 }

QMenu *FancyLineEdit::buttonMenu(Side side) const
{
    return  d->m_menu[side];
}

bool FancyLineEdit::hasMenuTabFocusTrigger(Side side) const
{
    return d->m_menuTabFocusTrigger[side];
}

void FancyLineEdit::setMenuTabFocusTrigger(Side side, bool v)
{
    if (d->m_menuTabFocusTrigger[side] == v)
        return;

    d->m_menuTabFocusTrigger[side] = v;
    d->m_iconbutton[side]->setFocusPolicy(v ? Qt::TabFocus : Qt::NoFocus);
}

bool FancyLineEdit::hasAutoHideButton(Side side) const
{
    return d->m_iconbutton[side]->hasAutoHide();
}

void FancyLineEdit::setHistoryCompleter(const QString &historyKey, bool restoreLastItemFromHistory)
{
    QTC_ASSERT(!d->m_historyCompleter, return);
    d->m_historyCompleter = new HistoryCompleter(historyKey, this);
    if (restoreLastItemFromHistory)
        setText(d->m_historyCompleter->historyItem());
    QLineEdit::setCompleter(d->m_historyCompleter);

    // Hitting <Return> in the popup first causes editingFinished()
    // being emitted and more updates finally calling setText() (again).
    // To make sure we report the "final" content delay the addEntry()
    // "a bit".
    connect(this, &QLineEdit::editingFinished,
            this, &FancyLineEdit::onEditingFinished, Qt::QueuedConnection);
}

void FancyLineEdit::onEditingFinished()
{
    d->m_historyCompleter->addEntry(text());
}

void FancyLineEdit::setSpecialCompleter(QCompleter *completer)
{
    QTC_ASSERT(!d->m_historyCompleter, return);
    QLineEdit::setCompleter(completer);
}

void FancyLineEdit::setAutoHideButton(Side side, bool h)
{
    d->m_iconbutton[side]->setAutoHide(h);
    if (h)
        d->m_iconbutton[side]->setIconOpacity(text().isEmpty() ?  0.0 : 1.0);
    else
        d->m_iconbutton[side]->setIconOpacity(1.0);
}

void FancyLineEdit::setButtonToolTip(Side side, const QString &tip)
{
    d->m_iconbutton[side]->setToolTip(tip);
}

void FancyLineEdit::setButtonFocusPolicy(Side side, Qt::FocusPolicy policy)
{
    d->m_iconbutton[side]->setFocusPolicy(policy);
}

void FancyLineEdit::setFiltering(bool on)
{
    if (on == d->m_isFiltering)
        return;

    d->m_isFiltering = on;
    if (on) {
        d->m_lastFilterText = text();
        // KDE has custom icons for this. Notice that icon namings are counter intuitive.
        // If these icons are not available we use the freedesktop standard name before
        // falling back to a bundled resource.
        QIcon icon = QIcon::fromTheme(layoutDirection() == Qt::LeftToRight ?
                         QLatin1String("edit-clear-locationbar-rtl") :
                         QLatin1String("edit-clear-locationbar-ltr"),
                         QIcon::fromTheme(QLatin1String("edit-clear"),
                                          Icons::EDIT_CLEAR.pixmap()));

        setButtonPixmap(Right, icon.pixmap(16));
        setButtonVisible(Right, true);
        setPlaceholderText(tr("Filter"));
        setButtonToolTip(Right, tr("Clear text"));
        setAutoHideButton(Right, true);
        connect(this, &FancyLineEdit::rightButtonClicked, this, &QLineEdit::clear);
    } else {
        disconnect(this, &FancyLineEdit::rightButtonClicked, this, &QLineEdit::clear);
    }
}

QString FancyLineEdit::initialText() const
{
    return d->m_initialText;
}

void FancyLineEdit::setInitialText(const QString &t)
{
    if (d->m_initialText != t) {
        d->m_initialText = t;
        d->m_firstChange = true;
        setText(t);
    }
}

QColor FancyLineEdit::errorColor() const
{
    return d->m_errorTextColor;
}

void FancyLineEdit::setErrorColor(const  QColor &c)
{
    d->m_errorTextColor = c;
    validate();
}

QColor FancyLineEdit::okColor() const
{
    return d->m_okTextColor;
}

void FancyLineEdit::setOkColor(const QColor &c)
{
    d->m_okTextColor = c;
    validate();
}

void FancyLineEdit::setValidationFunction(const FancyLineEdit::ValidationFunction &fn)
{
    d->m_validationFunction = fn;
    validate();
}

FancyLineEdit::ValidationFunction FancyLineEdit::defaultValidationFunction()
{
    return &FancyLineEdit::validateWithValidator;
}

bool FancyLineEdit::validateWithValidator(FancyLineEdit *edit, QString *errorMessage)
{
    Q_UNUSED(errorMessage);
    if (const QValidator *v = edit->validator()) {
        QString tmp = edit->text();
        int pos = edit->cursorPosition();
        return v->validate(tmp, pos) == QValidator::Acceptable;
    }
    return true;
}

FancyLineEdit::State FancyLineEdit::state() const
{
    return d->m_state;
}

bool FancyLineEdit::isValid() const
{
    return d->m_state == Valid;
}

QString FancyLineEdit::errorMessage() const
{
    return d->m_errorMessage;
}

void FancyLineEdit::validate()
{
    const QString t = text();

    if (d->m_isFiltering){
        if (t != d->m_lastFilterText) {
            d->m_lastFilterText = t;
            emit filterChanged(t);
        }
    }

    d->m_errorMessage.clear();
    // Are we displaying the initial text?
    const bool isDisplayingInitialText = !d->m_initialText.isEmpty() && t == d->m_initialText;
    const State newState = isDisplayingInitialText ?
                               DisplayingInitialText :
                               (d->m_validationFunction(this, &d->m_errorMessage) ? Valid : Invalid);
    setToolTip(d->m_errorMessage);
    // Changed..figure out if valid changed. DisplayingInitialText is not valid,
    // but should not show error color. Also trigger on the first change.
    if (newState != d->m_state || d->m_firstChange) {
        const bool validHasChanged = (d->m_state == Valid) != (newState == Valid);
        d->m_state = newState;
        d->m_firstChange = false;

        QPalette p = palette();
        p.setColor(QPalette::Active, QPalette::Text, newState == Invalid ? d->m_errorTextColor : d->m_okTextColor);
        setPalette(p);

        if (validHasChanged)
            emit validChanged(newState == Valid);
    }
    bool block = blockSignals(true);
    const QString fixedString = fixInputString(t);
    if (t != fixedString) {
        const int cursorPos = cursorPosition();
        setText(fixedString);
        setCursorPosition(qMin(cursorPos, fixedString.length()));
    }
    blockSignals(block);

    // Check buttons.
    if (d->m_oldText.isEmpty() || t.isEmpty()) {
        for (int i = 0; i < 2; ++i) {
            if (d->m_iconbutton[i]->hasAutoHide())
                d->m_iconbutton[i]->animateShow(!t.isEmpty());
        }
        d->m_oldText = t;
    }

    handleChanged(t);
}

QString FancyLineEdit::fixInputString(const QString &string)
{
    return string;
}

//
// IconButton - helper class to represent a clickable icon
//

IconButton::IconButton(QWidget *parent)
    : QAbstractButton(parent), m_autoHide(false)
{
    setCursor(Qt::ArrowCursor);
    setFocusPolicy(Qt::NoFocus);
}

void IconButton::paintEvent(QPaintEvent *)
{
    const qreal pixmapRatio = m_pixmap.devicePixelRatio();
    QStylePainter painter(this);
    QRect pixmapRect = QRect(0, 0, m_pixmap.width()/pixmapRatio, m_pixmap.height()/pixmapRatio);
    pixmapRect.moveCenter(rect().center());

    if (m_autoHide)
        painter.setOpacity(m_iconOpacity);

    painter.drawPixmap(pixmapRect, m_pixmap);

    if (hasFocus()) {
        QStyleOptionFocusRect focusOption;
        focusOption.initFrom(this);
        focusOption.rect = pixmapRect;
        if (HostOsInfo::isMacHost()) {
            focusOption.rect.adjust(-4, -4, 4, 4);
            painter.drawControl(QStyle::CE_FocusFrame, focusOption);
        } else {
            painter.drawPrimitive(QStyle::PE_FrameFocusRect, focusOption);
        }
    }
}

void IconButton::animateShow(bool visible)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "iconOpacity");
    animation->setDuration(FADE_TIME);
    animation->setEndValue(visible ? 1.0 : 0.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

QSize IconButton::sizeHint() const
{
    const qreal pixmapRatio = m_pixmap.devicePixelRatio();
    return QSize(m_pixmap.width()/pixmapRatio, m_pixmap.height()/pixmapRatio);
}

void IconButton::keyPressEvent(QKeyEvent *ke)
{
    QAbstractButton::keyPressEvent(ke);
    if (!ke->modifiers() && (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return))
        click();
    // do not forward to line edit
    ke->accept();
}

void IconButton::keyReleaseEvent(QKeyEvent *ke)
{
    QAbstractButton::keyReleaseEvent(ke);
    // do not forward to line edit
    ke->accept();
}

} // namespace Utils
