// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sessionview.h"

#include "session.h"

#include <utils/algorithm.h>

#include <QHeaderView>
#include <QItemSelection>
#include <QStringList>
#include <QStyledItemDelegate>

namespace Core {
namespace Internal {

// custom item delegate class
class RemoveItemFocusDelegate : public QStyledItemDelegate
{
public:
    RemoveItemFocusDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {
    }

protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

void RemoveItemFocusDelegate::paint(QPainter* painter, const QStyleOptionViewItem & option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    opt.state &= ~QStyle::State_HasFocus;
    QStyledItemDelegate::paint(painter, opt, index);
}

SessionView::SessionView(QWidget *parent)
    : Utils::TreeView(parent)
{
    setUniformRowHeights(false);
    setItemDelegate(new RemoveItemFocusDelegate(this));
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setWordWrap(false);
    setRootIsDecorated(false);
    setSortingEnabled(true);

    setModel(&m_sessionModel);
    sortByColumn(0, Qt::AscendingOrder);

    // Ensure that the full session name is visible.
    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    QItemSelection firstRow(m_sessionModel.index(0,0), m_sessionModel.index(
        0, m_sessionModel.columnCount() - 1));
    selectionModel()->select(firstRow, QItemSelectionModel::QItemSelectionModel::
        SelectCurrent);

    connect(this, &Utils::TreeView::activated, this, [this](const QModelIndex &index){
        emit sessionActivated(m_sessionModel.sessionAt(index.row()));
    });
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] {
        emit sessionsSelected(selectedSessions());
    });

    connect(&m_sessionModel, &SessionModel::sessionSwitched,
        this, &SessionView::sessionSwitched);
    connect(&m_sessionModel, &SessionModel::modelReset,
        this, &SessionView::selectActiveSession);
    connect(&m_sessionModel, &SessionModel::sessionCreated,
        this, &SessionView::selectSession);
 }

void SessionView::createNewSession()
{
    m_sessionModel.newSession(this);
}

void SessionView::deleteSelectedSessions()
{
    deleteSessions(selectedSessions());
}

void SessionView::deleteSessions(const QStringList &sessions)
{
    m_sessionModel.deleteSessions(sessions);
}

void SessionView::cloneCurrentSession()
{
    m_sessionModel.cloneSession(this, currentSession());
}

void SessionView::renameCurrentSession()
{
    m_sessionModel.renameSession(this, currentSession());
}

void SessionView::switchToCurrentSession()
{
    m_sessionModel.switchToSession(currentSession());
}

QString SessionView::currentSession()
{
    return m_sessionModel.sessionAt(selectionModel()->currentIndex().row());
}

SessionModel *SessionView::sessionModel()
{
    return &m_sessionModel;
}

void SessionView::selectActiveSession()
{
    selectSession(SessionManager::activeSession());
}

void SessionView::selectSession(const QString &sessionName)
{
    int row = m_sessionModel.indexOfSession(sessionName);
    selectionModel()->setCurrentIndex(model()->index(row, 0),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void SessionView::showEvent(QShowEvent *event)
{
    Utils::TreeView::showEvent(event);
    selectActiveSession();
    setFocus();
}

void SessionView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() != Qt::Key_Delete && event->key() != Qt::Key_Backspace) {
        TreeView::keyPressEvent(event);
        return;
    }
    const QStringList sessions = selectedSessions();
    if (!sessions.contains("default") && !Utils::anyOf(sessions,
            [](const QString &session) { return session == SessionManager::activeSession(); })) {
        deleteSessions(sessions);
    }
}

QStringList SessionView::selectedSessions() const
{
    return Utils::transform(selectionModel()->selectedRows(), [this](const QModelIndex &index) {
        return m_sessionModel.sessionAt(index.row());
    });
}

} // namespace Internal
} // namespace Core
