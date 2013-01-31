/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "newdialog.h"
#include "ui_newdialog.h"
#include "basefilewizard.h"

#include <utils/stylehelper.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/featureprovider.h>

#include <QAbstractProxyModel>
#include <QSortFilterProxyModel>
#include <QItemSelectionModel>
#include <QHeaderView>
#include <QPushButton>
#include <QStandardItem>
#include <QItemDelegate>
#include <QPainter>
#include <QDebug>

Q_DECLARE_METATYPE(Core::IWizard*)


namespace {

const int ICON_SIZE = 22;

struct WizardContainer
{
    WizardContainer() : wizard(0), wizardOption(0) {}
    WizardContainer(Core::IWizard *w, int i): wizard(w), wizardOption(i) {}
    Core::IWizard *wizard;
    int wizardOption;
};

inline Core::IWizard *wizardOfItem(const QStandardItem *item = 0)
{
    if (!item)
        return 0;
    return item->data(Qt::UserRole).value<WizardContainer>().wizard;
}

class PlatformFilterProxyModel : public QSortFilterProxyModel
{
//    Q_OBJECT
public:
    PlatformFilterProxyModel(QObject *parent = 0): QSortFilterProxyModel(parent) {}

    void setPlatform(const QString& platform)
    {
        m_platform = platform;
        invalidateFilter();
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        if (!sourceParent.isValid())
            return true;

        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        Core::IWizard *wizard = wizardOfItem(qobject_cast<QStandardItemModel*>(sourceModel())->itemFromIndex(sourceIndex));
        if (wizard)
            return wizard->isAvailable(m_platform);

        return true;
    }
private:
    QString m_platform;
};

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

}

Q_DECLARE_METATYPE(WizardContainer)

using namespace Core;
using namespace Core::Internal;

