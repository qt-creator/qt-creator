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

#pragma once

#include <QDialog>
#include <QSharedPointer>
#include <QTimer>

#include <functional>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QSortFilterProxyModel;
class QStringListModel;
class QPushButton;
QT_END_NAMESPACE

namespace Utils { class ProgressIndicator; }

namespace Gerrit {
namespace Internal {
namespace Ui { class GerritDialog; }
class GerritChange;
class GerritModel;
class GerritParameters;
class GerritServer;

class GerritDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GerritDialog(const QSharedPointer<GerritParameters> &p,
                          const QSharedPointer<GerritServer> &s,
                          const QString &repository,
                          QWidget *parent = nullptr);
    ~GerritDialog();
    QString repositoryPath() const;
    void setCurrentPath(const QString &path);
    void fetchStarted(const QSharedPointer<Gerrit::Internal::GerritChange> &change);
    void fetchFinished();

signals:
    void fetchDisplay(const QSharedPointer<Gerrit::Internal::GerritChange> &);
    void fetchCherryPick(const QSharedPointer<Gerrit::Internal::GerritChange> &);
    void fetchCheckout(const QSharedPointer<Gerrit::Internal::GerritChange> &);

private:
    void slotCurrentChanged();
    void slotActivated(const QModelIndex &);
    void slotRefreshStateChanged(bool);
    void slotFetchDisplay();
    void slotFetchCherryPick();
    void slotFetchCheckout();
    void slotRefresh();
    void remoteChanged();
    void updateRemotes(bool forceReload = false);
    void addRemote(const GerritServer &server, const QString &name);

    void manageProgressIndicator();

    void setProgressIndicatorVisible(bool v);
    QModelIndex currentIndex() const;
    QPushButton *addActionButton(const QString &text, const std::function<void()> &buttonSlot);
    void updateCompletions(const QString &query);
    void updateButtons();

    const QSharedPointer<GerritParameters> m_parameters;
    const QSharedPointer<GerritServer> m_server;
    QSortFilterProxyModel *m_filterModel;
    Ui::GerritDialog *m_ui;
    GerritModel *m_model;
    QStringListModel *m_queryModel;
    QPushButton *m_displayButton;
    QPushButton *m_cherryPickButton;
    QPushButton *m_checkoutButton;
    QPushButton *m_refreshButton;
    Utils::ProgressIndicator *m_progressIndicator;
    QTimer m_progressIndicatorTimer;
    QString m_repository;
    bool m_fetchRunning = false;
    bool m_updatingRemotes = false;
};

} // namespace Internal
} // namespace Gerrit
