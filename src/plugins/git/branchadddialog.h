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
#include <QItemDelegate>

namespace Git {
namespace Internal {

class BranchModel;

namespace Ui { class BranchAddDialog; }

class BranchValidationDelegate : public QItemDelegate
{
public:
    BranchValidationDelegate(QWidget *parent, BranchModel *model);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

private:
    BranchModel *m_model;
};

class BranchAddDialog : public QDialog
{
    Q_OBJECT

public:
    BranchAddDialog(const QStringList &localBranches, bool addBranch, QWidget *parent);
    ~BranchAddDialog() override;

    void setBranchName(const QString &);
    QString branchName() const;

    void setTrackedBranchName(const QString &name, bool remote);

    bool track() const;

    void setCheckoutVisible(bool visible);
    bool checkout() const;

private:
    void updateButtonStatus();

    Ui::BranchAddDialog *m_ui;
};

} // namespace Internal
} // namespace Git
