// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sessionpickerwidget.h"
#include "acpclienttr.h"

#include <utils/elidinglabel.h>
#include <utils/fileutils.h>
#include <utils/utilsicons.h>

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Acp;

namespace AcpClient::Internal {

static QString relativeTime(const QString &isoTimestamp)
{
    const QDateTime dt = QDateTime::fromString(isoTimestamp, Qt::ISODate).toLocalTime();
    if (!dt.isValid())
        return isoTimestamp;

    const qint64 secs = dt.secsTo(QDateTime::currentDateTime());
    if (secs < 60)
        return QObject::tr("just now");
    if (secs < 3600)
        return QObject::tr("%n minute(s) ago", nullptr, static_cast<int>(secs / 60));
    if (secs < 86400)
        return QObject::tr("%n hour(s) ago", nullptr, static_cast<int>(secs / 3600));
    if (secs < 7 * 86400)
        return QObject::tr("%n day(s) ago", nullptr, static_cast<int>(secs / 86400));
    return dt.toString(QStringLiteral("MMM d, yyyy"));
}

// ---------------------------------------------------------------------------
// SessionItemWidget — one row in the picker list
// ---------------------------------------------------------------------------

class SessionItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SessionItemWidget(const SessionInfo &session, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_sessionId(session.sessionId())
        , m_cwd(Utils::FilePath::fromUserInput(session.cwd()))
    {
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 2, 12, 2);
        layout->setSpacing(8);

        const QString title = session.title().value_or(session.sessionId());
        auto *titleLabel = new Utils::ElidingLabel(title, this);
        titleLabel->setElideMode(Qt::ElideRight);
        layout->addWidget(titleLabel, 1);

        if (session.updatedAt().has_value()) {
            auto *timeLabel = new QLabel(relativeTime(*session.updatedAt()), this);
            QPalette pal = timeLabel->palette();
            pal.setColor(QPalette::WindowText, pal.color(QPalette::PlaceholderText));
            timeLabel->setPalette(pal);
            layout->addWidget(timeLabel, 0, Qt::AlignRight);
        }
    }

signals:
    void clicked(const QString &sessionId, const Utils::FilePath &cwd);

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
            emit clicked(m_sessionId, m_cwd);
        QWidget::mousePressEvent(event);
    }

    void paintEvent(QPaintEvent *) override
    {
        if (underMouse()) {
            QPainter p(this);
            p.fillRect(rect(), palette().color(QPalette::Highlight).lighter(170));
        }
    }

    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::HoverEnter || e->type() == QEvent::HoverLeave)
            update();
        return QWidget::event(e);
    }

private:
    QString m_sessionId;
    Utils::FilePath m_cwd;
};

// ---------------------------------------------------------------------------
// NewSessionItemWidget — clickable "New Session" row styled like SessionItemWidget
// ---------------------------------------------------------------------------

class NewSessionItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NewSessionItemWidget(const QString &label,
                                  const QIcon &icon,
                                  const QString &subtitle,
                                  QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 2, 12, 2);
        layout->setSpacing(8);

        auto *iconLabel = new QLabel(this);
        const int size = fontMetrics().height();
        iconLabel->setPixmap(icon.pixmap(size, size));
        layout->addWidget(iconLabel);

        auto *textLabel = new QLabel(label, this);
        layout->addWidget(textLabel);

        if (!subtitle.isEmpty()) {
            auto *subtitleLabel = new Utils::ElidingLabel(subtitle, this);
            subtitleLabel->setElideMode(Qt::ElideMiddle);
            QPalette pal = subtitleLabel->palette();
            pal.setColor(QPalette::WindowText, pal.color(QPalette::PlaceholderText));
            subtitleLabel->setPalette(pal);
            layout->addWidget(subtitleLabel, 1);
        } else {
            layout->addStretch(1);
        }
    }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
            emit clicked();
        QWidget::mousePressEvent(event);
    }

    void paintEvent(QPaintEvent *) override
    {
        if (underMouse()) {
            QPainter p(this);
            p.fillRect(rect(), palette().color(QPalette::Highlight).lighter(170));
        }
    }

    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::HoverEnter || e->type() == QEvent::HoverLeave)
            update();
        return QWidget::event(e);
    }
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

    m_newSessionContainer = new QVBoxLayout;
    m_newSessionContainer->setContentsMargins(0, 0, 0, 0);
    m_newSessionContainer->setSpacing(0);
    m_bodyLayout->addLayout(m_newSessionContainer);

    m_topSeparator = createSeparator(this);
    m_bodyLayout->addWidget(m_topSeparator);

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

    m_loadMoreButton = new QPushButton(Tr::tr("Load more..."), this);
    m_loadMoreButton->setFlat(true);
    m_loadMoreButton->hide();
    connect(m_loadMoreButton, &QPushButton::clicked, this, [this] {
        m_loadMoreButton->setEnabled(false);
        emit loadMoreRequested(m_nextCursor);
    });
    m_bodyLayout->addWidget(m_loadMoreButton);
}

