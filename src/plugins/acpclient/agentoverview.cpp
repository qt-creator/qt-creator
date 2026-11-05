// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "agentoverview.h"
#include "acpclienttr.h"

#include <utils/infolabel.h>
#include <utils/progressindicator.h>
#include <utils/qtdesignwidgets.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QButtonGroup>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollArea>
#include <QVBoxLayout>

using namespace Utils;

namespace AcpClient::Internal {

using namespace StyleHelper::SpacingTokens;

static const int separatorWidth = 1;

static bool usesServerIcon(SessionActivity activity, const QIcon &serverIcon)
{
    return !serverIcon.isNull()
           && (activity == SessionActivity::Idle || activity == SessionActivity::None);
}

static const Icon *activityIcon(SessionActivity activity)
{
    switch (activity) {
    case SessionActivity::None:
    case SessionActivity::Idle:
        return nullptr;
    case SessionActivity::Running:
        return &Icons::RUN_SMALL;
    case SessionActivity::InputRequired:
        return &Icons::WARNING;
    }
    return nullptr;
}

QIcon sessionActivityIcon(SessionActivity activity, const QIcon &serverIcon,
                          const QIcon &runningIcon)
{
    if (usesServerIcon(activity, serverIcon))
        return serverIcon;
    if (activity == SessionActivity::Running && !runningIcon.isNull())
        return runningIcon;
    const Icon *icon = activityIcon(activity);
    return icon ? icon->icon() : QIcon();
}

QString sessionActivityText(SessionActivity activity)
{
    switch (activity) {
    case SessionActivity::None:
        return Tr::tr("No session");
    case SessionActivity::Idle:
        return Tr::tr("Idle");
    case SessionActivity::Running:
        return Tr::tr("Running");
    case SessionActivity::InputRequired:
        return Tr::tr("User input required");
    }
    return {};
}

static Theme::Color sessionActivityColor(SessionActivity activity)
{
    switch (activity) {
    case SessionActivity::Running:
        return Theme::Token_Text_Accent;
    case SessionActivity::InputRequired:
        return Theme::Token_Notification_Alert_Default;
    case SessionActivity::None:
    case SessionActivity::Idle:
        break;
    }
    return Theme::Token_Text_Muted;
}

/*!
    \class AcpClient::Internal::RunningIndicator
    \internal

    Animates a small progress indicator for running sessions. While active, it
    renders each frame into icon() and announces it with iconChanged().
*/

RunningIndicator::RunningIndicator(QObject *parent)
    : QObject(parent)
    , m_painter(std::make_unique<ProgressIndicatorPainter>(ProgressIndicatorSize::Small))
{
    m_painter->setUpdateCallback([this] { renderFrame(); });
}

RunningIndicator::~RunningIndicator() = default;

void RunningIndicator::setActive(bool active)
{
    if (active == m_active)
        return;
    m_active = active;
    if (active) {
        renderFrame();
        m_painter->startAnimation();
    } else {
        m_painter->stopAnimation();
    }
}

void RunningIndicator::renderFrame()
{
    const QSize size = m_painter->size();
    const qreal dpr = qApp->devicePixelRatio();
    QPixmap frame(size * dpr);
    frame.setDevicePixelRatio(dpr);
    frame.fill(Qt::transparent);
    QPainter painter(&frame);
    m_painter->paint(painter, QRect(QPoint(), size));
    painter.end();
    m_icon = QIcon(frame);
    emit iconChanged();
}

static const StyleHelper::TextFormat &nameFormat()
{
    static const StyleHelper::TextFormat tf{Theme::Token_Text_Default,
                                            StyleHelper::UiElementLabelMedium};
    return tf;
}

static const StyleHelper::TextFormat &detailFormat()
{
    static const StyleHelper::TextFormat tf{Theme::Token_Text_Muted,
                                            StyleHelper::UiElementIconStandard};
    return tf;
}

static QSize entryIconSize()
{
    return Icons::OK.pixmap().deviceIndependentSize().toSize();
}

static int summaryHeight()
{
    return qMax(entryIconSize().height(),
                qMax(nameFormat().lineHeight(), detailFormat().lineHeight()));
}

static QString sessionEntryLabel(const SessionEntry &entry)
{
    //: %1 = agent name, %2 = session state, such as "Running"
    return Tr::tr("%1 (%2)").arg(entry.agentName, sessionActivityText(entry.activity));
}

QString sessionEntryText(const SessionEntry &entry)
{
    const QString label = sessionEntryLabel(entry);
    return entry.path.isEmpty() ? label : label + " - " + entry.path;
}

QString sessionEntryToolTip(const SessionEntry &entry)
{
    if (entry.activity == SessionActivity::Idle)
        return entry.path;
    const QString state = sessionActivityText(entry.activity);
    return entry.path.isEmpty() ? state : entry.path + '\n' + state;
}

// Paints the server icon, the agent name, the path in a muted color and, at the
// right end of rect, the state of a session that is not idle.
static void paintSessionSummary(QPainter &p, const QRect &rect, const SessionEntry &entry,
                                const QIcon &runningIcon, bool enabled)
{
    const QSize iconSize = entryIconSize();
    const auto iconRect = [&](int x) {
        return QRect(QPoint(x, rect.top() + (rect.height() - iconSize.height()) / 2), iconSize);
    };
    const auto color = [enabled](Theme::Color c) {
        return creatorColor(enabled ? c : Theme::Token_Text_Subtle);
    };

    int x = rect.left();
    if (!entry.serverIcon.isNull())
        entry.serverIcon.paint(&p, iconRect(x));
    x += iconSize.width() + GapHS;

    const QFont detailFont = detailFormat().font();
    const QFontMetrics detailFm(detailFont);

    // The state, right aligned.
    int right = rect.right() + 1;
    if (entry.activity == SessionActivity::Running
        || entry.activity == SessionActivity::InputRequired) {
        const QString state = sessionActivityText(entry.activity);
        const int stateWidth = detailFm.horizontalAdvance(state);
        const int stateX = right - stateWidth;
        const int indicatorX = stateX - GapHXs - iconSize.width();
        if (indicatorX > x) {
            sessionActivityIcon(entry.activity, {}, runningIcon).paint(&p, iconRect(indicatorX));
            p.setFont(detailFont);
            p.setPen(color(sessionActivityColor(entry.activity)));
            p.drawText(QRect(stateX, rect.top(), stateWidth, rect.height()),
                       Qt::AlignLeft | Qt::AlignVCenter, state);
            right = indicatorX - GapHM;
        }
    }

    // The name, then the path in the room that is left.
    const QFont nameFont = nameFormat().font();
    const QFontMetrics nameFm(nameFont);
    const int nameAvailable = right - x;
    if (nameAvailable <= 0)
        return;
    const QString name = nameFm.elidedText(entry.agentName, Qt::ElideRight, nameAvailable);
    const int nameWidth = nameFm.horizontalAdvance(name);
    p.setFont(nameFont);
    p.setPen(color(nameFormat().themeColor));
    p.drawText(QRect(x, rect.top(), nameWidth, rect.height()),
               Qt::AlignLeft | Qt::AlignVCenter, name);
    x += nameWidth + GapHM;

    const int pathAvailable = right - x;
    if (entry.path.isEmpty() || pathAvailable <= 0)
        return;
    p.setFont(detailFont);
    p.setPen(color(detailFormat().themeColor));
    p.drawText(QRect(x, rect.top(), pathAvailable, rect.height()),
               Qt::AlignLeft | Qt::AlignVCenter,
               detailFm.elidedText(entry.path, Qt::ElideMiddle, pathAvailable));
}

/*!
    \class AcpClient::Internal::SessionSwitcherButton
    \internal

    Shows the current chat in the tool bar: an icon, the agent name and the
    workspace in a muted color, followed by an arrow that points down while the
    list of all chats is closed and up while it is open, which is when the
    button is checked. The icon is the state of a session that runs or waits
    for the user, and the icon of the server otherwise.
*/

SessionSwitcherButton::SessionSwitcherButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::TabFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void SessionSwitcherButton::setEntry(const SessionEntry &entry, const QIcon &runningIcon)
{
    m_entry = entry;
    setIcon(sessionActivityIcon(entry.activity, entry.serverIcon, runningIcon));
    setAccessibleName(sessionEntryText(entry));
    setToolTip(Tr::tr("Show All Chats") + '\n' + sessionEntryToolTip(entry));
    updateGeometry();
    update();
}

static int switcherFixedWidth()
{
    // Icon, name gap, path gap and arrow, with the padding at both ends.
    return PaddingHS + entryIconSize().width() + GapHS + GapHS + GapHXs
           + entryIconSize().width() + PaddingHS;
}

QSize SessionSwitcherButton::sizeHint() const
{
    const int nameWidth = QFontMetrics(nameFormat().font()).horizontalAdvance(m_entry.agentName);
    const int pathWidth = m_entry.path.isEmpty()
                              ? -GapHS
                              : QFontMetrics(detailFormat().font()).horizontalAdvance(m_entry.path);
    return {switcherFixedWidth() + nameWidth + pathWidth,
            PaddingVXs + summaryHeight() + PaddingVXs};
}

QSize SessionSwitcherButton::minimumSizeHint() const
{
    const int nameWidth = QFontMetrics(nameFormat().font()).horizontalAdvance(m_entry.agentName);
    return {switcherFixedWidth() - GapHS + nameWidth, sizeHint().height()};
}

void SessionSwitcherButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    const bool hovered = underMouse() && isEnabled();
    if (isChecked() || hovered || isDown()) {
        const Theme::Color fill = isChecked() ? Theme::Token_Foreground_Muted
                                              : Theme::Token_Foreground_Subtle;
        StyleHelper::drawCardBg(&p, rect(), creatorColor(fill));
    }
    if (hasFocus()) {
        StyleHelper::drawCardBg(&p, QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), Qt::NoBrush,
                                QPen(creatorColor(Theme::Token_Stroke_Strong)));
    }

    const QSize iconSize = entryIconSize();
    const auto iconRect = [&](int x) {
        return QRect(QPoint(x, (height() - iconSize.height()) / 2), iconSize);
    };
    const auto color = [this](Theme::Color c) {
        return creatorColor(isEnabled() ? c : Theme::Token_Text_Subtle);
    };
    const QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;

    int x = PaddingHS;
    if (!icon().isNull())
        icon().paint(&p, iconRect(x), Qt::AlignCenter, mode);
    x += iconSize.width() + GapHS;

    // The arrow follows the text, and stays visible when the text is elided.
    const int arrowRoom = GapHXs + iconSize.width() + PaddingHS;
    const int textRight = width() - arrowRoom;

    const QFont nameFont = nameFormat().font();
    const QFontMetrics nameFm(nameFont);
    const QString name = nameFm.elidedText(m_entry.agentName, Qt::ElideRight,
                                           qMax(0, textRight - x));
    const int nameWidth = nameFm.horizontalAdvance(name);
    p.setFont(nameFont);
    p.setPen(color(nameFormat().themeColor));
    p.drawText(QRect(x, 0, nameWidth, height()), Qt::AlignLeft | Qt::AlignVCenter, name);
    x += nameWidth;

    const int pathAvailable = textRight - x - GapHS;
    if (!m_entry.path.isEmpty() && pathAvailable > 0) {
        const QFont pathFont = detailFormat().font();
        const QFontMetrics pathFm(pathFont);
        const QString path = pathFm.elidedText(m_entry.path, Qt::ElideMiddle, pathAvailable);
        const int pathWidth = pathFm.horizontalAdvance(path);
        if (pathWidth > 0) {
            x += GapHS;
            p.setFont(pathFont);
            p.setPen(color(detailFormat().themeColor));
            p.drawText(QRect(x, 0, pathWidth, height()), Qt::AlignLeft | Qt::AlignVCenter, path);
            x += pathWidth;
        }
    }

    (isChecked() ? Icons::ARROW_UP : Icons::ARROW_DOWN)
        .icon()
        .paint(&p, iconRect(x + GapHXs), Qt::AlignCenter, mode);
}