NewDialog::NewDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Core::Internal::Ui::NewDialog),
    m_okButton(0)
{
    typedef QMap<QString, QStandardItem *> CategoryItemMap;

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    QPalette p = m_ui->frame->palette();
    p.setColor(QPalette::Window, p.color(QPalette::Base));
    m_ui->frame->setPalette(p);
    m_okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setDefault(true);
    m_okButton->setText(tr("&Choose..."));

    m_model = new QStandardItemModel(this);
    m_twoLevelProxyModel = new TwoLevelProxyModel(this);
    m_twoLevelProxyModel->setSourceModel(m_model);
    m_filterProxyModel = new PlatformFilterProxyModel(this);
    m_filterProxyModel->setSourceModel(m_model);

    m_ui->templateCategoryView->setModel(m_twoLevelProxyModel);
    m_ui->templateCategoryView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui->templateCategoryView->setItemDelegate(new FancyTopLevelDelegate);

    m_ui->templatesView->setIconSize(QSize(ICON_SIZE, ICON_SIZE));

    connect(m_ui->templateCategoryView, SIGNAL(clicked(QModelIndex)),
        this, SLOT(currentCategoryChanged(QModelIndex)));
    connect(m_ui->templatesView, SIGNAL(clicked(QModelIndex)),
        this, SLOT(currentItemChanged(QModelIndex)));

    connect(m_ui->templateCategoryView->selectionModel(),
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentCategoryChanged(QModelIndex)));
    connect(m_ui->templatesView,
            SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(okButtonClicked()));

    connect(m_okButton, SIGNAL(clicked()), this, SLOT(okButtonClicked()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(m_ui->comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(setSelectedPlatform(QString)));
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

    CategoryItemMap platformMap;

    m_model->clear();
    QStandardItem *parentItem = m_model->invisibleRootItem();

    QStandardItem *projectKindItem = new QStandardItem(tr("Projects"));
    projectKindItem->setData(IWizard::ProjectWizard, Qt::UserRole);
    projectKindItem->setFlags(0); // disable item to prevent focus
    QStandardItem *filesClassesKindItem = new QStandardItem(tr("Files and Classes"));
    filesClassesKindItem->setData(IWizard::FileWizard, Qt::UserRole);
    filesClassesKindItem->setFlags(0); // disable item to prevent focus

    parentItem->appendRow(projectKindItem);
    parentItem->appendRow(filesClassesKindItem);

    if (m_dummyIcon.isNull())
        m_dummyIcon = QIcon(QLatin1String(Core::Constants::ICON_NEWFILE));

    QStringList availablePlatforms = IWizard::allAvailablePlatforms();

    if (availablePlatforms.count() > 1) {
        m_ui->comboBox->addItem(tr("All Templates"), QString());
        foreach (const QString &platform, availablePlatforms) {
            const QString displayNameForPlatform = IWizard::displayNameForPlatform(platform);
            m_ui->comboBox->addItem(tr("%1 Templates").arg(displayNameForPlatform), platform);
        }
    } else {
        if (availablePlatforms.isEmpty()) {
            m_ui->comboBox->addItem(tr("All Templates"), QString());
        } else {
            const QString platform = availablePlatforms.first();
            const QString displayNameForPlatform = IWizard::displayNameForPlatform(platform);
            m_ui->comboBox->addItem(tr("%1 Templates").arg(displayNameForPlatform), platform);
        }
        m_ui->comboBox->setDisabled(true);
    }

    foreach (IWizard *wizard, wizards) {
        if (wizard->isAvailable(selectedPlatform())) {
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
            addItem(kindItem, wizard);
        }
    }
    if (projectKindItem->columnCount() == 0)
        parentItem->removeRow(0);
}

Core::IWizard *NewDialog::showDialog()
{
    static QString lastCategory;
    QModelIndex idx;

    if (!lastCategory.isEmpty())
        foreach (QStandardItem* item, m_categoryItems) {
            if (item->data(Qt::UserRole) == lastCategory)
                idx = m_twoLevelProxyModel->mapToSource(m_model->indexFromItem(item));
    }
    if (!idx.isValid())
        idx = m_twoLevelProxyModel->index(0,0, m_twoLevelProxyModel->index(0,0));

    m_ui->templateCategoryView->setCurrentIndex(idx);

    // We need to set ensure that the category has default focus
    m_ui->templateCategoryView->setFocus(Qt::NoFocusReason);

    for (int row = 0; row < m_twoLevelProxyModel->rowCount(); ++row)
        m_ui->templateCategoryView->setExpanded(m_twoLevelProxyModel->index(row, 0), true);

    // Ensure that item description is visible on first show
    currentItemChanged(m_ui->templatesView->rootIndex().child(0,0));

    updateOkButton();

    const int retVal = exec();

    idx = m_ui->templateCategoryView->currentIndex();
    QStandardItem *currentItem = m_model->itemFromIndex(m_twoLevelProxyModel->mapToSource(idx));
    if (currentItem)
        lastCategory = currentItem->data(Qt::UserRole).toString();

    if (retVal != Accepted)
        return 0;

    return currentWizard();
}

QString NewDialog::selectedPlatform() const
{
    int index = m_ui->comboBox->currentIndex();

    return m_ui->comboBox->itemData(index).toString();
}

int NewDialog::selectedWizardOption() const
{
    QStandardItem *item = m_model->itemFromIndex(m_ui->templatesView->currentIndex());
    return item->data(Qt::UserRole).value<WizardContainer>().wizardOption;
}

NewDialog::~NewDialog()
{
    delete m_ui;
}

IWizard *NewDialog::currentWizard() const
{
    QModelIndex index = m_filterProxyModel->mapToSource(m_ui->templatesView->currentIndex());
    return wizardOfItem(m_model->itemFromIndex(index));
}

void NewDialog::addItem(QStandardItem *topLEvelCategoryItem, IWizard *wizard)
{
    const QString categoryName = wizard->category();
    QStandardItem *categoryItem = 0;
    for (int i = 0; i < topLEvelCategoryItem->rowCount(); i++) {
        if (topLEvelCategoryItem->child(i, 0)->data(Qt::UserRole) == categoryName)
            categoryItem = topLEvelCategoryItem->child(i, 0);
    }
    if (!categoryItem) {
        categoryItem = new QStandardItem();
        topLEvelCategoryItem->appendRow(categoryItem);
        m_categoryItems.append(categoryItem);
        categoryItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        categoryItem->setText(QLatin1String("  ") + wizard->displayCategory());
        categoryItem->setData(wizard->category(), Qt::UserRole);
    }

    QStandardItem *wizardItem = new QStandardItem(wizard->displayName());
    QIcon wizardIcon;

    // spacing hack. Add proper icons instead
    if (wizard->icon().isNull())
        wizardIcon = m_dummyIcon;
    else
        wizardIcon = wizard->icon();
    wizardItem->setIcon(wizardIcon);
    wizardItem->setData(QVariant::fromValue(WizardContainer(wizard, 0)), Qt::UserRole);
    wizardItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    categoryItem->appendRow(wizardItem);

}

void NewDialog::currentCategoryChanged(const QModelIndex &index)
{
    if (index.parent() != m_model->invisibleRootItem()->index()) {
        m_ui->templatesView->setModel(m_filterProxyModel);
        QModelIndex sourceIndex = m_twoLevelProxyModel->mapToSource(index);
        sourceIndex = m_filterProxyModel->mapFromSource(sourceIndex);
        m_ui->templatesView->setRootIndex(sourceIndex);
        // Focus the first item by default
        m_ui->templatesView->setCurrentIndex(m_ui->templatesView->rootIndex().child(0,0));

        connect(m_ui->templatesView->selectionModel(),
                SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                this, SLOT(currentItemChanged(QModelIndex)));
    }
}

void NewDialog::currentItemChanged(const QModelIndex &index)
{
    QModelIndex sourceIndex = m_filterProxyModel->mapToSource(index);
    QStandardItem* cat = (m_model->itemFromIndex(sourceIndex));
    if (const IWizard *wizard = wizardOfItem(cat)) {
        QString desciption = wizard->description();
        QStringList displayNamesForSupporttedPlatforms;
        foreach (const QString &platform, wizard->supportedPlatforms())
            displayNamesForSupporttedPlatforms << IWizard::displayNameForPlatform(platform);
        if (!Qt::mightBeRichText(desciption))
            desciption.replace(QLatin1Char('\n'), QLatin1String("<br>"));
        desciption += QLatin1String("<br><br><b>");
        if (wizard->flags().testFlag(IWizard::PlatformIndependent))
            desciption += tr("Platform independent") + QLatin1String("</b>");
        else
            desciption += tr("Supported Platforms")
                    + QLatin1String("</b>: <tt>")
                    + displayNamesForSupporttedPlatforms.join(QLatin1String(" "))
                    + QLatin1String("</tt>");

        m_ui->templateDescription->setHtml(desciption);

        if (!wizard->descriptionImage().isEmpty()) {
            m_ui->imageLabel->setVisible(true);
            m_ui->imageLabel->setPixmap(wizard->descriptionImage());
        } else {
            m_ui->imageLabel->setVisible(false);
        }

    } else {
        m_ui->templateDescription->setText(QString());
    }
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

void NewDialog::setSelectedPlatform(const QString & /*platform*/)
{
    //The static cast allows us to keep PlatformFilterProxyModel anonymous
    static_cast<PlatformFilterProxyModel*>(m_filterProxyModel)->setPlatform(selectedPlatform());
}