void SessionPickerWidget::setCurrentProjectDir(const Utils::FilePath &dir)
{
    m_currentProjectDir = dir;
    m_currentGroupKey = dir.isEmpty() ? QString() : dir.toUserOutput();
}

void SessionPickerWidget::setNewSessionTargets(const QList<NewSessionTarget> &targets)
{
    while (QLayoutItem *item = m_newSessionContainer->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const QIcon plusIcon = Utils::Icons::PLUS_TOOLBAR.icon();
    for (const NewSessionTarget &t : targets) {
        const QString label = Tr::tr("New Session for %1").arg(t.label);
        auto *item = new NewSessionItemWidget(
            label, plusIcon, t.tooltip.isEmpty() ? t.cwd.toUserOutput() : t.tooltip, this);
        if (!t.tooltip.isEmpty())
            item->setToolTip(t.tooltip);
        const Utils::FilePath cwd = t.cwd;
        connect(item, &NewSessionItemWidget::clicked, this, [this, cwd] {
            requestNewSession(cwd);
        });
        m_newSessionContainer->addWidget(item);
    }

    auto *customItem = new NewSessionItemWidget(
        Tr::tr("New Session in Directory..."),
        Utils::Icons::OPENFILE_TOOLBAR.icon(),
        QString(),
        this);
    connect(customItem, &NewSessionItemWidget::clicked,
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
    m_topSeparator->hide();
    m_middleSeparator->hide();
}

void SessionPickerWidget::updateEmptyState()
{
    const bool hasCurrent = !m_currentGroupKey.isEmpty() && m_groups.contains(m_currentGroupKey);
    const bool hasOthers = m_groups.size() > (hasCurrent ? 1 : 0);
    m_emptyLabel->setVisible(m_groups.isEmpty());
    m_topSeparator->setVisible(hasCurrent || hasOthers);
    m_middleSeparator->setVisible(hasCurrent && hasOthers);
}

void SessionPickerWidget::updateCollapseState()
{
    for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
        if (it.key() != m_currentGroupKey || m_currentGroupKey.isEmpty())
            it.value().frame->setCollapsed(true);
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
        headerText = Tr::tr("(No working directory)");
    else
        headerText = cwd;

    Group group;
    group.frame = new CollapsibleFrame(this);
    group.frame->setFrameShape(QFrame::NoFrame);
    group.frame->headerLayout()->addWidget(createGroupHeaderLabel(headerText, group.frame), 1);
    group.layout = group.frame->bodyLayout();
    group.layout->setContentsMargins(0, 0, 0, 0);
    group.layout->setSpacing(0);

    if (!isCurrent)
        group.frame->setCollapsed(true);

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
    auto *item = new SessionItemWidget(session, this);
    connect(item, &SessionItemWidget::clicked, this,
            [this](const QString &sessionId, const Utils::FilePath &cwd) {
        if (m_resolved)
            return;
        emit sessionSelected(sessionId, cwd);
    });

    QString key = session.cwd();
    if (!key.isEmpty() && !m_currentProjectDir.isEmpty()
        && Utils::FilePath::fromUserInput(key) == m_currentProjectDir) {
        key = m_currentGroupKey;
    }
    Group &group = ensureGroup(key);
    group.layout->addWidget(item);
}

void SessionPickerWidget::updateLoadMoreButton()
{
    m_loadMoreButton->setVisible(!m_nextCursor.isEmpty());
}

} // namespace AcpClient::Internal

#include "sessionpickerwidget.moc"
