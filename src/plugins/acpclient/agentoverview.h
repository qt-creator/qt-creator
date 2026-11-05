// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractButton>
#include <QIcon>
#include <QList>
#include <QWidget>

#include <memory>

class QButtonGroup;
class QScrollArea;
class QVBoxLayout;

namespace Utils {
class InfoLabel;
class ProgressIndicatorPainter;
class QtcIconButton;
} // namespace Utils

namespace AcpClient::Internal {

enum class SessionActivity { None, Idle, Running, InputRequired };

// The server icon stands for a session that is idle or not there, the running
// icon for one that is running.
QIcon sessionActivityIcon(SessionActivity activity, const QIcon &serverIcon = {},
                          const QIcon &runningIcon = {});
QString sessionActivityText(SessionActivity activity);

class RunningIndicator : public QObject
{
    Q_OBJECT

public:
    explicit RunningIndicator(QObject *parent = nullptr);
    ~RunningIndicator() override;

    void setActive(bool active);
    bool isActive() const { return m_active; }
    QIcon icon() const { return m_icon; }

signals:
    void iconChanged();

private:
    void renderFrame();

    std::unique_ptr<Utils::ProgressIndicatorPainter> m_painter;
    QIcon m_icon;
    bool m_active = false;
};

struct SessionEntry
{
    QString agentName;
    QString path;
    SessionActivity activity = SessionActivity::None;
    QIcon serverIcon;
};

// "Agent (State) - path", for accessible names.
QString sessionEntryText(const SessionEntry &entry);
// The path and the state, one per line.
QString sessionEntryToolTip(const SessionEntry &entry);

class SessionSwitcherButton final : public QAbstractButton
{
    Q_OBJECT

public:
    explicit SessionSwitcherButton(QWidget *parent = nullptr);

    void setEntry(const SessionEntry &entry, const QIcon &runningIcon);
    const SessionEntry &entry() const { return m_entry; }

    QSize sizeHint() const final;
    QSize minimumSizeHint() const final;

protected:
    void paintEvent(QPaintEvent *event) final;

private:
    SessionEntry m_entry;
};

class AgentOverview : public QWidget
{
    Q_OBJECT

public:
    using Entry = SessionEntry;

    explicit AgentOverview(QWidget *parent = nullptr);

    void setEntries(const QList<Entry> &entries);
    void setRunningIcon(const QIcon &icon);
    void setCurrentEntry(int index);
    void focusCurrentEntry();

    QSize sizeHint() const override;

    QList<QAbstractButton *> entryButtons() const { return m_buttons; }
    Utils::InfoLabel *emptyLabel() const { return m_emptyLabel; }

signals:
    void entryActivated(int index);
    void entryCloseRequested(int index);
    void dismissRequested();

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QScrollArea *m_scrollArea = nullptr;
    QVBoxLayout *m_buttonsLayout = nullptr;
    QButtonGroup *m_buttonGroup = nullptr;
    QList<QAbstractButton *> m_buttons;
    QIcon m_runningIcon;
    Utils::InfoLabel *m_emptyLabel = nullptr;
};

} // namespace AcpClient::Internal