// Takes its room in the row at all times, but is only painted while the row is
// hovered. The row repaints on hover, which repaints this button with it.
class CloseChatButton final : public QtcIconButton
{
public:
    using QtcIconButton::QtcIconButton;

protected:
    void paintEvent(QPaintEvent *event) final
    {
        const QWidget *row = parentWidget();
        if (row && row->underMouse() && isEnabled())
            QtcIconButton::paintEvent(event);
    }
};

// One chat in the list: its summary and a close button at the right end. At
// rest it has the color of the list, hovered and current it is filled.
class SessionRow final : public QAbstractButton
{
public:
    SessionRow()
    {
        setCheckable(true);
        setAttribute(Qt::WA_Hover);
        setFocusPolicy(Qt::StrongFocus);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_closeButton = new CloseChatButton(this);
        m_closeButton->setIcon(Icons::CLOSE_FOREGROUND.icon());
        m_closeButton->setFocusPolicy(Qt::NoFocus);
        m_closeButton->setToolTip(Tr::tr("Close Chat"));
        m_closeButton->setObjectName("closeChatButton");
    }

    QtcIconButton *closeButton() const { return m_closeButton; }

    void setEntry(const SessionEntry &entry)
    {
        m_entry = entry;
        setAccessibleName(sessionEntryText(entry));
        setToolTip(sessionEntryToolTip(entry));
        update();
    }
    const SessionEntry &entry() const { return m_entry; }

