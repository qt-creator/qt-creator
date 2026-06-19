// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sessionpickerwidget.h"
#include "acpclienttr.h"

#include <coreplugin/icore.h>

#include <utils/elidinglabel.h>
#include <utils/fileutils.h>
#include <utils/qtcsettings.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

using namespace Acp;

namespace AcpClient::Internal {

const char kCurrentGroupCollapsedKey[] = "AcpClient/SessionPicker/CurrentGroupCollapsed";

static QString relativeTime(const QString &isoTimestamp)
{
    const QDateTime dt = QDateTime::fromString(isoTimestamp, Qt::ISODate).toLocalTime();
    if (!dt.isValid())
        return isoTimestamp;

    const qint64 secs = dt.secsTo(QDateTime::currentDateTime());
    if (secs < 60)
        return Tr::tr("just now");
    if (secs < 3600)
        return Tr::tr("%n minute(s) ago", nullptr, static_cast<int>(secs / 60));
    if (secs < 86400)
        return Tr::tr("%n hour(s) ago", nullptr, static_cast<int>(secs / 3600));
    if (secs < 7 * 86400)
        return Tr::tr("%n day(s) ago", nullptr, static_cast<int>(secs / 86400));
    return dt.toString(QStringLiteral("MMM d, yyyy"));
}

// ---------------------------------------------------------------------------
// PickerItem — one clickable row in the picker list.
//
// Always reserves a leading icon column (whether or not an icon is set) so that
// rows with and without an icon align their text.
// ---------------------------------------------------------------------------

class PickerItem : public QWidget
{
    Q_OBJECT

public:
    explicit PickerItem(const QString &text, const QIcon &icon = {}, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);

        m_layout = new QHBoxLayout(this);
        m_layout->setContentsMargins(12, 2, 12, 2);
        m_layout->setSpacing(8);

        const int iconSize = fontMetrics().height();
        auto *iconLabel = new QLabel(this);
        iconLabel->setFixedWidth(iconSize);
        iconLabel->setAlignment(Qt::AlignCenter);
        if (!icon.isNull())
            iconLabel->setPixmap(icon.pixmap(iconSize, iconSize));
        m_layout->addWidget(iconLabel);

        auto *textLabel = new Utils::ElidingLabel(text, this);
        textLabel->setElideMode(Qt::ElideRight);
        m_layout->addWidget(textLabel, 1);
    }

    // Dimmed, right-aligned secondary text (e.g. relative time or directory).
    // A non-eliding label keeps its natural width; an eliding one needs a
    // stretch factor to claim space next to the (stretching) main text.
    void setTrailingText(const QString &text,
                         Qt::TextElideMode elideMode = Qt::ElideNone,
                         int stretch = 0)
    {
        auto *label = new Utils::ElidingLabel(text, this);
        label->setElideMode(elideMode);
        QPalette pal = label->palette();
        pal.setColor(QPalette::WindowText, pal.color(QPalette::PlaceholderText));
        label->setPalette(pal);
        m_layout->addWidget(label, stretch, Qt::AlignRight);
    }

    void setDeleteButton()
    {
        auto *btn = new QToolButton(this);
        btn->setIcon(Utils::Icons::CLEAN_TOOLBAR.icon());
        btn->setAutoRaise(true);
        btn->setToolTip(Tr::tr("Delete Session"));
        const int sz = fontMetrics().height();
        btn->setFixedSize(sz + 4, sz + 4);
        connect(btn, &QToolButton::clicked, this, &PickerItem::deleteRequested);
        m_layout->addWidget(btn);
    }

signals:
    void clicked();
    void deleteRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
            emit clicked();
        QWidget::mousePressEvent(event);
    }

    void paintEvent(QPaintEvent *) override
    {
        if (!underMouse())
            return;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        Utils::StyleHelper::drawCardBg(
            &p, rect(), Utils::creatorColor(Utils::Theme::Token_Foreground_Muted));
    }

    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::HoverEnter || e->type() == QEvent::HoverLeave)
            update();
        return QWidget::event(e);
    }

private:
    QHBoxLayout *m_layout = nullptr;
};

// ---------------------------------------------------------------------------
// SessionPickerWidget
// ---------------------------------------------------------------------------

static QLabel *createGroupHeaderLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    QFont f = label->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 0.85);
    label->setFont(f);
    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, pal.color(QPalette::PlaceholderText));
    label->setPalette(pal);
    return label;
}

static QFrame *createSeparator(QWidget *parent)
{
    auto *sep = new QFrame(parent);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->hide();
    return sep;
}

