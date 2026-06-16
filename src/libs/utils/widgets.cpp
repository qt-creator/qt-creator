// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "widgets.h"

#include "filepath.h"
#include "guiutils.h"
#include "hostosinfo.h"
#include "icon.h"
#include "layoutbuilder.h"
#include "qtcassert.h"
#include "stylehelper.h"
#include "tooltip/tooltip.h"
#include "utilstr.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QCompleter>
#include <QDialogButtonBox>
#include <qdrawutil.h>
#include <QGraphicsOpacityEffect>
#include <QHeaderView>
#include <QHideEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QProgressDialog>
#include <QPropertyAnimation>
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QStylePainter>
#include <QTimer>

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

StyledBar::StyledBar(QWidget *parent)
    : QWidget(parent)
{
    StyleHelper::setPanelWidget(this);
    StyleHelper::setPanelWidgetSingleRow(this);
    setProperty(StyleHelper::C_LIGHT_COLORED, false);
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

void StyledBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    QStyleOptionToolBar option;
    option.rect = rect();
    option.state = QStyle::State_Horizontal;
    style()->drawControl(QStyle::CE_ToolBar, &option, &painter, this);
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
    connect(this, &QCheckBox::stateChanged, this, &TextFieldCheckBox::slotStateChanged);
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
