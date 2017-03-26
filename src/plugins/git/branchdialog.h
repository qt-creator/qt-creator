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
    ~BranchDialog() override;

    void refresh(const QString &repository, bool force);
    void refreshIfSame(const QString &repository);

private:
    void expandAndResize();
    void resizeColumns();
    void enableButtons();
    void refreshCurrentRepository();
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

    QModelIndex selectedIndex();

    Ui::BranchDialog *m_ui;
    BranchModel *m_model;
    QString m_repository;
};

} // namespace Internal
} // namespace Git
