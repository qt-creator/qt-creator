// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "widgets.h"

#include "algorithm.h"
#include "filepath.h"
#include "guiutils.h"
#include "hostosinfo.h"
#include "icon.h"
#include "layoutbuilder.h"
#include "qtcassert.h"
#include "qtcsettings.h"
#include "qtdesignwidgets.h"
#include "stylehelper.h"
#include "tooltip/tooltip.h"
#include "utilsicons.h"
#include "utilstr.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QCompleter>
#include <QDataStream>
#include <QContextMenuEvent>
#include <QDialogButtonBox>
#include <qdrawutil.h>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHideEvent>
#include <QKeyEvent>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QPointer>
#include <QProgressDialog>
#include <QPropertyAnimation>
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QStyledItemDelegate>
#include <QStylePainter>
#include <QTimer>
#include <QVBoxLayout>

#include <functional>
#include <utility>

#if QT_CONFIG(graphicseffect)
#include <QGraphicsOpacityEffect>
#endif

/*!
    \class Utils::DocumentTabBar
    \inheaderfile utils/widgets.h
    \inmodule QtCreator

    \brief The DocumentTabBar class adds handling of the middle mouse button
    for closing tabs to QTabBar.
*/

/*!
    \class Utils::FakeToolTip
    \inmodule QtCreator

    \brief The FakeToolTip class is a widget that pretends to be a tooltip.

    By default it has Qt::WA_DeleteOnClose set.
*/

/*!
    \class Utils::HeaderViewStretcher
    \inmodule QtCreator

    \brief The HeaderViewStretcher class fixes QHeaderView to resize all
    columns to contents, except one
    stretching column.

    As opposed to standard QTreeWidget, all columns are
    still interactively resizable.
*/

/*!
    \class Utils::OptionPushButton
    \inmodule QtCreator

    \brief The OptionPushButton class implements a QPushButton for which the menu is only opened
    if the user presses the menu indicator.

    Use OptionPushButton::setOptionalMenu() to set the menu and its actions.
    If the users clicks on the menu indicator of the push button, this menu is opened, and
    its actions are triggered when the user selects them.

    If the user clicks anywhere else on the button, the QAbstractButton::clicked() signal is
    sent, as if the button didn't have a menu.

    Note: You may not call QPushButton::setMenu(). Use OptionPushButton::setOptionalMenu() instead.
*/

/*!
    \class Utils::StatusLabel
    \inmodule QtCreator

    \brief The StatusLabel class displays messages for a while with a timeout.
*/

/*!
    \class Utils::TextFieldCheckBox
    \inmodule QtCreator
    \brief The TextFieldCheckBox class is a aheckbox that plays with
    \c QWizard::registerField.

    Provides a settable 'text' property containing predefined strings for
    \c true and \c false.
*/

/*!
    \class Utils::TextFieldComboBox
    \inmodule QtCreator
    \brief The TextFieldComboBox class is a non-editable combo box for text
    editing purposes that plays with \c QWizard::registerField (providing a
    settable 'text' property).

    Allows for a separation of values to be used for wizard fields replacement
    and display texts.
*/

namespace Utils {

#if QT_CONFIG(graphicseffect)

FadingWidget::FadingWidget(QWidget *parent) :
    FadingPanel(parent),
    m_opacityEffect(new QGraphicsOpacityEffect)
{
    m_opacityEffect->setOpacity(0);
    setGraphicsEffect(m_opacityEffect);

    // Workaround for issue with QGraphicsEffect. GraphicsEffect
    // currently clears with Window color. Remove if flickering
    // no longer occurs on fade-in
    QPalette pal;
    pal.setBrush(QPalette::All, QPalette::Window, Qt::transparent);
    setPalette(pal);
}

void FadingWidget::setOpacity(qreal value)
{
    m_opacityEffect->setOpacity(value);
}

void FadingWidget::fadeTo(qreal value)
{
    QPropertyAnimation *animation = new QPropertyAnimation(m_opacityEffect, "opacity");
    animation->setDuration(200);
    animation->setEndValue(value);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

qreal FadingWidget::opacity()
{
    return m_opacityEffect->opacity();
}

#else

// Without graphics effects there is no opacity to fade: the widget is shown
// as soon as it is not fully transparent, and keeps its space while hidden.
FadingWidget::FadingWidget(QWidget *parent) :
    FadingPanel(parent)
{
    QSizePolicy policy = sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    setSizePolicy(policy);
    setVisible(false);
}

void FadingWidget::setOpacity(qreal value)
{
    m_opacity = value;
    setVisible(value > 0);
}

void FadingWidget::fadeTo(qreal value)
{
    setOpacity(value);
}

qreal FadingWidget::opacity()
{
    return m_opacity;
}

#endif // QT_CONFIG(graphicseffect)

ExpandButton::ExpandButton(QWidget *parent)
    : QToolButton(parent)
{
    setCheckable(true);
    auto updateArrow = [this] (bool checked) {
        static const QIcon expand =
            Icon({{":/utils/images/arrowdown.png", Theme::PanelTextColorDark}}, Icon::Tint).icon();
        static const QIcon collapse =
            Icon({{":/utils/images/arrowup.png", Theme::PanelTextColorDark}}, Icon::Tint).icon();
        setIcon(checked ? collapse : expand);
    };
    updateArrow(false);
    connect(this, &QToolButton::toggled, this, updateArrow);
}

DetailsButton::DetailsButton(QWidget *parent)
    : ExpandButton(parent)
{
    setText(Tr::tr("Details"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    if (HostOsInfo::isMacHost())
        setFont(QGuiApplication::font());
}

QSize DetailsButton::sizeHint() const
{
    const QSize textSize = fontMetrics().size(Qt::TextSingleLine | Qt::TextShowMnemonic, text());
    return QSize(spacing + textSize.width() + spacing + 16 + spacing,
                 spacing + fontMetrics().height() + spacing);
}

QColor DetailsButton::outlineColor()
{
    return HostOsInfo::isMacHost()
    ? QGuiApplication::palette().color(QPalette::Mid)
    : StyleHelper::mergedColors(creatorColor(Theme::TextColorNormal),
                                creatorColor(Theme::BackgroundColorNormal), 15);
}

void DetailsButton::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)

    QPainter p(this);
    if (isEnabled() && (isChecked() || (!HostOsInfo::isMacHost() && underMouse()))) {
        p.save();
        p.setOpacity(0.125);
        p.fillRect(rect(), palette().color(QPalette::Text));
        p.restore();
    }

    if (!creatorTheme()->flag(Theme::FlatProjectsMode))
        qDrawPlainRect(&p, rect(), outlineColor());

    const QRect textRect(spacing + 3, 0, width(), height());
    p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, text());
    if (creatorTheme()->flag(Theme::FlatProjectsMode) || HostOsInfo::isMacHost()) {
        const QRect iconRect(width() - spacing - 15, 0, 16, height());
        icon().paint(&p, iconRect);
    } else {
        int arrowsize = 15;
        QStyleOption arrowOpt;
        arrowOpt.initFrom(this);
        arrowOpt.rect = QRect(size().width() - arrowsize - spacing, height() / 2 - arrowsize / 2,
                              arrowsize, arrowsize);
        style()->drawPrimitive(isChecked() ? QStyle::PE_IndicatorArrowUp
                                           : QStyle::PE_IndicatorArrowDown, &arrowOpt, &p, this);
    }
}

void DocumentTabBar::mousePressEvent(QMouseEvent *event)
{
    if (tabsClosable() && event->button() == Qt::MiddleButton)
        return; // do not switch tabs when closing tab is requested
    return QTabBar::mousePressEvent(event);
}

void DocumentTabBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (tabsClosable() && event->button() == Qt::MiddleButton) {
        int tabIndex = tabAt(event->pos());
        if (tabIndex != -1) {
            emit tabCloseRequested(tabIndex);
            return;
        }
    }
    QTabBar::mouseReleaseEvent(event);
}