    void setRunningIcon(const QIcon &icon)
    {
        m_runningIcon = icon;
        if (m_entry.activity == SessionActivity::Running)
            update();
    }

    QSize sizeHint() const final
    {
        const int height = qMax(PaddingVS + summaryHeight() + PaddingVS,
                                m_closeButton->sizeHint().height() + 2 * PaddingVXxs);
        return {PaddingHS + entryIconSize().width() * 2 + GapHS * 2
                    + m_closeButton->sizeHint().width() + PaddingHXs,
                height};
    }
    QSize minimumSizeHint() const final { return sizeHint(); }

protected:
    void keyPressEvent(QKeyEvent *event) final
    {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            click();
            return;
        }
        QAbstractButton::keyPressEvent(event);
    }

    void resizeEvent(QResizeEvent *event) final
    {
        QAbstractButton::resizeEvent(event);
        const QSize size = m_closeButton->sizeHint();
        m_closeButton->setGeometry(width() - PaddingHXs - size.width(),
                                   (height() - size.height()) / 2,
                                   size.width(),
                                   size.height());
    }

    void paintEvent(QPaintEvent *) final
    {
        QPainter p(this);
        const bool hovered = underMouse() && isEnabled();
        const Theme::Color fill = isChecked() ? Theme::Token_Foreground_Default
                                  : hovered || isDown() ? Theme::Token_Foreground_Muted
                                                        : Theme::Token_Background_Muted;
        StyleHelper::drawCardBg(&p, rect(), creatorColor(fill));
        if (hasFocus()) {
            StyleHelper::drawCardBg(&p, QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5),
                                    Qt::NoBrush, QPen(creatorColor(Theme::Token_Stroke_Strong)));
        }

        const int right = m_closeButton->geometry().left() - GapHS;
        const QRect content(PaddingHS, 0, right - PaddingHS, height());
        paintSessionSummary(p, content, m_entry, m_runningIcon, isEnabled());
    }

