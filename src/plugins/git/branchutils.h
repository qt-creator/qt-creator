/****************************************************************************
**
** Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
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

#include <QCoreApplication>
#include <QModelIndex>

QT_BEGIN_NAMESPACE
class QTreeView;
class QWidget;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

class BranchModel;

class BranchUtils
{
    Q_DECLARE_TR_FUNCTIONS(Git::Internal::BranchUtils)

public:
    explicit BranchUtils(QWidget *parent);
    QModelIndex selectedIndex();
    bool add();
    bool checkout();
    bool remove();
    bool rename();
    bool reset();
    bool isFastForwardMerge();
    bool merge(bool allowFastForward = false);
    void rebase();
    bool cherryPick();

protected:
    void setBranchView(QTreeView *branchView);
    BranchModel *m_model = nullptr;
    QString m_repository;

private:
    QWidget *m_widget = nullptr;
    QTreeView *m_branchView = nullptr;
};

} // namespace Internal
} // namespace Git
