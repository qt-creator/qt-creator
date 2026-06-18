// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fancylineedit.h"

#include "camelcasecursor.h"
#include "execmenu.h"
#include "futuresynchronizer.h"
#include "fancyiconbutton.h"
#include "historycompleter.h"
#include "hostosinfo.h"
#include "icon.h"
#include "qtcassert.h"
#include "utilstr.h"

#include <solutions/spinner/spinner.h>

#include <QAbstractItemView>
#include <QApplication>
#include <qapplicationstatic.h>
#include <QCompleter>
#include <QFutureWatcher>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenu>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <QShortcut>
#include <QStyle>
#include <QStyleOptionFocusRect>
#include <QStylePainter>
#include <QTimer>
#include <QValidator>
#include <QWindow>

#include <optional>

/*!
    \class Utils::FancyLineEdit
    \inmodule QtCreator

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

    \li The ability to validate the contents of the text field by setting the \a validationFunction.
    \endlist

    When invalid, the text color will turn red and a tooltip will
    contain the error message. This approach is less intrusive than a
    QValidator which will prevent the user from entering certain characters.

    A visible hint text results validation to be in state 'DisplayingInitialText',
    which is not valid, but is not marked red.
 */

/*!
    \class Utils::ClassNameValidatingLineEdit
    \inmodule QtCreator

    \brief The ClassNameValidatingLineEdit class implements a line edit that
    validates a C++ class name and emits a signal
    to derive suggested file names from it.
*/

/*!
  \class Utils::FileNameValidatingLineEdit
  \inmodule QtCreator

  \brief The FileNameValidatingLineEdit class is a control that lets the user
  choose a (base) file name, based on a QLineEdit.

  The class has
   some validation logic for embedding into QWizardPage.
*/

enum { margin = 6 };

namespace Utils {

static bool camelCaseNavigation = false;

class CompletionShortcut : public QObject
{
    Q_OBJECT

public:
    void setKeySequence(const QKeySequence &key)
    {
        if (m_key != key) {
            m_key = key;
            emit keyChanged(key);
        }
    }
    QKeySequence key() const { return m_key; }

signals:
    void keyChanged(const QKeySequence &key);

private:
    QKeySequence m_key = Qt::Key_Space | HostOsInfo::controlModifier();
};
Q_APPLICATION_STATIC(CompletionShortcut, completionShortcut)

bool CompletingLineEdit::event(QEvent *e)
{
    // workaround for QTCREATORBUG-9453
    if (e->type() == QEvent::ShortcutOverride) {
        if (QCompleter *comp = completer()) {
            if (comp->popup() && comp->popup()->isVisible()) {
                auto ke = static_cast<QKeyEvent *>(e);
                if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
                    ke->accept();
                    return true;
                }
            }
        }
    }
    return QLineEdit::event(e);
}

void CompletingLineEdit::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Down && !(e->modifiers() & ~Qt::KeypadModifier)) {
        if (QCompleter *comp = completer()) {
            if (!comp->popup()->isVisible()) {
                comp->complete();
                return;
            }
        }
    }
    QLineEdit::keyPressEvent(e);
}

// --------- FancyLineEditPrivate
class FancyLineEditPrivate : public QObject
{
public:
    explicit FancyLineEditPrivate(FancyLineEdit *parent);
    ~FancyLineEditPrivate();

    bool eventFilter(QObject *obj, QEvent *event) override;

    FancyLineEdit *m_lineEdit;
    FancyIconButton *m_iconbutton[2];
    HistoryCompleter *m_historyCompleter = nullptr;
    QShortcut m_completionShortcut;
    FancyLineEdit::ValidationFunction m_validationFunction = &FancyLineEdit::validateWithValidator;
    QString m_oldText;
    QMenu *m_menu[2];
    FancyLineEdit::State m_state = FancyLineEdit::Invalid;
    bool m_menuTabFocusTrigger[2];
    bool m_iconEnabled[2];

    bool m_isFiltering = false;
    bool m_firstChange = true;
    bool m_toolTipSet = false;
    bool m_validatePlaceHolder = false;

    QString m_lastFilterText;

    const QColor m_okTextColor;
    const QColor m_errorTextColor;
    const QColor m_placeholderTextColor;
    QString m_errorMessage;

    SpinnerSolution::Spinner *m_spinner = nullptr;
    QTimer m_spinnerDelayTimer;

    std::unique_ptr<QFutureWatcher<FancyLineEdit::AsyncValidationResult>> m_validatorWatcher;
};