private:
    QtcIconButton *m_closeButton = nullptr;
    SessionEntry m_entry;
    QIcon m_runningIcon;
};

/*!
    \class AcpClient::Internal::AgentOverview
    \internal

    Lists the open chats with the activity state of their sessions, one button
    per chat, in a rounded frame. Clicking a button, or the close button inside
    it, reports its index, which is the index of the chat it was created from in
    the list passed to setEntries(). Escape asks to dismiss the list.
*/

AgentOverview::AgentOverview(QWidget *parent)
    : QWidget(parent)
{
    // Never taller than all entries, but shorter when it is given less room.
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    auto *layout = new QVBoxLayout(this);
    // The bottom margin leaves room for the separator to the chat below.
    layout->setContentsMargins(PaddingHM, PaddingVM, PaddingHM,
                               PaddingVM + separatorWidth);
    layout->setSpacing(0);

    auto *frame = new QtcRectangleWidget;
    // Drawn like the chat input.
    frame->setRadius(RadiusM);
    frame->setFillBrush(creatorColor(Theme::Token_Background_Muted));
    frame->setStrokePen(QPen(creatorColor(Theme::Token_Foreground_Muted)));
    frame->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(frame);

    auto *frameLayout = new QVBoxLayout(frame);
    // The same padding as inside the chat input.
    frameLayout->setContentsMargins(PaddingHM, PaddingVM, PaddingHM, PaddingVM);
    frameLayout->setSpacing(0);

    auto *container = new QWidget;
    m_buttonsLayout = new QVBoxLayout(container);
    m_buttonsLayout->setContentsMargins(0, 0, 0, 0);
    m_buttonsLayout->setSpacing(GapVXxs);
    m_buttonsLayout->addStretch();

    m_scrollArea = new QScrollArea;
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidget(container);
    // setWidget() turns the background filling on. The frame paints it.
    container->setAutoFillBackground(false);
    m_scrollArea->viewport()->setAutoFillBackground(false);
    frameLayout->addWidget(m_scrollArea);

    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->setExclusive(true);
    connect(m_buttonGroup, &QButtonGroup::idClicked, this, &AgentOverview::entryActivated);

    m_emptyLabel = new InfoLabel(Tr::tr("No open chats."), InfoLabelType::Information);
    frameLayout->addWidget(m_emptyLabel);

    setEntries({});
}

