/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "newdialog.h"
#include "ui_newdialog.h"
#include "basefilewizard.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/iwizard.h>

#include <QtGui/QHeaderView>
#include <QtGui/QPushButton>
#include <QtGui/QStandardItem>
#include <QtCore/QDebug>

Q_DECLARE_METATYPE(Core::IWizard*)

static inline Core::IWizard *wizardOfItem(const QStandardItem *item = 0)
{
    if (!item)
        return 0;
    return item->data(Qt::UserRole).value<Core::IWizard*>();
}

using namespace Core;
using namespace Core::Internal;

NewDialog::NewDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Core::Internal::Ui::NewDialog),
    m_okButton(0),
    m_preferredWizardKinds(0)
{
    typedef QMap<QString, QStandardItem *> CategoryItemMap;
    m_ui->setupUi(this);
    m_okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setDefault(true);
    m_okButton->setText(tr("&Create"));

    m_ui->templatesTree->header()->hide();
    m_model = new QStandardItemModel(this);
    m_ui->templatesTree->setModel(m_model);

    connect(m_ui->templatesTree, SIGNAL(clicked(const QModelIndex&)),
        this, SLOT(currentItemChanged(const QModelIndex&)));
    connect(m_ui->templatesTree, SIGNAL(activated(const QModelIndex&)),
            m_okButton, SLOT(animateClick()));

    connect(m_okButton, SIGNAL(clicked()), this, SLOT(okButtonClicked()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

// Sort by category. id
bool wizardLessThan(const IWizard *w1, const IWizard *w2)
{
    if (const int cc = w1->category().compare(w2->category()))
        return cc < 0;
    return w1->id().compare(w2->id()) < 0;
}

void NewDialog::setPreferredWizardKinds(IWizard::WizardKinds kinds)
{
    m_preferredWizardKinds = kinds;
}

void NewDialog::setWizards(QList<IWizard*> wizards)
{
    typedef QMap<QString, QStandardItem *> CategoryItemMap;

    qStableSort(wizards.begin(), wizards.end(), wizardLessThan);

    CategoryItemMap categories;

    m_model->clear();
    foreach (IWizard *wizard, wizards) {
        // ensure category root
        const QString categoryName = wizard->category();
        CategoryItemMap::iterator cit = categories.find(categoryName);
        if (cit == categories.end()) {
            QStandardItem *parentItem = m_model->invisibleRootItem();
            QStandardItem *categoryItem = new QStandardItem();
            parentItem->appendRow(categoryItem);
            categoryItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            categoryItem->setText(wizard->displayCategory());
            categoryItem->setData(QVariant::fromValue(0), Qt::UserRole);
            cit = categories.insert(categoryName, categoryItem);
        }
        // add item
        QStandardItem *wizardItem = new QStandardItem(wizard->displayName());
        wizardItem->setIcon(wizard->icon());
        wizardItem->setData(QVariant::fromValue(wizard), Qt::UserRole);
        wizardItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        cit.value()->appendRow(wizardItem);
    }
}

Core::IWizard *NewDialog::showDialog()
{
    QStandardItem *itemToSelect = 0;
    if (m_preferredWizardKinds == 0) {
        m_ui->templatesTree->expandAll();
        if (QStandardItem *rootItem = m_model->invisibleRootItem()->child(0)) {
            if (rootItem->rowCount())
                itemToSelect = rootItem->child(0);
        }
    } else {
        for (int i = 0; i < m_model->invisibleRootItem()->rowCount(); ++i) {
            QStandardItem *category = m_model->invisibleRootItem()->child(i);
            bool hasOnlyPreferred = true;
            for (int j = 0; j < category->rowCount(); ++j) {
                QStandardItem *item = category->child(j);
                if (!(item->data(Qt::UserRole).value<IWizard*>()
                        ->kind() & m_preferredWizardKinds)) {
                    hasOnlyPreferred = false;
                    break;
                }
            }
            m_ui->templatesTree->setExpanded(category->index(), hasOnlyPreferred);
            if (hasOnlyPreferred && itemToSelect == 0 && category->rowCount() > 0) {
                itemToSelect = category->child(0);
            }
        }
    }
    if (itemToSelect) {
        m_ui->templatesTree->scrollTo(itemToSelect->index());
        m_ui->templatesTree->setCurrentIndex(itemToSelect->index());
    }
    updateOkButton();
    if (exec() != Accepted)
        return 0;
    return currentWizard();
}

NewDialog::~NewDialog()
{
    delete m_ui;
}

IWizard *NewDialog::currentWizard() const
{
    return wizardOfItem(m_model->itemFromIndex(m_ui->templatesTree->currentIndex()));
}

void NewDialog::currentItemChanged(const QModelIndex &index)
{
    QStandardItem* cat = m_model->itemFromIndex(index);
    if (const IWizard *wizard = wizardOfItem(cat))
        m_ui->descLabel->setText(wizard->description());
    else
        m_ui->descLabel->setText(QString());
    updateOkButton();
}

void NewDialog::okButtonClicked()
{
    if (m_ui->templatesTree->currentIndex().isValid())
        accept();
}

void NewDialog::updateOkButton()
{
    m_okButton->setEnabled(currentWizard() != 0);
}