FancyLineEditPrivate::FancyLineEditPrivate(FancyLineEdit *parent)
    : QObject(parent)
    , m_lineEdit(parent)
    , m_completionShortcut(completionShortcut()->key(), parent)
    , m_okTextColor(creatorColor(Theme::TextColorNormal))
    , m_errorTextColor(creatorColor(Theme::TextColorError))
    , m_placeholderTextColor(QApplication::palette().color(QPalette::PlaceholderText))
    , m_spinner(new SpinnerSolution::Spinner(SpinnerSolution::SpinnerSize::Small, m_lineEdit))
{
    m_spinner->setVisible(false);
    m_spinnerDelayTimer.setInterval(200);
    m_spinnerDelayTimer.setSingleShot(true);
    connect(&m_spinnerDelayTimer, &QTimer::timeout, m_spinner, [spinner = m_spinner] {
        spinner->setVisible(true);
    });

    m_completionShortcut.setContext(Qt::WidgetShortcut);
    connect(completionShortcut(), &CompletionShortcut::keyChanged,
            &m_completionShortcut, &QShortcut::setKey);

    for (int i = 0; i < 2; ++i) {
        m_iconbutton[i] = new FancyIconButton(parent);
        m_iconbutton[i]->installEventFilter(this);
        m_iconbutton[i]->hide();
        m_iconbutton[i]->setAutoHide(false);

        m_menu[i] = nullptr;

        m_menuTabFocusTrigger[i] = false;
        m_iconEnabled[i] = false;
    }
}

FancyLineEditPrivate::~FancyLineEditPrivate()
{
    if (m_validatorWatcher)
        m_validatorWatcher->cancel();
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
    updateMargins();

    connect(d->m_iconbutton[Left], &QAbstractButton::clicked, this, [this] {
        iconClicked(Left);
    });
    connect(d->m_iconbutton[Right], &QAbstractButton::clicked, this, [this] {
        iconClicked(Right);
    });
    connect(this, &QLineEdit::textChanged, this, &FancyLineEdit::validate);
    connect(&d->m_completionShortcut, &QShortcut::activated, this, [this] {
        if (!completer())
            return;
        completer()->setCompletionPrefix(text().left(cursorPosition()));
        completer()->complete();
    });
}

FancyLineEdit::~FancyLineEdit()
{
    if (d->m_historyCompleter) {
        // When dialog with FancyLineEdit widget closed by <Escape>
        // the QueuedConnection don't have enough time to call slot callback
        // because edit widget and all of its connections are destroyed before
        // QCoreApplicationPrivate::sendPostedEvents dispatch our queued signal.
        if (!text().isEmpty())
            d->m_historyCompleter->addEntry(text());
    }
}