void AgentOverview::setEntries(const QList<Entry> &entries)
{
    while (m_buttons.size() > entries.size()) {
        QAbstractButton *button = m_buttons.takeLast();
        m_buttonGroup->removeButton(button);
        delete button;
    }
    while (m_buttons.size() < entries.size()) {
        auto *row = new SessionRow;
        row->setRunningIcon(m_runningIcon);
        connect(row->closeButton(), &QAbstractButton::clicked, this, [this, row] {
            emit entryCloseRequested(m_buttons.indexOf(row));
        });
        m_buttonGroup->addButton(row, m_buttons.size());
        // Before the trailing stretch.
        m_buttonsLayout->insertWidget(m_buttons.size(), row);
        m_buttons.append(row);
    }

    for (int i = 0; i < entries.size(); ++i)
        static_cast<SessionRow *>(m_buttons.at(i))->setEntry(entries.at(i));

    // The list grows with its entries up to the height it is given.
    const int rowHeight = m_buttons.isEmpty() ? 0 : m_buttons.first()->sizeHint().height();
    m_scrollArea->setMinimumHeight(qMin<int>(entries.size(), 1) * rowHeight);
    m_scrollArea->setVisible(!entries.isEmpty());
    m_emptyLabel->setVisible(entries.isEmpty());
    updateGeometry();
}

// Tall enough for all entries. QScrollArea::sizeHint() cannot be used for this:
// it keeps the size of its widget from the first time it was asked, and it
// limits itself to a few lines of text.
QSize AgentOverview::sizeHint() const
{
    const QWidget *content = m_scrollArea->isVisibleTo(this) ? m_scrollArea->widget()
                                                             : m_emptyLabel;
    const QMargins outer = layout()->contentsMargins();
    const QMargins inner = m_scrollArea->parentWidget()->layout()->contentsMargins();
    QSize hint = QWidget::sizeHint();
    hint.setHeight(outer.top() + inner.top() + content->sizeHint().height() + inner.bottom()
                   + outer.bottom());
    return hint;
}

void AgentOverview::setRunningIcon(const QIcon &icon)
{
    m_runningIcon = icon;
    for (QAbstractButton *button : std::as_const(m_buttons))
        static_cast<SessionRow *>(button)->setRunningIcon(icon);
}

void AgentOverview::setCurrentEntry(int index)
{
    if (index >= 0 && index < m_buttons.size())
        m_buttons.at(index)->setChecked(true);
}

void AgentOverview::focusCurrentEntry()
{
    if (QAbstractButton *button = m_buttonGroup->checkedButton())
        button->setFocus();
    else if (!m_buttons.isEmpty())
        m_buttons.first()->setFocus();
}

void AgentOverview::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(QRect(0, height() - separatorWidth, width(), separatorWidth),
               creatorColor(Theme::Token_Stroke_Subtle));
}

bool AgentOverview::event(QEvent *event)
{
    // Escape belongs to the list while it has the focus, not to a shortcut.
    if (event->type() == QEvent::ShortcutOverride
        && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
        event->accept();
        return true;
    }
    return QWidget::event(event);
}

void AgentOverview::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit dismissRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace AcpClient::Internal
