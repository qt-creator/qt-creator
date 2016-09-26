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

#include "newdialog.h"
#include "ui_newdialog.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAbstractProxyModel>
#include <QDebug>
#include <QItemDelegate>
#include <QKeyEvent>
#include <QModelIndex>
#include <QPainter>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItem>
#include <QTimer>

Q_DECLARE_METATYPE(Core::IWizardFactory*)

namespace {

const int ICON_SIZE = 22;
const char LAST_CATEGORY_KEY[] = "Core/NewDialog/LastCategory";
const char LAST_PLATFORM_KEY[] = "Core/NewDialog/LastPlatform";

class WizardFactoryContainer
{
public:
    WizardFactoryContainer() : wizard(0), wizardOption(0) {}
    WizardFactoryContainer(Core::IWizardFactory *w, int i): wizard(w), wizardOption(i) {}
    Core::IWizardFactory *wizard;
    int wizardOption;
};

inline Core::IWizardFactory *factoryOfItem(const QStandardItem *item = 0)
{
    if (!item)
        return 0;
    return item->data(Qt::UserRole).value<WizardFactoryContainer>().wizard;
}

class PlatformFilterProxyModel : public QSortFilterProxyModel
{
//    Q_OBJECT
public:
    PlatformFilterProxyModel(QObject *parent = 0): QSortFilterProxyModel(parent) {}

    void setPlatform(Core::Id platform)
    {
        m_platform = platform;
        invalidateFilter();
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        if (!sourceParent.isValid())
            return true;

        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        Core::IWizardFactory *wizard = factoryOfItem(qobject_cast<QStandardItemModel*>(sourceModel())->itemFromIndex(sourceIndex));
        if (wizard)
            return wizard->isAvailable(m_platform);

        return true;
    }
private:
    Core::Id m_platform;
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

Q_DECLARE_METATYPE(WizardFactoryContainer)

using namespace Core;
using namespace Core::Internal;

QWidget *NewDialog::m_currentDialog = nullptr;

NewDialog::NewDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::NewDialog),
    m_okButton(0)
{
    QTC_CHECK(m_currentDialog == nullptr);

    m_currentDialog = this;

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags());
    setAttribute(Qt::WA_DeleteOnClose);
    ICore::registerWindow(this, Context("Core.NewDialog"));
    m_ui->setupUi(this);
    QPalette p = m_ui->frame->palette();
    p.setColor(QPalette::Window, p.color(QPalette::Base));
    m_ui->frame->setPalette(p);
    m_okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setDefault(true);
    m_okButton->setText(tr("Choose..."));

    m_model = new QStandardItemModel(this);
    m_twoLevelProxyModel = new TwoLevelProxyModel(this);
    m_twoLevelProxyModel->setSourceModel(m_model);
    m_filterProxyModel = new PlatformFilterProxyModel(this);
    m_filterProxyModel->setSourceModel(m_model);

    m_ui->templateCategoryView->setModel(m_twoLevelProxyModel);
    m_ui->templateCategoryView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui->templateCategoryView->setItemDelegate(new FancyTopLevelDelegate);

    m_ui->templatesView->setModel(m_filterProxyModel);
    m_ui->templatesView->setIconSize(QSize(ICON_SIZE, ICON_SIZE));

    connect(m_ui->templateCategoryView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &NewDialog::currentCategoryChanged);

    connect(m_ui->templatesView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &NewDialog::currentItemChanged);

    connect(m_ui->templatesView, &QListView::doubleClicked, this, &NewDialog::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &NewDialog::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &NewDialog::reject);

    connect(m_ui->comboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
            this, &NewDialog::setSelectedPlatform);
}

// Sort by category. id
static bool wizardFactoryLessThan(const IWizardFactory *f1, const IWizardFactory *f2)
{
    if (const int cc = f1->category().compare(f2->category()))
        return cc < 0;
    return f1->id().toString().compare(f2->id().toString()) < 0;
}