FakeToolTip::FakeToolTip(QWidget *parent) :
    QWidget(parent, Qt::ToolTip | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus)
{
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_DeleteOnClose);

    // Set the window and button text to the tooltip text color, since this
    // widget draws the background as a tooltip.
    QPalette p = palette();
    const QColor toolTipTextColor = p.color(QPalette::Inactive, QPalette::ToolTipText);
    p.setColor(QPalette::Inactive, QPalette::WindowText, toolTipTextColor);
    p.setColor(QPalette::Inactive, QPalette::ButtonText, toolTipTextColor);
    setPalette(p);

    const int margin = 1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, this);
    setContentsMargins(margin + 1, margin, margin, margin);
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, nullptr, this) / 255.0);
}

void FakeToolTip::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.initFrom(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();
}

void FakeToolTip::resizeEvent(QResizeEvent *)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.initFrom(this);
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);
}

FileCrumbLabel::FileCrumbLabel(QWidget *parent)
    : QLabel(parent)
{
    setTextFormat(Qt::RichText);
    setWordWrap(true);
    connect(this, &QLabel::linkActivated, this, [this](const QString &filePath) {
        emit pathClicked(FilePath::fromString(filePath));
    });
    setPath(FilePath());
}

static QString linkForPath(const FilePath &path, const QString &display)
{
    return "<a href=\"" + path.toFSPathString() + "\">" + display + "</a>";
}

void FileCrumbLabel::setPath(const FilePath &path)
{
    QStringList links;
    for (const FilePath &current : PathAndParents(path)) {
        const QString fileName = current.fileName();
        if (!fileName.isEmpty()) {
            links.prepend(linkForPath(current, fileName));
        } else if (HostOsInfo::isWindowsHost() && current.isRootPath()) {
            // Only on Windows add the drive letter, without the '/' at the end
            QString display = current.toUrlishString();
            if (display.endsWith('/'))
                display.chop(1);
            links.prepend(linkForPath(current, display));
        }
    }
    const auto pathSeparator = HostOsInfo::isWindowsHost() ? QLatin1String("&nbsp;\\ ")
                                                           : QLatin1String("&nbsp;/ ");
    const QString prefix = HostOsInfo::isWindowsHost() ? QString("\\ ") : QString("/ ");
    setText(prefix + links.join(pathSeparator));
}

HeaderViewStretcher::HeaderViewStretcher(QHeaderView *headerView, int columnToStretch)
    : QObject(headerView), m_columnToStretch(columnToStretch)
{
    headerView->installEventFilter(this);
    stretch();
}

void HeaderViewStretcher::stretch()
{
    QHideEvent fake;
    HeaderViewStretcher::eventFilter(parent(), &fake);
}

void HeaderViewStretcher::softStretch()
{
    const auto hv = qobject_cast<QHeaderView*>(parent());
    for (int i = 0; i < hv->count(); ++i)
        hv->resizeSections(QHeaderView::ResizeToContents);
}

bool HeaderViewStretcher::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == parent()) {
        if (ev->type() == QEvent::Show) {
            auto hv = qobject_cast<QHeaderView*>(obj);
            for (int i = 0; i < hv->count(); ++i)
                hv->setSectionResizeMode(i, QHeaderView::Interactive);
        } else if (ev->type() == QEvent::Hide) {
            auto hv = qobject_cast<QHeaderView*>(obj);
            for (int i = 0; i < hv->count(); ++i)
                hv->setSectionResizeMode(i, i == m_columnToStretch
                                                ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
        } else if (ev->type() == QEvent::Resize) {
            auto hv = qobject_cast<QHeaderView*>(obj);
            if (hv->sectionResizeMode(m_columnToStretch) == QHeaderView::Interactive) {
                auto re = static_cast<QResizeEvent*>(ev);
                int diff = re->size().width() - re->oldSize().width() ;
                hv->resizeSection(m_columnToStretch, qMax(32, hv->sectionSize(m_columnToStretch) + diff));
            }
        }
    }
    return false;
}

/*!
    Constructs an option push button with parent \a parent.
*/
OptionPushButton::OptionPushButton(QWidget *parent)
    : QPushButton(parent)
{}

/*!
    Constructs an option push button with text \a text and parent \a parent.
*/
OptionPushButton::OptionPushButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{}

/*!
    Associates the popup menu \a menu with this push button.
    This menu is shown if the user clicks on the menu indicator that is shown.
    If the user clicks anywhere else on the button, QAbstractButton::clicked() is sent instead.

    \note Calling this method removes all connections to the QAbstractButton::pressed() signal.

    Ownership of the menu is not transferred to the push button.
*/
void OptionPushButton::setOptionalMenu(QMenu *menu)
{
    setMenu(menu);
    // Hack away that QPushButton opens the menu on "pressed".
    // Also removes all other connections to "pressed".
    disconnect(this, &QPushButton::pressed, 0, 0);
}

/*!
    \internal
*/
void OptionPushButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (!menu() || event->button() != Qt::LeftButton)
        return QPushButton::mouseReleaseEvent(event);
    setDown(false);
    const QRect r = rect();
    if (!r.contains(event->position().toPoint()))
        return;
    QStyleOptionButton option;
    initStyleOption(&option);
    // for Mac style
    const QRect contentRect = style()->subElementRect(QStyle::SE_PushButtonContents, &option, this);
    // for Common style
    const int menuButtonSize = style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &option, this);
    // see: newBtn.rect = QRect(ir.right() - mbi - 2, ir.height()/2 - mbi/2 + 3, mbi - 6, mbi - 6);
    // in QCommonStyle::drawControl, CE_PushButtonBevel, QStyleOptionButton::HasMenu
    static const int magicBorder = 4;
    const int clickX = event->position().toPoint().x();
    if ((option.direction == Qt::LeftToRight
         && clickX <= std::min(contentRect.right(), r.right() - menuButtonSize - magicBorder))
        || (option.direction == Qt::RightToLeft
            && clickX >= std::max(contentRect.left(), r.left() + menuButtonSize + magicBorder))) {
        click();
    } else {
        showMenu();
    }
}

OverrideCursor::OverrideCursor(const QCursor &cursor)
    : m_cursor(cursor)
{
    QApplication::setOverrideCursor(cursor);
}

OverrideCursor::~OverrideCursor()
{
    if (m_set)
        QApplication::restoreOverrideCursor();
}

void OverrideCursor::set()
{
    if (!m_set) {
        QApplication::setOverrideCursor(m_cursor);
        m_set = true;
    }
}

void OverrideCursor::reset()
{
    if (m_set) {
        QApplication::restoreOverrideCursor();
        m_set = false;
    }
}

