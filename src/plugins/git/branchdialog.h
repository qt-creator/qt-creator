/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BRANCHDIALOG_H
#define BRANCHDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

namespace Ui { class BranchDialog; }

class BranchAddDialog;
class BranchModel;

/**
 * Branch dialog. Displays a list of local branches at the top and remote
 * branches below. Offers to checkout/delete local branches.
 *
 */
class BranchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BranchDialog(QWidget *parent = 0);
    ~BranchDialog();

public slots:
    void refresh(const QString &repository, bool force);
    void refreshIfSame(const QString &repository);

private slots:
    void enableButtons();
    void refresh();
    void add();
    void checkout();
    void remove();
    void rename();
    void diff();
    void log();
    void reset();
    void merge();
    void rebase();
    void cherryPick();
    void setRemoteTracking();

private:
    QModelIndex selectedIndex();

    Ui::BranchDialog *m_ui;
    BranchModel *m_model;
    QString m_repository;
};

} // namespace Internal
} // namespace Git

#endif // BRANCHDIALOG_H
