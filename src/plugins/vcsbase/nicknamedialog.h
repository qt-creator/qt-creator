// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QModelIndex;
class QPushButton;
class QSortFilterProxyModel;
class QStandardItemModel;
QT_END_NAMESPACE

namespace Utils {
class FilePath;
class TreeView;
}

namespace VcsBase::Internal {

class NickNameDialog : public QDialog
{
public:
    explicit NickNameDialog(QStandardItemModel *model, QWidget *parent = nullptr);
    ~NickNameDialog() override;

    QString nickName() const;

    // Utilities to initialize/populate the model
    static QStandardItemModel *createModel(QObject *parent);
    static bool populateModelFromMailCapFile(const Utils::FilePath &file,
                                             QStandardItemModel *model,
                                             QString *errorMessage);

    // Return a list for a completer on the field line edits
    static QStringList nickNameList(const QStandardItemModel *model);

private:
    void slotCurrentItemChanged(const QModelIndex &);
    void slotActivated(const QModelIndex &);

    QPushButton *okButton() const;

    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_filterModel;

    Utils::TreeView *m_filterTreeView;
    QDialogButtonBox *m_buttonBox;
};

} // VcsBase::Internal
