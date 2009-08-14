/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef BRANCHDIALOG_H
#define BRANCHDIALOG_H

#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
class QModelIndex;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

namespace Ui {
class BranchDialog;
}

class GitClient;
class LocalBranchModel;
class RemoteBranchModel;

/**
 * Branch dialog. Displays a list of local branches at the top and remote
 * branches below. Offers to checkout/delete local branches.
 *
 * TODO: Add new branch (optionally tracking a remote one).
 * How to find out that a local branch is a tracking one?
 */
class BranchDialog : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(BranchDialog)
public:
    explicit BranchDialog(QWidget *parent = 0);

    bool init(GitClient *client, const QString &workingDirectory, QString *errorMessage);

    virtual ~BranchDialog();

protected:
    virtual void changeEvent(QEvent *e);

private slots:
    void slotEnableButtons();
    void slotCheckoutSelectedBranch();
    void slotDeleteSelectedBranch();
    void slotLocalBranchActivated();
    void slotRemoteBranchActivated(const QModelIndex &);
    void slotCreateLocalBranch(const QString &branchName);

private:
    bool ask(const QString &title, const QString &what, bool defaultButton);
    void selectLocalBranch(const QString &b);

    int selectedLocalBranchIndex() const;
    int selectedRemoteBranchIndex() const;

    GitClient *m_client;
    Ui::BranchDialog *m_ui;
    QPushButton *m_checkoutButton;
    QPushButton *m_deleteButton;

    LocalBranchModel *m_localModel;
    RemoteBranchModel *m_remoteModel;
    QString m_repoDirectory;
};

} // namespace Internal
} // namespace Git

#endif // BRANCHDIALOG_H
