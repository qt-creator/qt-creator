/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "../iwizardfactory.h"
#include "newdialog.h"

#include <QDialog>
#include <QIcon>
#include <QList>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QSortFilterProxyModel;
class QPushButton;
class QStandardItem;
class QStandardItemModel;
QT_END_NAMESPACE

namespace Core {

namespace Internal {

namespace Ui { class NewDialog; }

class NewDialogWidget : public QDialog, public NewDialog
{
    Q_OBJECT

public:
    explicit NewDialogWidget(QWidget *parent);
    ~NewDialogWidget() override;

    void setWizardFactories(QList<IWizardFactory *> factories,
                            const Utils::FilePath &defaultLocation,
                            const QVariantMap &extraVariables) override;

    void showDialog() override;
    Utils::Id selectedPlatform() const;
    QWidget *widget() override { return this; }

    void setWindowTitle(const QString &title) override {
        QDialog::setWindowTitle(title);
    }

protected:
    bool event(QEvent *) override;

private:
    void currentCategoryChanged(const QModelIndex &);
    void currentItemChanged(const QModelIndex &);
    void accept() override;
    void reject() override;
    void updateOkButton();
    void setSelectedPlatform(int index);

    Core::IWizardFactory *currentWizardFactory() const;
    void addItem(QStandardItem *topLevelCategoryItem, IWizardFactory *factory);
    void saveState();

    Ui::NewDialog *m_ui;
    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_filterProxyModel;
    QPushButton *m_okButton = nullptr;
    QIcon m_dummyIcon;
    QList<QStandardItem *> m_categoryItems;
    Utils::FilePath m_defaultLocation;
    QVariantMap m_extraVariables;
};

} // namespace Internal
} // namespace Core
