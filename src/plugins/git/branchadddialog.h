// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>
#include <QItemDelegate>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QDialogButtonBox;
class QLineEdit;
QT_END_NAMESPACE

namespace Git::Internal {

class BranchModel;

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
public:
    enum Type {
        AddBranch,
        RenameBranch,
        AddTag,
        RenameTag
    };

    BranchAddDialog(const QStringList &localBranches, Type type, QWidget *parent);
    ~BranchAddDialog() override;

    void setBranchName(const QString &);
    QString branchName() const;

    void setTrackedBranchName(const QString &name, bool remote);

    bool track() const;

    void setCheckoutVisible(bool visible);
    bool checkout() const;

private:
    void updateButtonStatus();

    QLineEdit *m_branchNameEdit;
    QCheckBox *m_checkoutCheckBox;
    QCheckBox *m_trackingCheckBox;
    QDialogButtonBox *m_buttonBox;
};

} // Git::Internal
