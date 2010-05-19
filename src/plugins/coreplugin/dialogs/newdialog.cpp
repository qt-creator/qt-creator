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

#include <utils/stylehelper.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/iwizard.h>

#include <QtGui/QAbstractProxyModel>
#include <QtGui/QItemSelectionModel>
#include <QtGui/QHeaderView>
#include <QtGui/QPushButton>
#include <QtGui/QStandardItem>
#include <QtGui/QItemDelegate>
#include <QtGui/QPainter>
#include <QtCore/QDebug>

Q_DECLARE_METATYPE(Core::IWizard*)


namespace {

const int ICON_SIZE = 22;

class TwoLevelProxyModel : public QAbstractProxyModel
{
//    Q_OBJECT
public:
    TwoLevelProxyModel(QObject *parent = 0): QAbstractProxyModel(parent) {}

    QModelIndex index(int row, int column, const QModelIndex &parent) const
    {
        QModelIndex ourModelIndex = sourceModel()->index(row, column, mapToSource(parent));
        return createIndex(row, column, ourModelIndex.internalPointer());
    }

    QModelIndex parent(const QModelIndex &index) const
    {
        return mapFromSource(mapToSource(index).parent());
    }

    int rowCount(const QModelIndex &index) const
    {
        if (index.isValid() && index.parent().isValid() && !index.parent().parent().isValid())
            return 0;
        else
            return sourceModel()->rowCount(mapToSource(index));
    }

    int columnCount(const QModelIndex &index) const
    {
        return sourceModel()->columnCount(mapToSource(index));
    }

    QModelIndex	mapFromSource (const QModelIndex &index) const
    {
        if (!index.isValid())
            return QModelIndex();
        return createIndex(index.row(), index.column(), index.internalPointer());
    }

    QModelIndex	mapToSource (const QModelIndex &index) const
    {
        if (!index.isValid())
            return QModelIndex();
        return static_cast<TwoLevelProxyModel*>(sourceModel())->createIndex(index.row(), index.column(), index.internalPointer());
    }
};

#define ROW_HEIGHT 24

class FancyTopLevelDelegate : public QItemDelegate
{
public:
    FancyTopLevelDelegate(QObject *parent = 0)
        : QItemDelegate(parent) {}

    void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const
    {
        QStyleOptionViewItem newoption = option;
        if (!(option.state & QStyle::State_Enabled)) {
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            gradient.setColorAt(0, option.palette.window().color().lighter(106));
            gradient.setColorAt(1, option.palette.window().color().darker(106));
            painter->fillRect(rect, gradient);
            painter->setPen(option.palette.window().color().darker(130));
            if (rect.top())
                painter->drawLine(rect.topRight(), rect.topLeft());
            painter->drawLine(rect.bottomRight(), rect.bottomLeft());

            // Fake enabled state
            newoption.state |= newoption.state | QStyle::State_Enabled;
        }

        QItemDelegate::drawDisplay(painter, newoption, rect, text);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QSize size = QItemDelegate::sizeHint(option, index);


        size = size.expandedTo(QSize(0, ROW_HEIGHT));

        return size;
    }
};


inline Core::IWizard *wizardOfItem(const QStandardItem *item = 0)
{
    if (!item)
        return 0;
    return item->data(Qt::UserRole).value<Core::IWizard*>();
}

}

using namespace Core;
using namespace Core::Internal;