RemoveFileDialog::RemoveFileDialog(const FilePath &filePath)
    : QDialog(dialogParent())
{
    const bool isFile = filePath.isFile();
    setWindowTitle(isFile ? Tr::tr("Remove File") : Tr::tr("Remove Folder"));
    resize(514, 159);

    QFont font;
    font.setFamilies({"Courier New"});

    auto fileNameLabel = new QLabel(filePath.toUserOutput());
    fileNameLabel->setFont(font);
    fileNameLabel->setWordWrap(true);

    m_deleteFileCheckBox = new QCheckBox(Tr::tr("&Delete file permanently"));

    auto removeVCCheckBox = new QCheckBox(Tr::tr("&Remove from version control"));
    removeVCCheckBox->setVisible(false); // TODO

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;

    Column {
        isFile ? Tr::tr("File to remove:") : Tr::tr("Folder to remove:"),
        fileNameLabel,
        Space(10),
        m_deleteFileCheckBox,
        removeVCCheckBox,
        buttonBox
    }.attachTo(this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void RemoveFileDialog::setDeleteFileVisible(bool visible)
{
    m_deleteFileCheckBox->setVisible(visible);
}

bool RemoveFileDialog::isDeleteFileChecked() const
{
    return m_deleteFileCheckBox->isChecked();
}

StatusLabel::StatusLabel(QWidget *parent) : QLabel(parent)
{
    // A manual size let's us shrink below minimum text width which is what
    // we want in [fake] status bars.
    setMinimumSize(QSize(30, 10));
}

void StatusLabel::stopTimer()
{
    if (m_timer && m_timer->isActive())
        m_timer->stop();
}

void StatusLabel::showStatusMessage(const QString &message, int timeoutMS)
{
    setText(message.split('\n').constFirst());
    setToolTip(message);
    if (timeoutMS > 0) {
        if (!m_timer) {
            m_timer = new QTimer(this);
            m_timer->setSingleShot(true);
            connect(m_timer, &QTimer::timeout, this, &StatusLabel::slotTimeout);
        }
        m_timer->start(timeoutMS);
    } else {
        m_lastPermanentStatusMessage = message;
        stopTimer();
    }
}

void StatusLabel::slotTimeout()
{
    setText(m_lastPermanentStatusMessage.split('\n').constFirst());
    setToolTip(m_lastPermanentStatusMessage);
}

void StatusLabel::clearStatusMessage()
{
    stopTimer();
    m_lastPermanentStatusMessage.clear();
    clear();
    setToolTip({});
}

namespace Internal {

const char USER_SEPARATOR_PREFIX[] = "StyledBar.UserSeparator";
const char USER_SPACER_PREFIX[] = "StyledBar.UserSpacer";
const char USER_STRETCH_PREFIX[] = "StyledBar.UserStretch";

static bool isUserSeparator(const Key &id)
{
    return id.view().startsWith(USER_SEPARATOR_PREFIX);
}

static bool isUserSpacer(const Key &id)
{
    return id.view().startsWith(USER_SPACER_PREFIX);
}

static bool isUserStretch(const Key &id)
{
    return id.view().startsWith(USER_STRETCH_PREFIX);
}

static bool isUserItem(const Key &id)
{
    return isUserSeparator(id) || isUserSpacer(id) || isUserStretch(id);
}

static QIcon separatorIcon()
{
    return Icons::SLASH.icon();
}

static QIcon spacerIcon()
{
    return Icons::BOUNDING_RECT.icon();
}

static QIcon stretchIcon()
{
    return Icons::FITTOVIEW_TOOLBAR.icon();
}

class StyledBarItem final
{
public:
    Key id;
    QString displayName;
    QPointer<QWidget> widget;
    bool visible = true;
    QIcon icon;
    int stretch = 0;
};

static void paintToolBarBackground(QWidget *widget)
{
    QPainter painter(widget);
    QStyleOptionToolBar option;
    option.rect = widget->rect();
    option.state = QStyle::State_Horizontal;
    widget->style()->drawControl(QStyle::CE_ToolBar, &option, &painter, widget);
}

class StyledBarOverflowPopup final : public QWidget
{
public:
    StyledBarOverflowPopup(QWidget *parent, const std::function<void()> &onHide)
        : QWidget(parent, Qt::Popup)
        , m_onHide(onHide)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        StyleHelper::setPanelWidget(this);
        auto popupLayout = new QVBoxLayout(this);
        popupLayout->setContentsMargins(0, 0, 0, 0);
        popupLayout->setSpacing(0);
    }

    void disarm() { m_onHide = {}; }

protected:
    void paintEvent(QPaintEvent *) override { paintToolBarBackground(this); }

    void hideEvent(QHideEvent *event) override
    {
        QWidget::hideEvent(event);
        if (const std::function<void()> onHide = std::exchange(m_onHide, {}))
            onHide();
    }

private:
    std::function<void()> m_onHide;
};

class StyledBarPrivate final
{
public:
    explicit StyledBarPrivate(StyledBar *q) : q(q) {}

    bool isKnown(const Key &id) const { return Utils::contains(items, id, &StyledBarItem::id); }
    StyledBarItem item(const Key &id) const
    {
        return Utils::findOrDefault(items, Utils::equal(&StyledBarItem::id, id));
    }
    bool isPermanent(const Key &id) const
    {
        return permanentItems.contains(id) || stretchItems.contains(id) || isUserItem(id);
    }
    KeyList defaultOrder() const
    {
        return Utils::filtered(Utils::transform(items, &StyledBarItem::id),
                               [](const Key &id) { return !isUserItem(id); });
    }
    bool addItem(const Key &id, const QString &displayName, QWidget *widget, const QIcon &icon);
    KeyList mergedOrder(const KeyList &ids) const;
    void syncUserItems(const KeyList &ids);
    void setConfiguration(const KeyList &newOrder, const KeyList &newHiddenItems);
    void apply();
    void load();
    void save() const;

    bool isShown(const Key &id) const
    {
        const StyledBarItem barItem = item(id);
        return barItem.widget && barItem.visible
               && (isPermanent(id) || !hiddenItems.contains(id));
    }
    static int itemWidth(const QWidget *widget)
    {
        return std::max({widget->sizeHint().width(), widget->minimumWidth(), 0});
    }
    bool staysInBar(const Key &id) const
    {
        return permanentItems.contains(id) || stretchItems.contains(id);
    }
    int fullWidth() const;
    int minimumWidth() const;
    void updateOverflow();
    void showOverflow();

    StyledBar *q = nullptr;
    QHBoxLayout *layout = nullptr;
    QToolButton *extensionButton = nullptr;
    QPointer<StyledBarOverflowPopup> overflowPopup;
    QList<StyledBarItem> items;
    KeyList order;
    KeyList hiddenItems;
    KeyList permanentItems;
    KeyList stretchItems;
    KeyList layoutItems;
    KeyList overflowItems;
    Key settingsKey;
};

int StyledBarPrivate::fullWidth() const
{
    int width = 0;
    for (const Key &id : order) {
        if (isShown(id))
            width += itemWidth(item(id).widget);
    }
    return width;
}

int StyledBarPrivate::minimumWidth() const
{
    int width = 0;
    bool canOverflow = false;
    for (const Key &id : order) {
        if (!isShown(id))
            continue;
        if (staysInBar(id))
            width += itemWidth(item(id).widget);
        else
            canOverflow = true;
    }
    if (canOverflow)
        width += extensionButton->sizeHint().width();
    return width;
}

void StyledBarPrivate::updateOverflow()
{
    if (!layout || overflowPopup)
        return;
    const KeyList shown = Utils::filtered(order, [this](const Key &id) { return isShown(id); });
    const int extensionWidth = extensionButton->sizeHint().width();
    KeyList overflowing;
    if (fullWidth() > q->width()) {
        int remaining = q->width() - extensionWidth;
        for (const Key &id : shown) {
            if (staysInBar(id))
                remaining -= itemWidth(item(id).widget);
        }
        bool full = false;
        for (const Key &id : shown) {
            if (staysInBar(id))
                continue;
            const int width = itemWidth(item(id).widget);
            if (!full && width <= remaining) {
                remaining -= width;
            } else {
                full = true;
                overflowing.append(id);
            }
        }
    }
    overflowItems = overflowing;
    for (const StyledBarItem &barItem : std::as_const(items)) {
        if (!barItem.widget)
            continue;
        barItem.widget->setVisible(isShown(barItem.id) && !overflowing.contains(barItem.id));
    }
    const bool overflows = !overflowing.isEmpty();
    layout->setContentsMargins(0, 0, overflows ? extensionWidth : 0, 0);
    extensionButton->setVisible(overflows);
    if (overflows) {
        extensionButton->setGeometry(q->width() - extensionWidth, 0, extensionWidth, q->height());
        extensionButton->raise();
    }
}

void StyledBarPrivate::showOverflow()
{
    if (overflowItems.isEmpty() || overflowPopup)
        return;
    auto popup = new StyledBarOverflowPopup(q, [this] { apply(); });
    for (const Key &id : std::as_const(overflowItems)) {
        QWidget *widget = item(id).widget;
        if (layoutItems.contains(id) || !widget)
            continue;
        layout->removeWidget(widget);
        popup->layout()->addWidget(widget);
        widget->show();
    }
    overflowPopup = popup;
    popup->adjustSize();
    popup->move(extensionButton->mapToGlobal(
        QPoint(extensionButton->width() - popup->width(), extensionButton->height())));
    popup->show();
}

KeyList StyledBarPrivate::mergedOrder(const KeyList &ids) const
{
    KeyList result;
    for (const Key &id : ids) {
        if (isKnown(id) && !result.contains(id))
            result.append(id);
    }
    int insertPosition = 0;
    for (const Key &id : defaultOrder()) {
        const int index = result.indexOf(id);
        if (index >= 0)
            insertPosition = index + 1;
        else
            result.insert(insertPosition++, id);
    }
    return result;
}

void StyledBarPrivate::syncUserItems(const KeyList &ids)
{
    for (int i = items.size() - 1; i >= 0; --i) {
        const Key id = items.at(i).id;
        if (!isUserItem(id) || ids.contains(id))
            continue;
        delete items.at(i).widget;
        items.removeAt(i);
        stretchItems.removeAll(id);
        layoutItems.removeAll(id);
    }
    for (const Key &id : ids) {
        if (isKnown(id))
            continue;
        if (isUserSeparator(id))
            q->addSeparator(id);
        else if (isUserSpacer(id))
            q->addSpacer(id);
        else if (isUserStretch(id))
            q->addStretch(id);
    }
}

void StyledBarPrivate::setConfiguration(const KeyList &newOrder, const KeyList &newHiddenItems)
{
    syncUserItems(newOrder);
    order = mergedOrder(newOrder);
    hiddenItems = Utils::filtered(newHiddenItems, [this](const Key &id) {
        return isKnown(id) && !isPermanent(id);
    });
    apply();
}

void StyledBarPrivate::apply()
{
    if (!layout)
        return;
    if (StyledBarOverflowPopup *popup = overflowPopup) {
        overflowPopup.clear();
        popup->close();
    }
    for (const StyledBarItem &barItem : std::as_const(items)) {
        if (barItem.widget)
            layout->removeWidget(barItem.widget);
    }
    for (const Key &id : std::as_const(order)) {
        const StyledBarItem barItem = item(id);
        if (barItem.widget)
            layout->addWidget(barItem.widget, barItem.stretch);
    }
    updateOverflow();
    q->updateGeometry();
}

void StyledBarPrivate::load()
{
    if (settingsKey.isEmpty())
        return;
    QtcSettings &settings = userSettings();
    setConfiguration(keysFromStrings(settings.value(settingsKey + "/Order").toStringList()),
                     keysFromStrings(settings.value(settingsKey + "/Hidden").toStringList()));
}

void StyledBarPrivate::save() const
{
    if (settingsKey.isEmpty())
        return;
    QtcSettings &settings = userSettings();
    settings.setValueWithDefault(settingsKey + "/Order",
                                 stringsFromKeys(order),
                                 stringsFromKeys(defaultOrder()));
    settings.setValueWithDefault(settingsKey + "/Hidden", stringsFromKeys(hiddenItems));
}

static bool isUserItemTemplate(const Key &id)
{
    return id == Key(USER_SEPARATOR_PREFIX) || id == Key(USER_SPACER_PREFIX)
           || id == Key(USER_STRETCH_PREFIX);
}

static Key itemId(const QListWidgetItem *listItem)
{
    return keyFromString(listItem->data(Qt::UserRole).toString());
}

class StyledBarItemList : public QListWidget
{
protected:
    void startDrag(Qt::DropActions supportedActions) override
    {
        Q_UNUSED(supportedActions)
        const QList<QListWidgetItem *> selection = selectedItems();
        const bool dragsTemplate = Utils::anyOf(selection, [](const QListWidgetItem *listItem) {
            return isUserItemTemplate(itemId(listItem));
        });
        QListWidget::startDrag(dragsTemplate ? Qt::CopyAction : Qt::MoveAction);
    }

    void dragEnterEvent(QDragEnterEvent *event) override
    {
        if (acceptsDrop(event))
            QListWidget::dragEnterEvent(event);
        else
            event->ignore();
    }

    void dragMoveEvent(QDragMoveEvent *event) override
    {
        if (acceptsDrop(event))
            QListWidget::dragMoveEvent(event);
        else
            event->ignore();
    }

    void dropEvent(QDropEvent *event) override
    {
        if (acceptsDrop(event))
            QListWidget::dropEvent(event);
        else
            event->ignore();
    }

    virtual bool acceptsItem(const Key &id, Qt::DropAction action) const
    {
        return action == Qt::MoveAction || isUserItemTemplate(id);
    }

private:
    bool acceptsDrop(const QDropEvent *event) const
    {
        const QMimeData *mimeData = event->mimeData();
        const QStringList types = mimeTypes();
        if (types.isEmpty() || !mimeData || !mimeData->hasFormat(types.first()))
            return false;
        QByteArray encoded = mimeData->data(types.first());
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        while (!stream.atEnd()) {
            int row = 0;
            int column = 0;
            QMap<int, QVariant> roles;
            stream >> row >> column >> roles;
            const Key id = keyFromString(roles.value(Qt::UserRole).toString());
            if (!acceptsItem(id, event->dropAction()))
                return false;
        }
        return true;
    }
};

class StyledBarAvailableList final : public StyledBarItemList
{
public:
    explicit StyledBarAvailableList(const StyledBarPrivate *d)
        : m_d(d)
    {}

protected:
    bool acceptsItem(const Key &id, Qt::DropAction action) const override
    {
        return StyledBarItemList::acceptsItem(id, action) && !m_d->isPermanent(id);
    }

private:
    const StyledBarPrivate *m_d = nullptr;
};

class StyledBarItemDelegate final : public QStyledItemDelegate
{
public:
    enum TrailingText { Position, ReusableTag };

    StyledBarItemDelegate(TrailingText trailingText, QObject *parent)
        : QStyledItemDelegate(parent)
        , m_trailingText(trailingText)
    {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        using namespace StyleHelper::SpacingTokens;
        const bool selected = option.state.testFlag(QStyle::State_Selected);
        const bool hovered = option.state.testFlag(QStyle::State_MouseOver);
        if (selected)
            StyleHelper::drawPanelBgRect(painter, option.rect, creatorColor(Theme::Token_Accent_Subtle));
        else if (hovered)
            StyleHelper::drawPanelBgRect(painter, option.rect, creatorColor(Theme::Token_Foreground_Subtle));

        QRect rect = option.rect.adjusted(PaddingHM, 0, -PaddingHM, 0);
        const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        const QRect iconRect(rect.left(), rect.center().y() - iconSize / 2, iconSize, iconSize);
        if (!icon.isNull())
            icon.paint(painter, iconRect);
        rect.setLeft(iconRect.right() + 1 + GapHM);

        const QString trailing = trailingText(index);
        if (!trailing.isEmpty()) {
            const QFont font = StyleHelper::uiFont(StyleHelper::UiElementCaption);
            const int width = QFontMetrics(font).horizontalAdvance(trailing);
            painter->setFont(font);
            painter->setPen(creatorColor(selected ? Theme::Token_Text_On_Accent
                                                  : Theme::Token_Text_Muted));
            painter->drawText(rect, Qt::AlignRight | Qt::AlignVCenter, trailing);
            rect.setRight(rect.right() - width - GapHM);
        }

        const QFont font = StyleHelper::uiFont(StyleHelper::UiElementBody2);
        painter->setFont(font);
        painter->setPen(creatorColor(selected ? Theme::Token_Text_On_Accent
                                              : Theme::Token_Text_Default));
        const QString text = QFontMetrics(font).elidedText(index.data().toString(),
                                                           Qt::ElideRight, rect.width());
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        using namespace StyleHelper::SpacingTokens;
        const int textHeight
            = QFontMetrics(StyleHelper::uiFont(StyleHelper::UiElementBody2)).height();
        return {QStyledItemDelegate::sizeHint(option, index).width(),
                std::max(iconSize, textHeight) + 2 * PaddingVXs};
    }

private:
    QString trailingText(const QModelIndex &index) const
    {
        if (m_trailingText == Position)
            return QString::number(index.row() + 1);
        if (isUserItemTemplate(keyFromString(index.data(Qt::UserRole).toString())))
            return Tr::tr("Reusable");
        return {};
    }

    static constexpr int iconSize = 16;
    const TrailingText m_trailingText;
};

static Icon tintedIcon(const FilePath &image)
{
    return Icon({{image, Theme::Token_Text_Default}}, Icon::Tint);
}

class StyledBarCustomizeDialog final : public QDialog
{
public:
    StyledBarCustomizeDialog(StyledBar *bar, StyledBarPrivate *d)
        : QDialog(bar)
        , m_d(d)
        , m_savedOrder(d->order)
        , m_savedHiddenItems(d->hiddenItems)
    {
        using namespace StyleHelper::SpacingTokens;
        setWindowTitle(Tr::tr("Customize Toolbar"));

        m_list = new StyledBarItemList;
        m_list->setObjectName("ShownItems");
        m_list->setItemDelegate(new StyledBarItemDelegate(StyledBarItemDelegate::Position, m_list));
        m_availableList = new StyledBarAvailableList(d);
        m_availableList->setObjectName("AvailableItems");
        m_availableList->setItemDelegate(
            new StyledBarItemDelegate(StyledBarItemDelegate::ReusableTag, m_availableList));
        for (QListWidget *list : {m_list, m_availableList}) {
            list->setDragDropMode(QAbstractItemView::DragDrop);
            list->setDefaultDropAction(Qt::MoveAction);
            list->setSelectionMode(QAbstractItemView::SingleSelection);
            list->setSpacing(GapVXxs / 2);
            list->setMouseTracking(true);
            list->viewport()->setAttribute(Qt::WA_Hover);
            QPalette palette = list->palette();
            palette.setColor(QPalette::Base, creatorColor(Theme::Token_Background_Muted));
            list->setPalette(palette);
        }

        m_moveUpButton = new QtcIconButton;
        m_moveUpButton->setObjectName("MoveUpButton");
        m_moveUpButton->setIcon(Icons::ARROW_UP.icon());
        m_moveUpButton->setToolTip(Tr::tr("Move Up"));
        m_moveDownButton = new QtcIconButton;
        m_moveDownButton->setObjectName("MoveDownButton");
        m_moveDownButton->setIcon(Icons::ARROW_DOWN.icon());
        m_moveDownButton->setToolTip(Tr::tr("Move Down"));

        m_addButton = new QtcButton({}, QtcButton::MediumSecondary);
        m_addButton->setObjectName("AddButton");
        m_addButton->setThemedPixmap(tintedIcon(":/utils/images/next.png"));
        m_addButton->setToolTip(Tr::tr("Add to Toolbar"));
        m_hideButton = new QtcButton({}, QtcButton::MediumSecondary);
        m_hideButton->setObjectName("HideButton");
        m_hideButton->setThemedPixmap(tintedIcon(":/utils/images/prev.png"));
        m_hideButton->setToolTip(Tr::tr("Hide from Toolbar"));
        for (QtcButton *button : {m_addButton, m_hideButton})
            button->setAccessibleName(button->toolTip());

        auto restoreButton = new QtcButton(Tr::tr("Restore Defaults"), QtcButton::SmallGhost);
        restoreButton->setObjectName("RestoreDefaultsButton");
        restoreButton->setThemedPixmap(Icons::RESET);
        m_statusLabel = new QLabel;
        StyleHelper::applyTf(m_statusLabel, {Theme::Token_Text_Subtle,
                                             StyleHelper::UiElementCaption});
        auto cancelButton = new QtcButton(Tr::tr("Cancel"), QtcButton::MediumSecondary);
        cancelButton->setObjectName("CancelButton");
        auto okButton = new QtcButton(Tr::tr("OK"), QtcButton::MediumPrimary);
        okButton->setObjectName("OkButton");

        fill(d->order, d->hiddenItems);

        using namespace Layouting;
        Column {
            Row {
                Column {
                    Row {
                        new QtcLabel(Tr::tr("Available Items"), QtcLabel::Secondary),
                        st,
                        spacing(GapHM)
                    },
                    m_availableList,
                    spacing(GapVM)
                },
                Column { st, m_addButton, m_hideButton, st, spacing(GapVM) },
                Column {
                    Row {
                        new QtcLabel(Tr::tr("Shown Items"), QtcLabel::Secondary),
                        st,
                        m_moveUpButton,
                        m_moveDownButton,
                        spacing(GapHM)
                    },
                    m_list,
                    spacing(GapVM)
                },
                spacing(GapHL),
                customMargins(PaddingHXl, PaddingVXl, PaddingHXl, PaddingVXl)
            },
            createHr(),
            Row {
                restoreButton,
                m_statusLabel,
                st,
                cancelButton,
                okButton,
                spacing(GapHM),
                customMargins(PaddingHXl, PaddingVL, PaddingHXl, PaddingVL)
            },
            noMargin,
            spacing(0)
        }.attachTo(this);

        connect(m_moveUpButton, &QtcIconButton::clicked, this, [this] { moveCurrent(-1); });
        connect(m_moveDownButton, &QtcIconButton::clicked, this, [this] { moveCurrent(1); });
        connect(m_hideButton, &QtcButton::clicked, this, &StyledBarCustomizeDialog::hideCurrent);
        connect(m_addButton, &QtcButton::clicked, this, &StyledBarCustomizeDialog::addCurrent);
        connect(m_list, &QListWidget::itemDoubleClicked,
                this, &StyledBarCustomizeDialog::hideCurrent);
        connect(m_availableList, &QListWidget::itemDoubleClicked,
                this, &StyledBarCustomizeDialog::addCurrent);
        for (QListWidget *list : {m_list, m_availableList}) {
            connect(list, &QListWidget::currentRowChanged,
                    this, &StyledBarCustomizeDialog::updateButtons);
        }
        updateButtons();
        connect(restoreButton, &QtcButton::clicked, this, [this] {
            fill(m_d->defaultOrder(), {});
            m_statusLabel->setText(Tr::tr("Defaults restored"));
        });
        connect(okButton, &QtcButton::clicked, this, &QDialog::accept);
        connect(cancelButton, &QtcButton::clicked, this, &QDialog::reject);

        connect(m_list->model(), &QAbstractItemModel::rowsInserted,
                this, [this](const QModelIndex &, int first, int last) {
                    instantiateTemplates(first, last);
                    preview();
                });
        connect(m_list->model(), &QAbstractItemModel::dataChanged,
                this, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
                    instantiateTemplates(topLeft.row(), bottomRight.row());
                    preview();
                });
        for (QListWidget *list : {m_list, m_availableList}) {
            QAbstractItemModel *model = list->model();
            connect(model, &QAbstractItemModel::dataChanged,
                    this, [this, list](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
                        restoreIcons(list, topLeft.row(), bottomRight.row());
                    });
            if (list != m_list) {
                connect(model, &QAbstractItemModel::rowsInserted,
                        this, &StyledBarCustomizeDialog::preview);
            }
            connect(model, &QAbstractItemModel::rowsRemoved,
                    this, &StyledBarCustomizeDialog::preview);
            connect(model, &QAbstractItemModel::rowsMoved,
                    this, &StyledBarCustomizeDialog::preview);
        }
    }

    void done(int result) override
    {
        if (result == QDialog::Accepted)
            m_d->save();
        else
            m_d->setConfiguration(m_savedOrder, m_savedHiddenItems);
        QDialog::done(result);
    }

protected:
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            if (const auto button = qobject_cast<QAbstractButton *>(focusWidget()))
                button->click();
            else
                accept();
            return;
        }
        QDialog::keyPressEvent(event);
    }