void FancyLineEdit::setTextKeepingActiveCursor(const QString &text)
{
    std::optional<int> cursor = hasFocus() ? std::make_optional(cursorPosition()) : std::nullopt;
    setText(text);
    if (cursor)
        setCursorPosition(*cursor);
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

QAbstractButton *FancyLineEdit::button(Side side) const
{
    return d->m_iconbutton[side];
}

void FancyLineEdit::iconClicked(Side side)
{
    if (d->m_menu[side]) {
        execMenuAtWidget(d->m_menu[side], button(side));
    } else {
        emit buttonClicked(side);
        if (side == Left)
            emit leftButtonClicked();
        else if (side == Right)
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
        Side iconpos = Side(i);
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

void FancyLineEdit::setButtonIcon(Side side, const QIcon &icon)
{
    d->m_iconbutton[side]->setIcon(icon);
    updateMargins();
    updateButtonPositions();
    update();
}

QIcon FancyLineEdit::buttonIcon(Side side) const
{
    return d->m_iconbutton[side]->icon();
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

void FancyLineEdit::setHistoryCompleter(
    const Key &historyKey, bool restoreLastItemFromHistory, int maxLines)
{
    QTC_ASSERT(!d->m_historyCompleter, return);
    d->m_historyCompleter = new HistoryCompleter(historyKey, maxLines, this);
    if (restoreLastItemFromHistory && d->m_historyCompleter->hasHistory())
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

void FancyLineEdit::keyPressEvent(QKeyEvent *event)
{
    if (camelCaseNavigation) {
        if (event == QKeySequence::MoveToPreviousWord)
            CamelCaseCursor::left(this, QTextCursor::MoveAnchor);
        else if (event == QKeySequence::SelectPreviousWord)
            CamelCaseCursor::left(this, QTextCursor::KeepAnchor);
        else if (event == QKeySequence::MoveToNextWord)
            CamelCaseCursor::right(this, QTextCursor::MoveAnchor);
        else if (event == QKeySequence::SelectNextWord)
            CamelCaseCursor::right(this, QTextCursor::KeepAnchor);
        else
            CompletingLineEdit::keyPressEvent(event);
    } else {
        CompletingLineEdit::keyPressEvent(event);
    }
}

void FancyLineEdit::setCamelCaseNavigationEnabled(bool enabled)
{
    camelCaseNavigation = enabled;
}

void FancyLineEdit::setCompletionShortcut(const QKeySequence &shortcut)
{
    completionShortcut()->setKeySequence(shortcut);
}

void FancyLineEdit::setValueAlternatives(const QStringList &values)
{
    for (const QString &value : values)
        d->m_historyCompleter->addEntry(value);
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
        // KDE has custom icons for this. The "ltr" and "rtl" suffixes describe the direction
        // into which the arrows are pointing. They do not describe which writing direction they
        // are intended to be used for.
        const QLatin1String pointingWest("edit-clear-locationbar-rtl");
        const QLatin1String pointingEast("edit-clear-locationbar-ltr");
        const QIcon icon = Icon::fromTheme(layoutDirection() == Qt::LeftToRight ? pointingWest
                                                                                : pointingEast);
        setButtonIcon(Right, icon);
        setButtonVisible(Right, true);
        setPlaceholderText(Tr::tr("Filter"));
        setButtonToolTip(Right, Tr::tr("Clear text"));
        setAutoHideButton(Right, true);
        connect(this, &FancyLineEdit::rightButtonClicked, this, &QLineEdit::clear);
    } else {
        disconnect(this, &FancyLineEdit::rightButtonClicked, this, &QLineEdit::clear);
    }
}

/*!
    Set a synchronous or asynchronous validation function \a fn.
    Asynchronous validation functions can continue to run after destruction of the
    FancyLineEdit instance. During shutdown asynchronous validation functions can continue
    to run until before the plugin instances are deleted (at that point the plugin manager
    waits for them to finish before continuing).

    \sa defaultValidationFunction()
 */
void FancyLineEdit::setValidationFunction(const FancyLineEdit::ValidationFunction &fn)
{
    d->m_validationFunction = fn;
    validate();
}

/*!
    Returns the default validation function, which synchonously executes the line edit's
    validator.

    \sa setValidationFunction()
*/
FancyLineEdit::ValidationFunction FancyLineEdit::defaultValidationFunction()
{
    return &FancyLineEdit::validateWithValidator;
}

Result<> FancyLineEdit::validateWithValidator(FancyLineEdit &edit)
{
    if (const QValidator *v = edit.validator()) {
        QString tmp = edit.text();
        int pos = edit.cursorPosition();
        if (v->validate(tmp, pos) != QValidator::Acceptable)
            return ResultError(QString());
    }
    return ResultOk;
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

void FancyLineEdit::setValidatePlaceHolder(bool on)
{
    d->m_validatePlaceHolder = on;
}

void FancyLineEdit::handleValidationResult(AsyncValidationResult result, const QString &oldText)
{
    d->m_spinner->setVisible(false);
    d->m_spinnerDelayTimer.stop();

    const QString newText = result ? *result : oldText;
    if (!result)
        d->m_errorMessage = result.error();
    else
        d->m_errorMessage.clear();

    // Are we displaying the placeholder text?
    const bool isDisplayingPlaceholderText = !placeholderText().isEmpty() && newText.isEmpty();
    const bool validates = result.has_value();
    const State newState = isDisplayingPlaceholderText ? DisplayingPlaceholderText
                                                       : (validates ? Valid : Invalid);
    if (!validates || d->m_toolTipSet) {
        setToolTip(d->m_errorMessage);
        d->m_toolTipSet = true;
    }
    // Changed..figure out if valid changed. Also trigger on the first change.
    // Invalid DisplayingPlaceholderText shows also error color.
    if (newState != d->m_state || d->m_firstChange) {
        const bool validHasChanged = (d->m_state == Valid) != (newState == Valid);
        d->m_state = newState;
        d->m_firstChange = false;

        QPalette p = palette();
        p.setColor(QPalette::Active,
                   QPalette::Text,
                   newState == Invalid ? d->m_errorTextColor : d->m_okTextColor);
        p.setColor(QPalette::Active,
                   QPalette::PlaceholderText,
                   validates || !d->m_validatePlaceHolder ? d->m_placeholderTextColor
                                                          : d->m_errorTextColor);
        setPalette(p);

        if (validHasChanged)
            emit validChanged(newState == Valid);
    }
    const QString fixedString = fixInputString(newText);
    if (newText != fixedString) {
        const int cursorPos = cursorPosition();
        QSignalBlocker blocker(this);
        setText(fixedString);
        setCursorPosition(qMin(cursorPos, fixedString.size()));
    }

    // Check buttons.
    if (d->m_oldText.isEmpty() || newText.isEmpty()) {
        for (auto &button : std::as_const(d->m_iconbutton)) {
            if (button->hasAutoHide())
                button->animateShow(!newText.isEmpty());
        }
        d->m_oldText = newText;
    }

    handleChanged(newText);
}

void FancyLineEdit::validate()
{
    if (d->m_validationFunction.index() == 0) {
        AsyncValidationFunction &validationFunction = std::get<0>(d->m_validationFunction);
        if (!validationFunction)
            return;

        if (d->m_validatorWatcher)
            d->m_validatorWatcher->cancel();

        const QString oldText = text();

        if (d->m_isFiltering) {
            if (oldText != d->m_lastFilterText) {
                d->m_lastFilterText = oldText;
                emit filterChanged(oldText);
            }
        }

        d->m_validatorWatcher = std::make_unique<QFutureWatcher<AsyncValidationResult>>();
        connect(d->m_validatorWatcher.get(),
                &QFutureWatcher<AsyncValidationResult>::finished,
                this,
                [this, oldText] {
                    FancyLineEdit::AsyncValidationResult result = d->m_validatorWatcher->result();

                    handleValidationResult(result, oldText);
                });

        d->m_spinnerDelayTimer.start();

        AsyncValidationFuture future = validationFunction(text());
        d->m_validatorWatcher->setFuture(future);
        Utils::futureSynchronizer()->addFuture(future);

        return;
    }

    if (d->m_validationFunction.index() == 1) {
        SynchronousValidationFunction &validationFunction = std::get<1>(d->m_validationFunction);
        if (!validationFunction)
            return;

        const QString t = text();

        if (d->m_isFiltering) {
            if (t != d->m_lastFilterText) {
                d->m_lastFilterText = t;
                emit filterChanged(t);
            }
        }

        Result<QString> result;

        if (const Result<> validates = validationFunction(*this))
            result = t;
        else
            result = ResultError(validates.error());

        handleValidationResult(result, t);
    }

    if (d->m_validationFunction.index() == 2) {
        SimpleSynchronousValidationFunction &validationFunction = std::get<2>(d->m_validationFunction);
        if (!validationFunction)
            return;

        const QString t = text();

        if (d->m_isFiltering) {
            if (t != d->m_lastFilterText) {
                d->m_lastFilterText = t;
                emit filterChanged(t);
            }
        }

        Result<QString> result;

        if (const Result<> validates = validationFunction(t))
            result = t;
        else
            result = ResultError(validates.error());

        handleValidationResult(result, t);
    }
}

QString FancyLineEdit::fixInputString(const QString &string)
{
    return string;
}

// Match something like "Namespace1::Namespace2::ClassName".

struct ClassNameValidatingLineEditPrivate
{
    QRegularExpression m_nameRegexp;
    QString m_namespaceDelimiter{"::"};
    bool m_namespacesEnabled = false;
    bool m_lowerCaseFileName = true;
    bool m_forceFirstCapitalLetter = false;
};

// --------------------- ClassNameValidatingLineEdit
ClassNameValidatingLineEdit::ClassNameValidatingLineEdit(QWidget *parent) :
    FancyLineEdit(parent),
    d(new ClassNameValidatingLineEditPrivate)
{
    setValidationFunction([this](const QString &text) { return validateClassName(text); });
    updateRegExp();
}

ClassNameValidatingLineEdit::~ClassNameValidatingLineEdit()
{
    delete d;
}

bool ClassNameValidatingLineEdit::namespacesEnabled() const
{
    return d->m_namespacesEnabled;
}

void ClassNameValidatingLineEdit::setNamespacesEnabled(bool b)
{
    d->m_namespacesEnabled = b;
}

/**
 * @return Language-specific namespace delimiter, e.g. '::' or '.'
 */
QString ClassNameValidatingLineEdit::namespaceDelimiter()
{
    return d->m_namespaceDelimiter;
}

/**
 * @brief Sets language-specific namespace delimiter, e.g. '::' or '.'
 * Do not use identifier characters in delimiter
 */
void ClassNameValidatingLineEdit::setNamespaceDelimiter(const QString &delimiter)
{
    d->m_namespaceDelimiter = delimiter;
}

Result<> ClassNameValidatingLineEdit::validateClassName(const QString &text) const
{
    QTC_ASSERT(d->m_nameRegexp.isValid(), return ResultError(ResultAssert));

    if (!d->m_namespacesEnabled && text.contains(d->m_namespaceDelimiter))
        return ResultError(Tr::tr("The class name must not contain namespace delimiters."));

    if (text.isEmpty())
        return ResultError(Tr::tr("Please enter a class name."));

    if (!d->m_nameRegexp.match(text).hasMatch())
        return ResultError(Tr::tr("The class name contains invalid characters."));

    return ResultOk;
}

void ClassNameValidatingLineEdit::handleChanged(const QString &t)
{
    if (isValid()) {
        // Suggest file names, strip namespaces
        QString fileName = d->m_lowerCaseFileName ? t.toLower() : t;
        if (d->m_namespacesEnabled) {
            const int namespaceIndex = fileName.lastIndexOf(d->m_namespaceDelimiter);
            if (namespaceIndex != -1)
                fileName.remove(0, namespaceIndex + d->m_namespaceDelimiter.size());
        }
        emit updateFileName(fileName);
    }
}

QString ClassNameValidatingLineEdit::fixInputString(const QString &string)
{
    if (!forceFirstCapitalLetter())
        return string;

    QString fixedString = string;
    if (!string.isEmpty() && string.at(0).isLower())
        fixedString[0] = string.at(0).toUpper();

    return fixedString;
}

void ClassNameValidatingLineEdit::updateRegExp() const
{
    const QString pattern = "^%1(%2%1)*$";
    d->m_nameRegexp.setPattern(pattern.arg(QString("[a-zA-Z_][a-zA-Z0-9_]*"),
                                           QRegularExpression::escape(d->m_namespaceDelimiter)));
}

QString ClassNameValidatingLineEdit::createClassName(const QString &name)
{
    // Remove spaces and convert the adjacent characters to uppercase
    QString className = name;
    static const QRegularExpression spaceMatcher(" +(\\w)");
    QTC_CHECK(spaceMatcher.isValid());
    while (true) {
        const QRegularExpressionMatch match = spaceMatcher.match(className);
        if (!match.hasMatch())
            break;
        className.replace(match.capturedStart(), match.capturedLength(), match.captured(1).toUpper());
    }

    // Filter out any remaining invalid characters
    static const QRegularExpression regexp("[^a-zA-Z0-9_]");
    className.remove(regexp);

    // If the first character is numeric, prefix the name with a "_"
    if (className.at(0).isNumber()) {
        className.prepend(QLatin1Char('_'));
    } else {
        // Convert the first character to uppercase
        className.replace(0, 1, className.left(1).toUpper());
    }

    return className;
}

bool ClassNameValidatingLineEdit::lowerCaseFileName() const
{
    return d->m_lowerCaseFileName;
}

void ClassNameValidatingLineEdit::setLowerCaseFileName(bool v)
{
    d->m_lowerCaseFileName = v;
}

bool ClassNameValidatingLineEdit::forceFirstCapitalLetter() const
{
    return d->m_forceFirstCapitalLetter;
}

void ClassNameValidatingLineEdit::setForceFirstCapitalLetter(bool b)
{
    d->m_forceFirstCapitalLetter = b;
}

#define WINDOWS_DEVICES_PATTERN "(CON|AUX|PRN|NUL|COM[1-9]|LPT[1-9])(\\..*)?"

// Naming a file like a device name will break on Windows, even if it is
// "com1.txt". Since we are cross-platform, we generally disallow such file
//  names.
static const QRegularExpression &windowsDeviceNoSubDirPattern()
{
    static const QRegularExpression rc(QString("^" WINDOWS_DEVICES_PATTERN "$"),
                                       QRegularExpression::CaseInsensitiveOption);
    QTC_ASSERT(rc.isValid(), return rc);
    return rc;
}

static const QRegularExpression &windowsDeviceSubDirPattern()
{
    static const QRegularExpression rc(QString("^.*[/\\\\]" WINDOWS_DEVICES_PATTERN "$"),
                                       QRegularExpression::CaseInsensitiveOption);
    QTC_ASSERT(rc.isValid(), return rc);
    return rc;
}

// ----------- FileNameValidatingLineEdit
FileNameValidatingLineEdit::FileNameValidatingLineEdit(QWidget *parent) :
    FancyLineEdit(parent),
    m_allowDirectories(false),
    m_forceFirstCapitalLetter(false)
{
    setValidationFunction([this](const QString &text) {
        if (const Result<> res = validateFileNameExtension(text, requiredExtensions()); !res)
            return res;
        if (const Result<> res = validateFileName(text, allowDirectories()); !res)
            return res;
        return ResultOk;
    });
}

bool FileNameValidatingLineEdit::allowDirectories() const
{
    return m_allowDirectories;
}

void FileNameValidatingLineEdit::setAllowDirectories(bool v)
{
    m_allowDirectories = v;
}

bool FileNameValidatingLineEdit::forceFirstCapitalLetter() const
{
    return m_forceFirstCapitalLetter;
}

void FileNameValidatingLineEdit::setForceFirstCapitalLetter(bool b)
{
    m_forceFirstCapitalLetter = b;
}

/* Validate a file base name, check for forbidden characters/strings. */

#define SLASHES "/\\"

Result<> FileNameValidatingLineEdit::validateFileName(const QString &name, bool allowDirectories)
{
    static const char notAllowedCharsSubDir[]   = ",^@={}[]~!?:&*\"|#%<>$\"'();`' ";
    static const char notAllowedCharsNoSubDir[] = ",^@={}[]~!?:&*\"|#%<>$\"'();`' " SLASHES;

    static const char *notAllowedSubStrings[] = {".."};

    if (name.isEmpty())
        return ResultError(Tr::tr("Name is empty."));

    // Characters
    const char *notAllowedChars = allowDirectories ? notAllowedCharsSubDir : notAllowedCharsNoSubDir;
    for (const char *c = notAllowedChars; *c; c++) {
        if (name.contains(QLatin1Char(*c))) {
            const QChar qc = QLatin1Char(*c);
            if (qc.isSpace())
                return ResultError(Tr::tr("Name contains white space."));
            return ResultError(Tr::tr("Invalid character \"%1\".").arg(qc));
        }
    }

    // Substrings
    for (const char * const s : notAllowedSubStrings) {
        const QLatin1String notAllowedSubString(s);
        if (name.contains(notAllowedSubString))
            return ResultError(Tr::tr("Invalid characters \"%1\".").arg(QString(notAllowedSubString)));
    }

    // Windows devices
    bool matchesWinDevice = name.contains(windowsDeviceNoSubDirPattern());
    if (!matchesWinDevice && allowDirectories)
        matchesWinDevice = name.contains(windowsDeviceSubDirPattern());
    if (matchesWinDevice) {
        return ResultError(Tr::tr("Name matches MS Windows device"
                                  " (CON, AUX, PRN, NUL,"
                                  " COM1, COM2, ..., COM9,"
                                  " LPT1, LPT2, ..., LPT9)"));
    }
    return ResultOk;
}

QString FileNameValidatingLineEdit::fixInputString(const QString &string)
{
    if (!forceFirstCapitalLetter())
        return string;

    QString fixedString = string;
    if (!string.isEmpty() && string.at(0).isLower())
        fixedString[0] = string.at(0).toUpper();

    return fixedString;
}

Result<> FileNameValidatingLineEdit::validateFileNameExtension(const QString &fileName,
                                                               const QStringList &requiredExtensions)
{
    // file extension
    if (requiredExtensions.isEmpty())
        return ResultOk;

    for (const QString &requiredExtension : requiredExtensions) {
        QString extension = QLatin1Char('.') + requiredExtension;
        if (fileName.endsWith(extension, Qt::CaseSensitive) && extension.size() < fileName.size())
            return ResultOk;
    }

    if (requiredExtensions.size() == 1)
        return ResultError(Tr::tr("File extension %1 is required:").arg(requiredExtensions.first()));

    return ResultError(Tr::tr("File extensions %1 are required:").arg(requiredExtensions.join(", ")));
}

QStringList FileNameValidatingLineEdit::requiredExtensions() const
{
    return m_requiredExtensionList;
}

void FileNameValidatingLineEdit::setRequiredExtensions(const QStringList &extensions)
{
    m_requiredExtensionList = extensions;
}

} // namespace Utils

#include <fancylineedit.moc>
