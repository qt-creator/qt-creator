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

const int ICON_SIZE = 48;
const char LAST_CATEGORY_KEY[] = "Core/NewDialog/LastCategory";
const char LAST_PLATFORM_KEY[] = "Core/NewDialog/LastPlatform";
const char ALLOW_ALL_TEMPLATES[] = "Core/NewDialog/AllowAllTemplates";
const char SHOW_PLATOFORM_FILTER[] = "Core/NewDialog/ShowPlatformFilter";
const char BLACKLISTED_CATEGORIES_KEY[] = "Core/NewDialog/BlacklistedCategories";
const char ALTERNATIVE_WIZARD_STYLE[] = "Core/NewDialog/AlternativeWizardStyle";

using namespace Core;
using namespace Core::Internal;

class WizardFactoryContainer
{
public:
    WizardFactoryContainer() = default;
    WizardFactoryContainer(Core::IWizardFactory *w, int i): wizard(w), wizardOption(i) {}
    Core::IWizardFactory *wizard = nullptr;
    int wizardOption = 0;
};

inline Core::IWizardFactory *factoryOfItem(const QStandardItem *item = nullptr)
{
    if (!item)
        return nullptr;
    return item->data(Qt::UserRole).value<WizardFactoryContainer>().wizard;
}

class PlatformFilterProxyModel : public QSortFilterProxyModel
{
//    Q_OBJECT
public:
    PlatformFilterProxyModel(QObject *parent = nullptr): QSortFilterProxyModel(parent)
    {
        m_blacklistedCategories =
                        Id::fromStringList(ICore::settings()->value(BLACKLISTED_CATEGORIES_KEY).toStringList());
    }

    void setPlatform(Core::Id platform)
    {
        m_platform = platform;
        invalidateFilter();
    }

    void manualReset()
    {
        beginResetModel();
        endResetModel();
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        if (!sourceParent.isValid())
            return true;

        if (!sourceParent.parent().isValid()) { // category
            const QModelIndex sourceCategoryIndex = sourceModel()->index(sourceRow, 0, sourceParent);
            for (int i = 0; i < sourceModel()->rowCount(sourceCategoryIndex); ++i)
                if (filterAcceptsRow(i, sourceCategoryIndex))
                    return true;
            return false;
        }

        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        Core::IWizardFactory *wizard =
                factoryOfItem(qobject_cast<QStandardItemModel*>(sourceModel())->itemFromIndex(sourceIndex));

        if (wizard) {
            if (m_blacklistedCategories.contains(Core::Id::fromString(wizard->category())))
                return false;
            return wizard->isAvailable(m_platform);
        }

        return true;
    }
private:
    Core::Id m_platform;
    QSet<Id> m_blacklistedCategories;
};

#define ROW_HEIGHT 24

class FancyTopLevelDelegate : public QItemDelegate
{
public:
    FancyTopLevelDelegate(QObject *parent = nullptr)
        : QItemDelegate(parent) {}

    void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const override
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

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
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
    m_ui(new Ui::NewDialog)
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

    m_filterProxyModel = new PlatformFilterProxyModel(this);
    m_filterProxyModel->setSourceModel(m_model);

    m_ui->templateCategoryView->setModel(m_filterProxyModel);
    m_ui->templateCategoryView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui->templateCategoryView->setItemDelegate(new FancyTopLevelDelegate);

    m_ui->templatesView->setModel(m_filterProxyModel);
    m_ui->templatesView->setIconSize(QSize(ICON_SIZE, ICON_SIZE));

    const bool alternativeWizardStyle = ICore::settings()->value(ALTERNATIVE_WIZARD_STYLE, false).toBool();

    if (alternativeWizardStyle) {
        m_ui->templatesView->setGridSize(QSize(256, 128));
        m_ui->templatesView->setIconSize(QSize(96, 96));
        m_ui->templatesView->setSpacing(4);

        m_ui->templatesView->setViewMode(QListView::IconMode);
        m_ui->templatesView->setMovement(QListView::Static);
        m_ui->templatesView->setResizeMode(QListView::Adjust);
        m_ui->templatesView->setSelectionRectVisible(false);
        m_ui->templatesView->setWrapping(true);
        m_ui->templatesView->setWordWrap(true);
    }

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
    projectKindItem->setFlags(nullptr); // disable item to prevent focus
    QStandardItem *filesKindItem = new QStandardItem(tr("Files and Classes"));
    filesKindItem->setData(IWizardFactory::FileWizard, Qt::UserRole);
    filesKindItem->setFlags(nullptr); // disable item to prevent focus

    parentItem->appendRow(projectKindItem);
    parentItem->appendRow(filesKindItem);