void NewDialog::setWizardFactories(QList<IWizardFactory *> factories,
                                   const QString &defaultLocation,
                                   const QVariantMap &extraVariables)
{
    m_defaultLocation = defaultLocation;
    m_extraVariables = extraVariables;
    std::stable_sort(factories.begin(), factories.end(), wizardFactoryLessThan);

    m_model->clear();
    QStandardItem *parentItem = m_model->invisibleRootItem();

    QStandardItem *projectKindItem = new QStandardItem(tr("Projects"));
    projectKindItem->setData(IWizardFactory::ProjectWizard, Qt::UserRole);
    projectKindItem->setFlags(0); // disable item to prevent focus
    QStandardItem *filesKindItem = new QStandardItem(tr("Files and Classes"));
    filesKindItem->setData(IWizardFactory::FileWizard, Qt::UserRole);
    filesKindItem->setFlags(0); // disable item to prevent focus

    parentItem->appendRow(projectKindItem);
    parentItem->appendRow(filesKindItem);

    if (m_dummyIcon.isNull())
        m_dummyIcon = Utils::Icons::NEWFILE.icon();

    QSet<Id> availablePlatforms = IWizardFactory::allAvailablePlatforms();
    m_ui->comboBox->addItem(tr("All Templates"), Id().toSetting());

    foreach (Id platform, availablePlatforms) {
        const QString displayNameForPlatform = IWizardFactory::displayNameForPlatform(platform);
        m_ui->comboBox->addItem(tr("%1 Templates").arg(displayNameForPlatform), platform.toSetting());
    }

    m_ui->comboBox->setCurrentIndex(0); // "All templates"
    m_ui->comboBox->setEnabled(!availablePlatforms.isEmpty());

    foreach (IWizardFactory *factory, factories) {
        QStandardItem *kindItem;
        switch (factory->kind()) {
        case IWizardFactory::ProjectWizard:
            kindItem = projectKindItem;
            break;
        case IWizardFactory::FileWizard:
        default:
            kindItem = filesKindItem;
            break;
        }
        addItem(kindItem, factory);
    }
    if (projectKindItem->columnCount() == 0)
        parentItem->removeRow(0);
}

void NewDialog::showDialog()
{
    QModelIndex idx;

    QString lastPlatform = ICore::settings()->value(QLatin1String(LAST_PLATFORM_KEY)).toString();
    QString lastCategory = ICore::settings()->value(QLatin1String(LAST_CATEGORY_KEY)).toString();

    if (!lastPlatform.isEmpty()) {
        int index = m_ui->comboBox->findData(lastPlatform);
        if (index != -1)
            m_ui->comboBox->setCurrentIndex(index);
    }

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
    show();
}

Id NewDialog::selectedPlatform() const
{
    const int index = m_ui->comboBox->currentIndex();
    return Id::fromSetting(m_ui->comboBox->itemData(index));
}

QWidget *NewDialog::currentDialog()
{
    return m_currentDialog;
}

bool NewDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            ke->accept();
            return true;
        }
    }
    return QDialog::event(event);
}

NewDialog::~NewDialog()
{
    QTC_CHECK(m_currentDialog != nullptr);
    m_currentDialog = nullptr;

    delete m_ui;
}

IWizardFactory *NewDialog::currentWizardFactory() const
{
    QModelIndex index = m_filterProxyModel->mapToSource(m_ui->templatesView->currentIndex());
    return factoryOfItem(m_model->itemFromIndex(index));
}

void NewDialog::addItem(QStandardItem *topLevelCategoryItem, IWizardFactory *factory)
{
    const QString categoryName = factory->category();
    QStandardItem *categoryItem = 0;
    for (int i = 0; i < topLevelCategoryItem->rowCount(); i++) {
        if (topLevelCategoryItem->child(i, 0)->data(Qt::UserRole) == categoryName)
            categoryItem = topLevelCategoryItem->child(i, 0);
    }
    if (!categoryItem) {
        categoryItem = new QStandardItem();
        topLevelCategoryItem->appendRow(categoryItem);
        m_categoryItems.append(categoryItem);
        categoryItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        categoryItem->setText(QLatin1String("  ") + factory->displayCategory());
        categoryItem->setData(factory->category(), Qt::UserRole);
    }

    QStandardItem *wizardItem = new QStandardItem(factory->displayName());
    QIcon wizardIcon;

    // spacing hack. Add proper icons instead
    if (factory->icon().isNull())
        wizardIcon = m_dummyIcon;
    else
        wizardIcon = factory->icon();
    wizardItem->setIcon(wizardIcon);
    wizardItem->setData(QVariant::fromValue(WizardFactoryContainer(factory, 0)), Qt::UserRole);
    wizardItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    categoryItem->appendRow(wizardItem);

}