private:
    static KeyList idsOf(const QListWidget *list)
    {
        KeyList result;
        for (int row = 0; row < list->count(); ++row) {
            const Key id = itemId(list->item(row));
            if (!isUserItemTemplate(id))
                result.append(id);
        }
        return result;
    }

    static bool canHide(const StyledBarPrivate *d, const Key &id)
    {
        return isUserItem(id) || !d->isPermanent(id);
    }

    void preview()
    {
        if (m_updating)
            return;
        KeyList order = idsOf(m_list);
        const KeyList hidden = idsOf(m_availableList);
        int insertPosition = 0;
        for (const Key &id : std::as_const(m_d->order)) {
            const int index = order.indexOf(id);
            if (index >= 0)
                insertPosition = index + 1;
            else if (hidden.contains(id))
                order.insert(insertPosition++, id);
        }
        m_d->setConfiguration(order, hidden);
        updateButtons();
        m_list->viewport()->update();
    }

    QListWidgetItem *createListItem(const Key &id, const QString &displayName,
                                    const QIcon &icon) const
    {
        auto listItem = new QListWidgetItem(icon, displayName);
        listItem->setData(Qt::UserRole, stringFromKey(id));
        listItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
        return listItem;
    }

    void fill(const KeyList &order, const KeyList &hidden)
    {
        m_updating = true;
        m_list->clear();
        m_availableList->clear();
        m_availableList->addItem(
            createListItem(USER_SEPARATOR_PREFIX, Tr::tr("Separator"), separatorIcon()));
        m_availableList->addItem(
            createListItem(USER_SPACER_PREFIX, Tr::tr("Spacer"), spacerIcon()));
        m_availableList->addItem(
            createListItem(USER_STRETCH_PREFIX, Tr::tr("Stretch"), stretchIcon()));
        for (const Key &id : order) {
            QListWidget *list = hidden.contains(id) ? m_availableList : m_list;
            const StyledBarItem barItem = m_d->item(id);
            list->addItem(createListItem(id, barItem.displayName, barItem.icon));
        }
        m_updating = false;
        preview();
    }

    Key newUserItemId(const Key &prefix) const
    {
        const KeyList ids = idsOf(m_list);
        Key id;
        for (int number = 1; id.isEmpty() || ids.contains(id); ++number)
            id = Key(prefix, number);
        return id;
    }

    void instantiateTemplates(int first, int last)
    {
        for (int row = first; row <= last; ++row) {
            QListWidgetItem *listItem = m_list->item(row);
            const Key id = itemId(listItem);
            if (isUserItemTemplate(id))
                listItem->setData(Qt::UserRole, stringFromKey(newUserItemId(id)));
        }
    }

    QIcon iconOf(const Key &id) const
    {
        if (isUserSeparator(id))
            return separatorIcon();
        if (isUserSpacer(id))
            return spacerIcon();
        if (isUserStretch(id))
            return stretchIcon();
        return m_d->item(id).icon;
    }

    // Dropped items lose icons that cannot be serialized into the drag's mime data.
    void restoreIcons(QListWidget *list, int first, int last) const
    {
        for (int row = first; row <= last; ++row) {
            QListWidgetItem *listItem = list->item(row);
            if (!listItem || !listItem->icon().isNull())
                continue;
            const QIcon icon = iconOf(itemId(listItem));
            if (!icon.isNull())
                listItem->setIcon(icon);
        }
    }

    void updateButtons()
    {
        const int row = m_list->currentRow();
        const QListWidgetItem *current = m_list->currentItem();
        m_hideButton->setEnabled(current && canHide(m_d, itemId(current)));
        m_addButton->setEnabled(m_availableList->currentItem());
        m_moveUpButton->setEnabled(row > 0);
        m_moveDownButton->setEnabled(row >= 0 && row < m_list->count() - 1);
    }

    void hideCurrent()
    {
        const QListWidgetItem *current = m_list->currentItem();
        if (!current || !canHide(m_d, itemId(current)))
            return;
        const int row = m_list->currentRow();
        m_updating = true;
        QListWidgetItem *listItem = m_list->takeItem(row);
        m_statusLabel->setText(Tr::tr("%1 hidden").arg(listItem->text()));
        if (isUserItem(itemId(listItem)))
            delete listItem;
        else
            m_availableList->addItem(listItem);
        m_updating = false;
        m_list->setCurrentRow(std::min(row, m_list->count() - 1));
        preview();
    }

    void addCurrent()
    {
        const int availableRow = m_availableList->currentRow();
        if (availableRow < 0)
            return;
        const Key id = itemId(m_availableList->item(availableRow));
        m_updating = true;
        QListWidgetItem *listItem = isUserItemTemplate(id)
            ? m_availableList->item(availableRow)->clone()
            : m_availableList->takeItem(availableRow);
        const int row = m_list->currentRow() < 0 ? m_list->count() : m_list->currentRow() + 1;
        m_list->insertItem(row, listItem);
        m_updating = false;
        m_list->setCurrentRow(row);
        m_statusLabel->setText(Tr::tr("%1 added").arg(listItem->text()));
        preview();
    }

    void moveCurrent(int offset)
    {
        const int row = m_list->currentRow();
        const int newRow = row + offset;
        if (row < 0 || newRow < 0 || newRow >= m_list->count())
            return;
        m_updating = true;
        m_list->insertItem(newRow, m_list->takeItem(row));
        m_updating = false;
        m_list->setCurrentRow(newRow);
        preview();
    }

    StyledBarPrivate *m_d = nullptr;
    const KeyList m_savedOrder;
    const KeyList m_savedHiddenItems;
    QListWidget *m_list = nullptr;
    QListWidget *m_availableList = nullptr;
    QtcIconButton *m_moveUpButton = nullptr;
    QtcIconButton *m_moveDownButton = nullptr;
    QtcButton *m_hideButton = nullptr;
    QtcButton *m_addButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    bool m_updating = false;
};

} // namespace Internal