SessionPickerWidget::SessionPickerWidget(QWidget *parent)
    : CollapsibleFrame(parent)
{
    setFrameShape(QFrame::NoFrame);
    setCollapsible(false);
    setHeaderVisible(false);

    m_currentGroupContainer = new QVBoxLayout;
    m_currentGroupContainer->setContentsMargins(0, 0, 0, 0);
    m_currentGroupContainer->setSpacing(0);
    m_bodyLayout->addLayout(m_currentGroupContainer);

    m_middleSeparator = createSeparator(this);
    m_bodyLayout->addWidget(m_middleSeparator);

    m_otherGroupsContainer = new QVBoxLayout;
    m_otherGroupsContainer->setContentsMargins(0, 0, 0, 0);
    m_otherGroupsContainer->setSpacing(0);
    m_bodyLayout->addLayout(m_otherGroupsContainer);

    m_emptyLabel = new QLabel(Tr::tr("No sessions available"), this);
    m_emptyLabel->setContentsMargins(12, 8, 12, 8);
    QPalette emptyPal = m_emptyLabel->palette();
    emptyPal.setColor(QPalette::WindowText, emptyPal.color(QPalette::PlaceholderText));
    m_emptyLabel->setPalette(emptyPal);
    m_emptyLabel->hide();
    m_bodyLayout->addWidget(m_emptyLabel);

    m_loadMoreButton = new QPushButton(Tr::tr("Load More..."), this);
    m_loadMoreButton->setFlat(true);
    m_loadMoreButton->hide();
    connect(m_loadMoreButton, &QPushButton::clicked, this, [this] {
        m_loadMoreButton->setEnabled(false);
        emit loadMoreRequested(m_nextCursor);
    });
    m_bodyLayout->addWidget(m_loadMoreButton);

    // The "New Session in Directory..." entry sits at the very bottom, below
    // all project groups.
    m_bottomSeparator = createSeparator(this);
    m_bodyLayout->addWidget(m_bottomSeparator);

    m_newSessionContainer = new QVBoxLayout;
    m_newSessionContainer->setContentsMargins(0, 0, 0, 0);
    m_newSessionContainer->setSpacing(0);
    m_bodyLayout->addLayout(m_newSessionContainer);
}

void SessionPickerWidget::setCurrentProjectDir(const Utils::FilePath &dir)
{
    m_currentProjectDir = dir;
    m_currentGroupKey = dir.isEmpty() ? QString() : dir.toUserOutput();
}

void SessionPickerWidget::setCanDeleteSessions(bool canDelete)
{
    m_canDeleteSessions = canDelete;
}

void SessionPickerWidget::removeSession(const QString &sessionId)
{
    QWidget *item = m_sessionItems.take(sessionId);
    if (!item)
        return;
    if (QWidget *pw = item->parentWidget())
        if (QLayout *l = pw->layout())
            l->removeWidget(item);
    delete item;
    updateEmptyState();
}

void SessionPickerWidget::setNewSessionTargets(const QList<NewSessionTarget> &targets)
{
    m_newSessionTargets = targets;

    while (QLayoutItem *item = m_newSessionContainer->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    // Each project is represented by its own group (with a "New Session" entry);
    // the top container only offers picking an arbitrary directory.
    auto *customItem = new PickerItem(
        Tr::tr("New Session in Directory..."), Utils::Icons::OPENFILE_TOOLBAR.icon(), this);
    connect(customItem, &PickerItem::clicked,
            this, &SessionPickerWidget::requestCustomDirectorySession);
    m_newSessionContainer->addWidget(customItem);
}

void SessionPickerWidget::requestNewSession(const Utils::FilePath &cwd)
{
    if (m_resolved)
        return;
    m_resolved = true;
    setEnabled(false);
    emit newSessionRequested(cwd);
    deleteLater();
}

void SessionPickerWidget::requestCustomDirectorySession()
{
    if (m_resolved)
        return;

    const Utils::FilePath dir = Utils::FileUtils::getExistingDirectory(
        Tr::tr("Choose Working Directory"), m_currentProjectDir);
    if (dir.isEmpty())
        return;

    requestNewSession(dir);
}

void SessionPickerWidget::setInitialSessions(const QList<SessionInfo> &sessions,
                                             const std::optional<QString> &nextCursor)
{
    clearGroups();

    // Always show a group per project, even without prior sessions, so each
    // offers a "New Session" entry.
    ensureProjectGroups();

    for (const SessionInfo &session : sessions)
        addSessionItem(session);

    updateCollapseState();
    updateEmptyState();

    m_nextCursor = nextCursor.value_or(QString{});
    updateLoadMoreButton();
}

void SessionPickerWidget::appendSessions(const QList<SessionInfo> &sessions,
                                         const std::optional<QString> &nextCursor)
{
    for (const SessionInfo &session : sessions)
        addSessionItem(session);

    updateEmptyState();

    m_nextCursor = nextCursor.value_or(QString{});
    m_loadMoreButton->setEnabled(true);
    updateLoadMoreButton();
}

void SessionPickerWidget::clearGroups()
{
    const auto clearContainer = [](QVBoxLayout *layout) {
        while (QLayoutItem *item = layout->takeAt(0)) {
            delete item->widget();
            delete item;
        }
    };
    clearContainer(m_currentGroupContainer);
    clearContainer(m_otherGroupsContainer);
    m_groups.clear();
    m_emptyLabel->hide();
    m_bottomSeparator->hide();
    m_middleSeparator->hide();
}

void SessionPickerWidget::updateEmptyState()
{
    const bool hasCurrent = !m_currentGroupKey.isEmpty() && m_groups.contains(m_currentGroupKey);
    const bool hasOthers = m_groups.size() > (hasCurrent ? 1 : 0);
    m_emptyLabel->setVisible(m_groups.isEmpty());
    m_bottomSeparator->setVisible(hasCurrent || hasOthers);
    m_middleSeparator->setVisible(hasCurrent && hasOthers);
}

void SessionPickerWidget::updateCollapseState()
{
    for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
        if (it.key() != m_currentGroupKey || m_currentGroupKey.isEmpty())
            it.value().frame->setCollapsed(true);
    }
}

