// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../iwizardfactory.h"
#include "newdialog.h"

#include <QCoreApplication>
#include <QDialog>
#include <QIcon>
#include <QList>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QListView;
class QModelIndex;
class QPushButton;
class QSortFilterProxyModel;
class QStandardItem;
class QStandardItemModel;
class QTextBrowser;
class QTreeView;
QT_END_NAMESPACE

namespace Core::Internal {

class NewDialogWidget : public QDialog, public NewDialog
{
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

    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_filterProxyModel;
    QComboBox *m_comboBox;
    QTreeView *m_templateCategoryView;
    QListView *m_templatesView;
    QLabel *m_imageLabel;
    QTextBrowser *m_templateDescription;
    QPushButton *m_okButton = nullptr;
    QList<QStandardItem *> m_categoryItems;
    Utils::FilePath m_defaultLocation;
    QVariantMap m_extraVariables;
};

} // Core::Internal
