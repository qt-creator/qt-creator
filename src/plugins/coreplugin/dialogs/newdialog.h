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

#include "../iwizardfactory.h"

#include <QDialog>
#include <QIcon>
#include <QList>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QAbstractProxyModel;
class QModelIndex;
class QSortFilterProxyModel;
class QPushButton;
class QStandardItem;
class QStandardItemModel;
class QStringList;
QT_END_NAMESPACE

namespace Core {

namespace Internal {

namespace Ui { class NewDialog; }

class NewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewDialog(QWidget *parent);
    ~NewDialog();

    void setWizardFactories(QList<IWizardFactory*> factories, const QString &defaultLocation, const QVariantMap &extraVariables);

    void showDialog();
    Id selectedPlatform() const;

    static QWidget *currentDialog();

protected:
    bool event(QEvent *);

private:
    void currentCategoryChanged(const QModelIndex &);
    void currentItemChanged(const QModelIndex &);
    void accept();
    void reject();
    void updateOkButton();
    void setSelectedPlatform(const QString &platform);

    Core::IWizardFactory *currentWizardFactory() const;
    void addItem(QStandardItem *topLevelCategoryItem, IWizardFactory *factory);
    void saveState();

    static QWidget *m_currentDialog;

    Ui::NewDialog *m_ui;
    QStandardItemModel *m_model;
    QAbstractProxyModel *m_twoLevelProxyModel;
    QSortFilterProxyModel *m_filterProxyModel;
    QPushButton *m_okButton;
    QIcon m_dummyIcon;
    QList<QStandardItem*> m_categoryItems;
    QString m_defaultLocation;
    QVariantMap m_extraVariables;
};

} // namespace Internal
} // namespace Core