void SessionPickerWidget::ensureProjectGroups()
{
    // Current project first so it lands in the "current" container.
    if (!m_currentGroupKey.isEmpty())
        ensureGroup(m_currentGroupKey);

    for (const NewSessionTarget &t : std::as_const(m_newSessionTargets)) {
        if (!t.cwd.isEmpty())
            ensureGroup(t.cwd.toUserOutput());
    }
}

SessionPickerWidget::Group &SessionPickerWidget::ensureGroup(const QString &cwd)
{
    auto it = m_groups.find(cwd);
    if (it != m_groups.end())
        return it.value();

    const bool isCurrent = !m_currentGroupKey.isEmpty() && cwd == m_currentGroupKey;
    QString headerText;
    if (isCurrent)
        headerText = Tr::tr("Current Project (%1)").arg(cwd);
    else if (cwd.isEmpty())
        headerText = Tr::tr("(No Working Directory)");
    else
        headerText = cwd;

    Group group;
    group.cwd = isCurrent ? m_currentProjectDir : Utils::FilePath::fromUserInput(cwd);
    group.frame = new CollapsibleFrame(this);
    group.frame->setFrameShape(QFrame::NoFrame);
    group.frame->headerLayout()->addWidget(createGroupHeaderLabel(headerText, group.frame), 1);
    group.layout = group.frame->bodyLayout();
    group.layout->setContentsMargins(0, 0, 0, 0);
    group.layout->setSpacing(0);

    // Let the user start a new session in this preexisting workspace.
    auto *newSessionItem = new PickerItem(
        Tr::tr("New Session in %1").arg(group.cwd.fileName()),
        Utils::Icons::PLUS_TOOLBAR.icon(),
        group.frame);
    const Utils::FilePath cwdPath = group.cwd;
    connect(newSessionItem, &PickerItem::clicked, this, [this, cwdPath] {
        requestNewSession(cwdPath);
    });
    group.layout->addWidget(newSessionItem);

    if (isCurrent) {
        const bool collapsed
            = Core::ICore::settings()->value(kCurrentGroupCollapsedKey, false).toBool();
        group.frame->setCollapsed(collapsed);
        connect(group.frame, &CollapsibleFrame::collapsedChanged, this, [](bool c) {
            Core::ICore::settings()->setValue(kCurrentGroupCollapsedKey, c);
        });
    } else {
        group.frame->setCollapsed(true);
    }

    QVBoxLayout *container = isCurrent ? m_currentGroupContainer : m_otherGroupsContainer;
    container->addWidget(group.frame);

    return m_groups.insert(cwd, group).value();
}

void SessionPickerWidget::setResolved(const QString &)
{
    if (m_resolved)
        return;
    m_resolved = true;

    setEnabled(false);
    deleteLater();
}

void SessionPickerWidget::addSessionItem(const SessionInfo &session)
{
    const QString title = session.title().value_or(session.sessionId());
    auto *item = new PickerItem(title, {}, this);
    if (session.updatedAt().has_value())
        item->setTrailingText(relativeTime(*session.updatedAt()));

    const QString sessionId = session.sessionId();
    m_sessionItems.insert(sessionId, item);
    const Utils::FilePath cwd = Utils::FilePath::fromUserInput(session.cwd());
    connect(item, &PickerItem::clicked, this, [this, sessionId, cwd] {
        if (m_resolved)
            return;
        emit sessionSelected(sessionId, cwd);
    });
    if (m_canDeleteSessions) {
        item->setDeleteButton();
        connect(item, &PickerItem::deleteRequested, this, [this, sessionId] {
            emit deleteSessionRequested(sessionId);
        });
    }

    // Canonicalize the cwd so sessions land in the matching project group
    // (m_currentGroupKey is the current project's canonical user-output path).
    const Utils::FilePath cwdPath = Utils::FilePath::fromUserInput(session.cwd());
    const QString key = cwdPath.isEmpty() ? QString() : cwdPath.toUserOutput();
    Group &group = ensureGroup(key);
    group.layout->addWidget(item);
}

void SessionPickerWidget::updateLoadMoreButton()
{
    m_loadMoreButton->setVisible(!m_nextCursor.isEmpty());
}

} // namespace AcpClient::Internal

#include "sessionpickerwidget.moc"