/*!
    \class Utils::StyledBar
    \inheaderfile utils/widgets.h
    \inmodule QtCreator

    \brief The StyledBar class is a panel widget for tool bars.

    Widgets added with addItem() are laid out by the bar itself. The user can
    then change their order and hide them in the customization dialog that
    the context menu of the bar opens, except for the items passed to
    setPermanentItems(). Use setItemVisible()
    instead of QWidget::setVisible() to show or hide such items from code.
    In the customization dialog, the user can also add any number of
    separators, fixed-width spacers, and stretches. Hiding them removes
    them again.
    With setSettingsKey(), the configuration is stored in the user settings.

    Like QToolBar, the bar moves the trailing items that do not fit into a
    popup that an extension button at its end opens. Permanent items always
    stay in the bar.
*/

StyledBar::StyledBar(QWidget *parent)
    : QWidget(parent)
    , d(new Internal::StyledBarPrivate(this))
{
    StyleHelper::setPanelWidget(this);
    StyleHelper::setPanelWidgetSingleRow(this);
    setProperty(StyleHelper::C_LIGHT_COLORED, false);
}

StyledBar::~StyledBar()
{
    if (d->overflowPopup)
        d->overflowPopup->disarm();
    delete d;
}

bool Internal::StyledBarPrivate::addItem(const Key &id, const QString &displayName,
                                         QWidget *widget, const QIcon &icon)
{
    QTC_ASSERT(widget, return false);
    QTC_ASSERT(!isKnown(id), return false);
    if (!layout) {
        QTC_ASSERT(!q->layout(), return false);
        layout = new QHBoxLayout(q);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->setSizeConstraint(QLayout::SetNoConstraint);
        extensionButton = new QToolButton(q);
        extensionButton->setObjectName("StyledBarExtensionButton");
        extensionButton->setIcon(Icons::TOOLBAR_EXTENSION.icon());
        extensionButton->setToolTip(Tr::tr("Show More Items"));
        extensionButton->hide();
        QObject::connect(extensionButton, &QToolButton::clicked, q, [this] { showOverflow(); });
    }
    items.append({id, displayName, widget, true, icon});
    order = mergedOrder(order);
    apply();
    return true;
}

