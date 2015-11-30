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

#ifndef NEWDIALOG_H
#define NEWDIALOG_H

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
    QString selectedPlatform() const;

    static bool isRunning();

protected:
    bool event(QEvent *);

private slots:
    void currentCategoryChanged(const QModelIndex &);
    void currentItemChanged(const QModelIndex &);
    void accept();
    void reject();
    void updateOkButton();
    void setSelectedPlatform(const QString &platform);

private:
    Core::IWizardFactory *currentWizardFactory() const;
    void addItem(QStandardItem *topLevelCategoryItem, IWizardFactory *factory);
    void saveState();

    static bool m_isRunning;

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

#endif // NEWDIALOG_H