NewDialog::NewDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Core::Internal::Ui::NewDialog),
    m_okButton(0)
{
    typedef QMap<QString, QStandardItem *> CategoryItemMap;
    m_ui->setupUi(this);
    m_okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setDefault(true);
    m_okButton->setText(tr("&Choose..."));

    m_model = new QStandardItemModel(this);
    m_proxyModel = new TwoLevelProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_ui->templateCategoryView->setModel(m_proxyModel);
    m_ui->templateCategoryView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui->templateCategoryView->setItemDelegate(new FancyTopLevelDelegate);

    m_ui->templatesView->setIconSize(QSize(ICON_SIZE, ICON_SIZE));

    connect(m_ui->templateCategoryView, SIGNAL(clicked(const QModelIndex&)),
        this, SLOT(currentCategoryChanged(const QModelIndex&)));
    connect(m_ui->templatesView, SIGNAL(clicked(const QModelIndex&)),
        this, SLOT(currentItemChanged(const QModelIndex&)));

    connect(m_ui->templateCategoryView->selectionModel(),
            SIGNAL(currentChanged(const QModelIndex&,const QModelIndex&)),
            this, SLOT(currentCategoryChanged(const QModelIndex&)));
    connect(m_ui->templatesView,
            SIGNAL(doubleClicked(const QModelIndex&)),
            this, SLOT(okButtonClicked()));

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

void NewDialog::setWizards(QList<IWizard*> wizards)
{
    typedef QMap<QString, QStandardItem *> CategoryItemMap;

    qStableSort(wizards.begin(), wizards.end(), wizardLessThan);


    CategoryItemMap categories;

    m_model->clear();
    QStandardItem *projectKindItem = new QStandardItem(tr("Projects"));
    projectKindItem->setData(IWizard::ProjectWizard, Qt::UserRole);
    projectKindItem->setFlags(0); // disable item to prevent focus
    QStandardItem *filesClassesKindItem = new QStandardItem(tr("Files and Classes"));
    filesClassesKindItem->setData(IWizard::FileWizard, Qt::UserRole);
    filesClassesKindItem->setFlags(0); // disable item to prevent focus
    QStandardItem *parentItem = m_model->invisibleRootItem();
    parentItem->appendRow(projectKindItem);
    parentItem->appendRow(filesClassesKindItem);

    if (m_dummyIcon.isNull()) {
        m_dummyIcon = QPixmap(ICON_SIZE, ICON_SIZE);
        m_dummyIcon.fill(Qt::transparent);
    }

    foreach (IWizard *wizard, wizards) {
        // ensure category root
        const QString categoryName = wizard->category();

        CategoryItemMap::iterator cit = categories.find(categoryName);
        if (cit == categories.end()) {
            QStandardItem *categoryItem = new QStandardItem();
            QStandardItem *kindItem;
            switch (wizard->kind()) {
            case IWizard::ProjectWizard:
                kindItem = projectKindItem;
                break;
            case IWizard::ClassWizard:
            case IWizard::FileWizard:
            default:
                kindItem = filesClassesKindItem;
                break;
            }
            kindItem->appendRow(categoryItem);
            m_categoryItems.append(categoryItem);
            categoryItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            categoryItem->setText(wizard->displayCategory());
            categoryItem->setData(wizard->category(), Qt::UserRole);
            cit = categories.insert(categoryName, categoryItem);
        }
        // add item
        QStandardItem *wizardItem = new QStandardItem(wizard->displayName());
        QIcon wizardIcon;

        // spacing hack. Add proper icons instead
        if (wizard->icon().isNull()) {
            wizardIcon = m_dummyIcon;
        } else {
            wizardIcon = wizard->icon();
        }
        wizardItem->setIcon(wizardIcon);
        wizardItem->setData(QVariant::fromValue(wizard), Qt::UserRole);
        wizardItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        cit.value()->appendRow(wizardItem);
    }


    if (!projectKindItem->hasChildren()) {
        QModelIndex idx = projectKindItem->index();
        m_model->removeRow(idx.row());
    }
    if (!filesClassesKindItem->hasChildren()) {
        QModelIndex idx = filesClassesKindItem->index();
        m_model->removeRow(idx.row());
    }
}

Core::IWizard *NewDialog::showDialog()
{
    static QString lastCategory;
    QModelIndex idx;

    if (!lastCategory.isEmpty())
        foreach(QStandardItem* item, m_categoryItems) {
            if (item->data(Qt::UserRole) == lastCategory) {
                idx = m_proxyModel->mapToSource(m_model->indexFromItem(item));
        }
    }
    if (!idx.isValid())
        idx = m_proxyModel->index(0,0, m_proxyModel->index(0,0));

    m_ui->templateCategoryView->setCurrentIndex(idx);

    // We need to set ensure that the category has default focus
    m_ui->templateCategoryView->setFocus(Qt::NoFocusReason);

    for (int row = 0; row < m_proxyModel->rowCount(); ++row)
        m_ui->templateCategoryView->setExpanded(m_proxyModel->index(row, 0), true);

    // Ensure that item description is visible on first show
    currentItemChanged(m_ui->templatesView->rootIndex().child(0,0));

    updateOkButton();

    const int retVal = exec();

    idx = m_ui->templateCategoryView->currentIndex();
    lastCategory = m_model->itemFromIndex(m_proxyModel->mapToSource(idx))->data(Qt::UserRole).toString();

    if (retVal != Accepted)
        return 0;

    return currentWizard();
}

NewDialog::~NewDialog()
{
    delete m_ui;
}

IWizard *NewDialog::currentWizard() const
{
    return wizardOfItem(m_model->itemFromIndex(m_ui->templatesView->currentIndex()));
}

void NewDialog::currentCategoryChanged(const QModelIndex &index)
{
    if (index.parent() != m_model->invisibleRootItem()->index()) {
        m_ui->templatesView->setModel(m_model);
        m_ui->templatesView->setRootIndex(m_proxyModel->mapToSource(index));

        // Focus the first item by default
        m_ui->templatesView->setCurrentIndex(m_ui->templatesView->rootIndex().child(0,0));

        connect(m_ui->templatesView->selectionModel(),
                SIGNAL(currentChanged(const QModelIndex&,const QModelIndex&)),
                this, SLOT(currentItemChanged(const QModelIndex&)));
    }
}

void NewDialog::currentItemChanged(const QModelIndex &index)
{
    QStandardItem* cat = m_model->itemFromIndex(index);
    if (const IWizard *wizard = wizardOfItem(cat))
        m_ui->templateDescription->setText(wizard->description());
    else
        m_ui->templateDescription->setText(QString());
    updateOkButton();
}

void NewDialog::okButtonClicked()
{
    if (m_ui->templatesView->currentIndex().isValid())
        accept();
}

void NewDialog::updateOkButton()
{
    m_okButton->setEnabled(currentWizard() != 0);
}