    if (m_dummyIcon.isNull())
        m_dummyIcon = QIcon(":/utils/images/wizardicon-file.png");

    QSet<Id> availablePlatforms = IWizardFactory::allAvailablePlatforms();

    const bool allowAllTemplates = ICore::settings()->value(ALLOW_ALL_TEMPLATES, true).toBool();
    if (allowAllTemplates)
        m_ui->comboBox->addItem(tr("All Templates"), Id().toSetting());

    foreach (Id platform, availablePlatforms) {
        const QString displayNameForPlatform = IWizardFactory::displayNameForPlatform(platform);
        m_ui->comboBox->addItem(tr("%1 Templates").arg(displayNameForPlatform), platform.toSetting());
    }

    m_ui->comboBox->setCurrentIndex(0); // "All templates"
    m_ui->comboBox->setEnabled(!availablePlatforms.isEmpty());

    const bool showPlatformFilter = ICore::settings()->value(SHOW_PLATOFORM_FILTER, true).toBool();
    if (!showPlatformFilter)
        m_ui->comboBox->hide();

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

    static_cast<PlatformFilterProxyModel*>(m_filterProxyModel)->manualReset();

    if (!lastCategory.isEmpty())
        foreach (QStandardItem* item, m_categoryItems) {
            if (item->data(Qt::UserRole) == lastCategory)
                idx = m_filterProxyModel->mapFromSource(m_model->indexFromItem(item));
    }
    if (!idx.isValid())
        idx = m_filterProxyModel->index(0,0, m_filterProxyModel->index(0,0));

    m_ui->templateCategoryView->setCurrentIndex(idx);

    // We need to set ensure that the category has default focus
    m_ui->templateCategoryView->setFocus(Qt::NoFocusReason);

    for (int row = 0; row < m_filterProxyModel->rowCount(); ++row)
        m_ui->templateCategoryView->setExpanded(m_filterProxyModel->index(row, 0), true);

    // Ensure that item description is visible on first show
    currentItemChanged(m_filterProxyModel->index(0, 0, m_ui->templatesView->rootIndex()));

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
        auto ke = static_cast<QKeyEvent *>(event);
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

static QIcon iconWithText(const QIcon &icon, const QString &text)
{
    if (text.isEmpty())
        return icon;
    QIcon iconWithText;
    for (const QSize &pixmapSize : icon.availableSizes()) {
        QPixmap pixmap = icon.pixmap(pixmapSize);
        const int fontSize = pixmap.height() / 4;
        const int margin = pixmap.height() / 8;
        QFont font;
        font.setPixelSize(fontSize);
        font.setStretch(85);
        QPainter p(&pixmap);
        p.setFont(font);
        QTextOption textOption(Qt::AlignHCenter | Qt::AlignBottom);
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        p.drawText(pixmap.rect().adjusted(margin, margin, -margin, -margin), text, textOption);
        iconWithText.addPixmap(pixmap);
    }
    return iconWithText;
}

void NewDialog::addItem(QStandardItem *topLevelCategoryItem, IWizardFactory *factory)
{
    const QString categoryName = factory->category();
    QStandardItem *categoryItem = nullptr;
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
    wizardItem->setIcon(iconWithText(wizardIcon, factory->iconText()));
    wizardItem->setData(QVariant::fromValue(WizardFactoryContainer(factory, 0)), Qt::UserRole);
    wizardItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    categoryItem->appendRow(wizardItem);

}

void NewDialog::currentCategoryChanged(const QModelIndex &index)
{
    if (index.parent() != m_model->invisibleRootItem()->index()) {
        QModelIndex sourceIndex = m_filterProxyModel->mapToSource(index);
        sourceIndex = m_filterProxyModel->mapFromSource(sourceIndex);
        m_ui->templatesView->setRootIndex(sourceIndex);
        // Focus the first item by default
        m_ui->templatesView->setCurrentIndex(
                    m_filterProxyModel->index(0, 0, m_ui->templatesView->rootIndex()));
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
    const QModelIndex filterIdx = m_ui->templateCategoryView->currentIndex();
    const QModelIndex idx = m_filterProxyModel->mapToSource(filterIdx);
    QStandardItem *currentItem = m_model->itemFromIndex(idx);
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
    m_okButton->setEnabled(currentWizardFactory() != nullptr);
}

void NewDialog::setSelectedPlatform(const QString & /*platform*/)
{
    //The static cast allows us to keep PlatformFilterProxyModel anonymous
    static_cast<PlatformFilterProxyModel*>(m_filterProxyModel)->setPlatform(selectedPlatform());
}
