// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
class QStandardItemModel;
class QModelIndex;
class QPushButton;
QT_END_NAMESPACE

namespace VcsBase {
namespace Internal {

namespace Ui { class NickNameDialog; }

class NickNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NickNameDialog(QStandardItemModel *model, QWidget *parent = nullptr);
    ~NickNameDialog() override;

    QString nickName() const;

    // Utilities to initialize/populate the model
    static QStandardItemModel *createModel(QObject *parent);
    static bool populateModelFromMailCapFile(const QString &file,
                                             QStandardItemModel *model,
                                             QString *errorMessage);

    // Return a list for a completer on the field line edits
    static QStringList nickNameList(const QStandardItemModel *model);

private:
    void slotCurrentItemChanged(const QModelIndex &);
    void slotActivated(const QModelIndex &);

    QPushButton *okButton() const;

    Ui::NickNameDialog *m_ui;
    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_filterModel;
};

} // namespace Internal
} // namespace VcsBase