void NewDialog::currentCategoryChanged(const QModelIndex &index)
{
    if (index.parent() != m_model->invisibleRootItem()->index()) {
        QModelIndex sourceIndex = m_twoLevelProxyModel->mapToSource(index);
        sourceIndex = m_filterProxyModel->mapFromSource(sourceIndex);
        m_ui->templatesView->setRootIndex(sourceIndex);
        // Focus the first item by default
        m_ui->templatesView->setCurrentIndex(m_ui->templatesView->rootIndex().child(0,0));
    }
}

void NewDialog::currentItemChanged(const QModelIndex &index)
{
    QModelIndex sourceIndex = m_filterProxyModel->mapToSource(index);
    QStandardItem* cat = (m_model->itemFromIndex(sourceIndex));
    if (const IWizardFactory *wizard = factoryOfItem(cat)) {
        QString desciption = wizard->description();
        QStringList displayNamesForSupportedPlatforms;
        foreach (Id platform, wizard->supportedPlatforms())
            displayNamesForSupportedPlatforms << IWizardFactory::displayNameForPlatform(platform);
        Utils::sort(displayNamesForSupportedPlatforms);
        if (!Qt::mightBeRichText(desciption))
            desciption.replace(QLatin1Char('\n'), QLatin1String("<br>"));
        desciption += QLatin1String("<br><br><b>");
        if (wizard->flags().testFlag(IWizardFactory::PlatformIndependent))
            desciption += tr("Platform independent") + QLatin1String("</b>");
        else
            desciption += tr("Supported Platforms")
                    + QLatin1String("</b>: <tt>")
                    + displayNamesForSupportedPlatforms.join(QLatin1Char(' '))
                    + QLatin1String("</tt>");

        m_ui->templateDescription->setHtml(desciption);

        if (!wizard->descriptionImage().isEmpty()) {
            m_ui->imageLabel->setVisible(true);
            m_ui->imageLabel->setPixmap(wizard->descriptionImage());
        } else {
            m_ui->imageLabel->setVisible(false);
        }

    } else {
        m_ui->templateDescription->clear();
    }
    updateOkButton();
}

void NewDialog::saveState()
{
    QModelIndex idx = m_ui->templateCategoryView->currentIndex();
    QStandardItem *currentItem = m_model->itemFromIndex(m_twoLevelProxyModel->mapToSource(idx));
    if (currentItem)
        ICore::settings()->setValue(QLatin1String(LAST_CATEGORY_KEY),
                                    currentItem->data(Qt::UserRole));
    ICore::settings()->setValue(QLatin1String(LAST_PLATFORM_KEY), m_ui->comboBox->currentData());
}

static void runWizard(IWizardFactory *wizard, const QString &defaultLocation, Id platform,
                      const QVariantMap &variables)
{
    QString path = wizard->runPath(defaultLocation);
    wizard->runWizard(path, ICore::dialogParent(), platform, variables);
}

void NewDialog::accept()
{
    saveState();
    if (m_ui->templatesView->currentIndex().isValid()) {
        IWizardFactory *wizard = currentWizardFactory();
        if (QTC_GUARD(wizard)) {
            QTimer::singleShot(0, std::bind(&runWizard, wizard, m_defaultLocation,
                                            selectedPlatform(), m_extraVariables));
        }
    }
    QDialog::accept();
}

void NewDialog::reject()
{
    saveState();
    QDialog::reject();
}

void NewDialog::updateOkButton()
{
    m_okButton->setEnabled(currentWizardFactory() != 0);
}

void NewDialog::setSelectedPlatform(const QString & /*platform*/)
{
    //The static cast allows us to keep PlatformFilterProxyModel anonymous
    static_cast<PlatformFilterProxyModel*>(m_filterProxyModel)->setPlatform(selectedPlatform());
}