/*!
    Adds \a widget with the identifier \a id to the end of the bar. The user
    sees it as \a displayName when configuring the bar.
*/
void StyledBar::addItem(const Key &id, const QString &displayName, QWidget *widget)
{
    const auto button = qobject_cast<QAbstractButton *>(widget);
    d->addItem(id, displayName, widget, button ? button->icon() : QIcon());
}

/*!
    Adds a stretchable space with the identifier \a id to the end of the bar.
    The user can move it, but not hide it.
*/
void StyledBar::addStretch(const Key &id)
{
    QTC_ASSERT(!d->isKnown(id), return);
    auto spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    d->stretchItems.append(id);
    d->layoutItems.append(id);
    if (!d->addItem(id, Tr::tr("Stretch"), spacer, Internal::stretchIcon())) {
        d->stretchItems.removeAll(id);
        d->layoutItems.removeAll(id);
        delete spacer;
    }
}

/*!
    Adds a fixed-width space with the identifier \a id to the end of the bar.
    The user can move and hide it in the toolbar customization dialog.
*/
void StyledBar::addSpacer(const Key &id)
{
    QTC_ASSERT(!d->isKnown(id), return);
    auto spacer = new QWidget(this);
    spacer->setFixedWidth(StyleHelper::SpacingTokens::GapHXl);
    d->layoutItems.append(id);
    if (!d->addItem(id, Tr::tr("Spacer"), spacer, Internal::spacerIcon())) {
        d->layoutItems.removeAll(id);
        delete spacer;
    }
}

