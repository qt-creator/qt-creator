// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QDialog>
#include <QTimer>

#include <functional>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QLabel;
class QModelIndex;
class QSortFilterProxyModel;
class QStringListModel;
class QPushButton;
class QTextBrowser;
QT_END_NAMESPACE

namespace Utils {
class FancyLineEdit;
class ProgressIndicator;
class TreeView;
} // Utils

namespace Gerrit::Internal {

class GerritChange;
class GerritModel;
class GerritRemoteChooser;
class GerritServer;

class GerritDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GerritDialog(const std::shared_ptr<GerritServer> &s,
                          const Utils::FilePath &repository,
                          QWidget *parent = nullptr);
    ~GerritDialog() override;
    Utils::FilePath repositoryPath() const;
    void setCurrentPath(const Utils::FilePath &path);
    void fetchStarted(const std::shared_ptr<Gerrit::Internal::GerritChange> &change);
    void fetchFinished();
    void refresh();
    void scheduleUpdateRemotes();

signals:
    void fetchDisplay(const std::shared_ptr<Gerrit::Internal::GerritChange> &);
    void fetchCherryPick(const std::shared_ptr<Gerrit::Internal::GerritChange> &);
    void fetchCheckout(const std::shared_ptr<Gerrit::Internal::GerritChange> &);

private:
    void slotCurrentChanged();
    void slotActivated(const QModelIndex &);
    void slotRefreshStateChanged(bool);
    void slotFetchDisplay();
    void slotFetchCherryPick();
    void slotFetchCheckout();
    void remoteChanged();
    void updateRemotes(bool forceReload = false);
    void showEvent(QShowEvent *event) override;

    void manageProgressIndicator();

    void setProgressIndicatorVisible(bool v);
    QModelIndex currentIndex() const;
    void updateCompletions(const QString &query);
    QPushButton *addActionButton(const QString &text, const std::function<void ()> &buttonSlot);
    void updateButtons();

    const std::shared_ptr<GerritServer> m_server;
    QSortFilterProxyModel *m_filterModel;
    GerritModel *m_model;
    QStringListModel *m_queryModel;
    QPushButton *m_displayButton;
    QPushButton *m_cherryPickButton;
    QPushButton *m_checkoutButton;
    QPushButton *m_refreshButton;
    Utils::ProgressIndicator *m_progressIndicator;
    QTimer m_progressIndicatorTimer;
    Utils::FilePath m_repository;
    bool m_fetchRunning = false;
    bool m_updatingRemotes = false;
    bool m_shouldUpdateRemotes = false;

    QLabel *m_repositoryLabel;
    GerritRemoteChooser *m_remoteComboBox;
    Utils::TreeView *m_treeView;
    QTextBrowser *m_detailsBrowser;
    QDialogButtonBox *m_buttonBox;
    Utils::FancyLineEdit *m_queryLineEdit;
};

} // Gerrit::Internal
