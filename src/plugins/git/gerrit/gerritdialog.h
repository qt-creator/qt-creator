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

#ifndef GERRIT_INTERNAL_GERRITDIALOG_H
#define GERRIT_INTERNAL_GERRITDIALOG_H

#include <utils/pathchooser.h>

#include <QDialog>
#include <QSharedPointer>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QTreeView;
class QLabel;
class QModelIndex;
class QSortFilterProxyModel;
class QStandardItem;
class QStringListModel;
class QPushButton;
class QDialogButtonBox;
class QTextBrowser;
QT_END_NAMESPACE

namespace Utils {
class FancyLineEdit;
class ProgressIndicator;
class TreeView;
}

namespace Gerrit {
namespace Internal {
class GerritParameters;
class GerritModel;
class GerritChange;
class QueryValidatingLineEdit;

class GerritDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GerritDialog(const QSharedPointer<GerritParameters> &p,
                          QWidget *parent = 0);
    ~GerritDialog();
    QString repositoryPath() const;
    void setCurrentPath(const QString &path);

signals:
    void fetchDisplay(const QSharedPointer<Gerrit::Internal::GerritChange> &);
    void fetchCherryPick(const QSharedPointer<Gerrit::Internal::GerritChange> &);
    void fetchCheckout(const QSharedPointer<Gerrit::Internal::GerritChange> &);

public slots:
    void fetchStarted(const QSharedPointer<Gerrit::Internal::GerritChange> &change);
    void fetchFinished();

private:
    void slotCurrentChanged();
    void slotActivated(const QModelIndex &);
    void slotRefreshStateChanged(bool);
    void slotFetchDisplay();
    void slotFetchCherryPick();
    void slotFetchCheckout();
    void slotRefresh();

    void manageProgressIndicator();

    void setProgressIndicatorVisible(bool v);
    QModelIndex currentIndex() const;
    QPushButton *addActionButton(const QString &text, const std::function<void()> &buttonSlot);
    void updateCompletions(const QString &query);
    void updateButtons();

    const QSharedPointer<GerritParameters> m_parameters;
    QSortFilterProxyModel *m_filterModel;
    GerritModel *m_model;
    QStringListModel *m_queryModel;
    Utils::TreeView *m_treeView;
    QTextBrowser *m_detailsBrowser;
    Utils::FancyLineEdit *m_queryLineEdit;
    Utils::FancyLineEdit *m_filterLineEdit;
    Utils::PathChooser *m_repositoryChooser;
    QDialogButtonBox *m_buttonBox;
    QPushButton *m_displayButton;
    QPushButton *m_cherryPickButton;
    QPushButton *m_checkoutButton;
    QPushButton *m_refreshButton;
    QLabel *m_repositoryChooserLabel;
    Utils::ProgressIndicator *m_progressIndicator;
    QTimer m_progressIndicatorTimer;
    bool m_fetchRunning;
};

} // namespace Internal
} // namespace Gerrit

#endif // GERRIT_INTERNAL_GERRITDIALOG_H