/*!
    Adds a separator with the identifier \a id to the end of the bar.
    The user can move and hide it in the toolbar customization dialog.
*/
void StyledBar::addSeparator(const Key &id)
{
    QTC_ASSERT(!d->isKnown(id), return);
    auto separator = new StyledSeparator(this);
    d->layoutItems.append(id);
    if (!d->addItem(id, Tr::tr("Separator"), separator, Internal::separatorIcon())) {
        d->layoutItems.removeAll(id);
        delete separator;
    }
}

/*!
    Prevents the user from hiding the items with the identifiers \a ids.
*/
void StyledBar::setPermanentItems(const KeyList &ids)
{
    d->permanentItems = ids;
    d->hiddenItems = Utils::filtered(d->hiddenItems,
                                     [this](const Key &id) { return !d->isPermanent(id); });
    d->apply();
}

/*!
    Shows or hides the item \a id depending on \a visible. An item the user
    has hidden stays hidden.
*/
void StyledBar::setItemVisible(const Key &id, bool visible)
{
    const auto it = std::find_if(d->items.begin(), d->items.end(),
                                 Utils::equal(&Internal::StyledBarItem::id, id));
    QTC_ASSERT(it != d->items.end(), return);
    if (it->visible == visible)
        return;
    it->visible = visible;
    d->apply();
}

/*!
    Sets the stretch factor of the item \a id to \a stretch. As in
    QBoxLayout, once an item has a stretch factor, extra space goes to the
    items with one, in proportion to their factors.
*/
void StyledBar::setItemStretch(const Key &id, int stretch)
{
    const auto it = std::find_if(d->items.begin(), d->items.end(),
                                 Utils::equal(&Internal::StyledBarItem::id, id));
    QTC_ASSERT(it != d->items.end(), return);
    if (it->stretch == stretch)
        return;
    it->stretch = stretch;
    d->apply();
}

/*!
    Orders the items as given by \a ids. Unknown identifiers are ignored,
    items missing from \a ids are placed after their predecessor in the
    order in which they were added.

    Separators and spacers added by the user are the exception: an unknown
    identifier with their prefix creates a new one, and those missing from
    \a ids are removed from the bar.
*/
void StyledBar::setItemOrder(const KeyList &ids)
{
    d->setConfiguration(ids, d->hiddenItems);
    d->save();
}

/*!
    Returns the identifiers of all items in the order in which they appear
    in the bar, including hidden ones.
*/
KeyList StyledBar::itemOrder() const
{
    return d->order;
}

/*!
    Hides the items with the identifiers \a ids, except for permanent ones.
*/
void StyledBar::setHiddenItems(const KeyList &ids)
{
    d->setConfiguration(d->order, ids);
    d->save();
}

/*!
    Returns the identifiers of the hidden items.
*/
KeyList StyledBar::hiddenItems() const
{
    return d->hiddenItems;
}

/*!
    Restores the order in which the items were added and shows all of them.
*/
void StyledBar::resetItemConfiguration()
{
    d->setConfiguration(d->defaultOrder(), {});
    d->save();
}

/*!
    Restores the item configuration from the user settings at \a key and
    stores it there on every change. Call this after adding the items.
*/
void StyledBar::setSettingsKey(const Key &key)
{
    d->settingsKey = key;
    d->load();
}

void StyledBar::setSingleRow(bool singleRow)
{
    StyleHelper::setPanelWidgetSingleRow(this, singleRow);
}

bool StyledBar::isSingleRow() const
{
    return property(StyleHelper::C_PANEL_WIDGET_SINGLE_ROW).toBool();
}

void StyledBar::setLightColored(bool lightColored)
{
    if (isLightColored() == lightColored)
        return;
    setProperty(StyleHelper::C_LIGHT_COLORED, lightColored);
    const QList<QWidget *> children = findChildren<QWidget *>();
    for (QWidget *childWidget : children)
        childWidget->style()->polish(childWidget);
}

bool StyledBar::isLightColored() const
{
    return property(StyleHelper::C_LIGHT_COLORED).toBool();
}

QSize StyledBar::sizeHint() const
{
    QSize size = QWidget::sizeHint();
    if (d->layout)
        size.setWidth(d->fullWidth());
    return size;
}

QSize StyledBar::minimumSizeHint() const
{
    QSize size = QWidget::minimumSizeHint();
    if (d->layout)
        size.setWidth(d->minimumWidth());
    return size;
}

bool StyledBar::event(QEvent *event)
{
    const bool result = QWidget::event(event);
    if (event->type() == QEvent::LayoutRequest)
        d->updateOverflow();
    return result;
}

void StyledBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    d->updateOverflow();
}

void StyledBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    Internal::paintToolBarBackground(this);
}

void StyledBar::contextMenuEvent(QContextMenuEvent *event)
{
    if (d->items.isEmpty()) {
        QWidget::contextMenuEvent(event);
        return;
    }

    QMenu menu(this);
    connect(menu.addAction(Tr::tr("Customize Toolbar...")), &QAction::triggered, this, [this] {
        auto dialog = new Internal::StyledBarCustomizeDialog(this, d);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->open();
    });
    connect(menu.addAction(Tr::tr("Reset Toolbar")), &QAction::triggered,
            this, &StyledBar::resetItemConfiguration);
    menu.exec(event->globalPos());
}

StyledSeparator::StyledSeparator(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(10);
}

void StyledSeparator::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    QStyleOption option;
    option.rect = rect();
    option.state = QStyle::State_Horizontal;
    option.palette = palette();
    style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &option, &painter, this);
}

TextFieldCheckBox::TextFieldCheckBox(const QString &text, QWidget *parent) :
    QCheckBox(text, parent),
    m_trueText(QLatin1String("true")), m_falseText(QLatin1String("false"))
{
    connect(this, &QCheckBox::checkStateChanged, this, &TextFieldCheckBox::slotStateChanged);
}

QString TextFieldCheckBox::text() const
{
    return isChecked() ? m_trueText : m_falseText;
}

void TextFieldCheckBox::setText(const QString &s)
{
    setChecked(s == m_trueText);
}

void TextFieldCheckBox::slotStateChanged(int cs)
{
    emit textChanged(cs == Qt::Checked ? m_trueText : m_falseText);
}

TextFieldComboBox::TextFieldComboBox(QWidget *parent) :
    QComboBox(parent)
{
    setEditable(false);
    connect(this, &QComboBox::currentIndexChanged,
            this, &TextFieldComboBox::slotCurrentIndexChanged);
}

QString TextFieldComboBox::text() const
{
    return valueAt(currentIndex());
}

void TextFieldComboBox::setText(const QString &s)
{
    const int index = findData(QVariant(s), Qt::UserRole);
    if (index != -1 && index != currentIndex())
        setCurrentIndex(index);
}

void TextFieldComboBox::slotCurrentIndexChanged(int i)
{
    emit text4Changed(valueAt(i));
}

void TextFieldComboBox::setItems(const QStringList &displayTexts,
                                 const QStringList &values)
{
    QTC_ASSERT(displayTexts.size() == values.size(), return);
    clear();
    addItems(displayTexts);
    const int count = values.count();
    for (int i = 0; i < count; i++)
        setItemData(i, QVariant(values.at(i)), Qt::UserRole);
}

QString TextFieldComboBox::valueAt(int i) const
{
    return i >= 0 && i < count() ? itemData(i, Qt::UserRole).toString() : QString();
}

/*!
 * Opens \a menu at the specified \a widget position.
 * This function computes the position where to show the menu, and opens it with
 * QMenu::exec().
 */
QAction *execMenuAtWidget(QMenu *menu, QWidget *widget)
{
    QPoint p;
    QRect screen = widget->screen()->availableGeometry();
    QSize sh = menu->sizeHint();
    QRect rect = widget->rect();
    if (widget->isRightToLeft()) {
        if (widget->mapToGlobal(QPoint(0, rect.bottom())).y() + sh.height() <= screen.height())
            p = widget->mapToGlobal(rect.bottomRight());
        else
            p = widget->mapToGlobal(rect.topRight() - QPoint(0, sh.height()));
        p.rx() -= sh.width();
    } else {
        if (widget->mapToGlobal(QPoint(0, rect.bottom())).y() + sh.height() <= screen.height())
            p = widget->mapToGlobal(rect.bottomLeft());
        else
            p = widget->mapToGlobal(rect.topLeft() - QPoint(0, sh.height()));
    }
    p.rx() = qMax(screen.left(), qMin(p.x(), screen.right() - sh.width()));
    p.ry() += 1;

    return menu->exec(p);
}

/*!
    Adds tool tips to the \a menu that show the action's tool tip when hovering
    over an entry.
 */
void addToolTipsToMenu(QMenu *menu)
{
    QObject::connect(menu, &QMenu::hovered, menu, [menu](QAction *action) {
        ToolTip::show(menu->mapToGlobal(menu->actionGeometry(action).topRight()), action->toolTip());
    });
}

QProgressDialog *createProgressDialog(int maxValue, const QString &windowTitle,
                                      const QString &labelText)
{
    QProgressDialog *progressDialog = new QProgressDialog(labelText, Tr::tr("Cancel"),
                                                          0, maxValue, dialogParent());
    progressDialog->setWindowModality(Qt::ApplicationModal);
    progressDialog->setMinimumDuration(INT_MAX); // see QTBUG-135797
    progressDialog->setWindowTitle(windowTitle);
    progressDialog->setFixedSize(progressDialog->sizeHint());
    progressDialog->setAutoClose(false);
    progressDialog->show();
    return progressDialog;
}

namespace AsynchronousMessageBox {
namespace {
QWidget *message(QMessageBox::Icon icon, const QString &title, const QString &desciption)
{
    QMessageBox *messageBox = new QMessageBox(icon,
                                              title,
                                              desciption,
                                              QMessageBox::Ok,
                                              Utils::dialogParent());

    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    messageBox->setModal(true);
    messageBox->show();
    return messageBox;
}
} // anonymous namespace

QWidget *warning(const QString &title, const QString &desciption)
{
    return message(QMessageBox::Warning, title, desciption);
}

QWidget *information(const QString &title, const QString &desciption)
{
    return message(QMessageBox::Information, title, desciption);
}

QWidget *critical(const QString &title, const QString &desciption)
{
    return message(QMessageBox::Critical, title, desciption);
}
} // namespace AsynchronousMessageBox

} // namespace Utils
