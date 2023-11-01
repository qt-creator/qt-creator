// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "newdialogwidget.h"

#include "../coreplugintr.h"
#include "../icontext.h"
#include "../icore.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QCoreApplication>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QItemDelegate>
#include <QKeyEvent>
#include <QLabel>
#include <QListView>
#include <QModelIndex>
#include <QPainter>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItem>
#include <QTextBrowser>
#include <QTreeView>

Q_DECLARE_METATYPE(Core::IWizardFactory*)

using namespace Utils;

namespace Core::Internal {

const int ICON_SIZE = 48;
const char LAST_CATEGORY_KEY[] = "Core/NewDialog/LastCategory";
const char LAST_PLATFORM_KEY[] = "Core/NewDialog/LastPlatform";
const char ALLOW_ALL_TEMPLATES[] = "Core/NewDialog/AllowAllTemplates";
const char SHOW_PLATOFORM_FILTER[] = "Core/NewDialog/ShowPlatformFilter";
const char BLACKLISTED_CATEGORIES_KEY[] = "Core/NewDialog/BlacklistedCategories";
const char ALTERNATIVE_WIZARD_STYLE[] = "Core/NewDialog/AlternativeWizardStyle";

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
public:
    PlatformFilterProxyModel(QObject *parent = nullptr): QSortFilterProxyModel(parent)
    {
        m_blacklistedCategories =
                        Id::fromStringList(ICore::settings()->value(BLACKLISTED_CATEGORIES_KEY).toStringList());
    }

    void setPlatform(Id platform)
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
            if (m_blacklistedCategories.contains(Id::fromString(wizard->category())))
                return false;
            return wizard->isAvailable(m_platform);
        }

        return true;
    }
private:
    Id m_platform;
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

}  // Core::Internal

Q_DECLARE_METATYPE(Core::Internal::WizardFactoryContainer)

namespace Core::Internal {

NewDialogWidget::NewDialogWidget(QWidget *parent)
    : QDialog(parent)
    , m_comboBox(new QComboBox)
    , m_templateCategoryView(new QTreeView)
    , m_templatesView(new QListView)
    , m_imageLabel(new QLabel)
    , m_templateDescription(new QTextBrowser)

{
    setObjectName("Core.NewDialog");
    setAttribute(Qt::WA_DeleteOnClose);
    ICore::registerWindow(this, Context("Core.NewDialog"));
    resize(880, 520);

    auto frame = new QFrame;
    frame->setAutoFillBackground(true);
    frame->setFrameShape(QFrame::StyledPanel);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok,
                                          Qt::Horizontal);

    m_templateCategoryView->setObjectName("templateCategoryView");
    m_templateCategoryView->setStyleSheet(QString::fromUtf8(" QTreeView::branch {\n"
                                                            "         background: transparent;\n"
                                                            " }"));
    m_templateCategoryView->setIndentation(0);
    m_templateCategoryView->setRootIsDecorated(false);
    m_templateCategoryView->setItemsExpandable(false);
    m_templateCategoryView->setHeaderHidden(true);
    m_templateCategoryView->header()->setVisible(false);

    m_templatesView->setObjectName("templatesView");
    m_templatesView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_templatesView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_templatesView->setUniformItemSizes(false);

    m_templateDescription->setObjectName("templateDescription");
    m_templateDescription->setFocusPolicy(Qt::NoFocus);
    m_templateDescription->setFrameShape(QFrame::NoFrame);

    using namespace Layouting;

    Column { m_imageLabel, m_templateDescription }.attachTo(frame);

    Column {
        Row { Tr::tr("Choose a template:"), st, m_comboBox },
        Row { m_templateCategoryView, m_templatesView, frame },
        buttonBox
    }.attachTo(this);

    QPalette p = frame->palette();
    p.setColor(QPalette::Window, p.color(QPalette::Base));
    frame->setPalette(p);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setDefault(true);
    m_okButton->setText(Tr::tr("Choose..."));

    m_model = new QStandardItemModel(this);

    m_filterProxyModel = new PlatformFilterProxyModel(this);
    m_filterProxyModel->setSourceModel(m_model);

    m_templateCategoryView->setModel(m_filterProxyModel);
    m_templateCategoryView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_templateCategoryView->setItemDelegate(new FancyTopLevelDelegate(this));

    m_templatesView->setModel(m_filterProxyModel);
    m_templatesView->setIconSize(QSize(ICON_SIZE, ICON_SIZE));

    const bool alternativeWizardStyle = ICore::settings()->value(ALTERNATIVE_WIZARD_STYLE, false).toBool();

    if (alternativeWizardStyle) {
        m_templatesView->setGridSize(QSize(256, 128));
        m_templatesView->setIconSize(QSize(96, 96));
        m_templatesView->setSpacing(4);

        m_templatesView->setViewMode(QListView::IconMode);
        m_templatesView->setMovement(QListView::Static);
        m_templatesView->setResizeMode(QListView::Adjust);
        m_templatesView->setSelectionRectVisible(false);
        m_templatesView->setWrapping(true);
        m_templatesView->setWordWrap(true);
    }

    connect(m_templateCategoryView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &NewDialogWidget::currentCategoryChanged);

    connect(m_templatesView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &NewDialogWidget::currentItemChanged);

    connect(m_templatesView, &QListView::doubleClicked, this, &NewDialogWidget::accept);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &NewDialogWidget::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &NewDialogWidget::reject);

    connect(m_comboBox,
            &QComboBox::currentIndexChanged,
            this,
            &NewDialogWidget::setSelectedPlatform);
}

// Sort by category. id
static bool wizardFactoryLessThan(const IWizardFactory *f1, const IWizardFactory *f2)
{
    if (const int cc = f1->category().compare(f2->category()))
        return cc < 0;
    return f1->id().toString().compare(f2->id().toString()) < 0;
}

void NewDialogWidget::setWizardFactories(QList<IWizardFactory *> factories,
                                   const FilePath &defaultLocation,
                                   const QVariantMap &extraVariables)
{
    m_defaultLocation = defaultLocation;
    m_extraVariables = extraVariables;
    std::stable_sort(factories.begin(), factories.end(), wizardFactoryLessThan);

    m_model->clear();
    QStandardItem *parentItem = m_model->invisibleRootItem();

    QStandardItem *projectKindItem = new QStandardItem(Tr::tr("Projects"));
    projectKindItem->setData(IWizardFactory::ProjectWizard, Qt::UserRole);
    projectKindItem->setFlags({}); // disable item to prevent focus
    QStandardItem *filesKindItem = new QStandardItem(Tr::tr("Files and Classes"));
    filesKindItem->setData(IWizardFactory::FileWizard, Qt::UserRole);
    filesKindItem->setFlags({}); // disable item to prevent focus

    parentItem->appendRow(projectKindItem);
    parentItem->appendRow(filesKindItem);

    const QSet<Id> availablePlatforms = IWizardFactory::allAvailablePlatforms();

    const bool allowAllTemplates = ICore::settings()->value(ALLOW_ALL_TEMPLATES, true).toBool();
    if (allowAllTemplates)
        m_comboBox->addItem(Tr::tr("All Templates"), Id().toSetting());

    for (Id platform : availablePlatforms) {
        const QString displayNameForPlatform = IWizardFactory::displayNameForPlatform(platform);
        m_comboBox->addItem(Tr::tr("%1 Templates").arg(displayNameForPlatform), platform.toSetting());
    }

    m_comboBox->setCurrentIndex(0); // "All templates"
    m_comboBox->setEnabled(!availablePlatforms.isEmpty());

    const bool showPlatformFilter = ICore::settings()->value(SHOW_PLATOFORM_FILTER, true).toBool();
    if (!showPlatformFilter)
        m_comboBox->hide();

    for (IWizardFactory *factory : std::as_const(factories)) {
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
    if (filesKindItem->columnCount() == 0)
        parentItem->removeRow(1);
    if (projectKindItem->columnCount() == 0)
        parentItem->removeRow(0);
}

void NewDialogWidget::showDialog()
{
    QModelIndex idx;

    QString lastPlatform = ICore::settings()->value(LAST_PLATFORM_KEY).toString();
    QString lastCategory = ICore::settings()->value(LAST_CATEGORY_KEY).toString();

    if (!lastPlatform.isEmpty()) {
        int index = m_comboBox->findData(lastPlatform);
        if (index != -1)
            m_comboBox->setCurrentIndex(index);
    }

    static_cast<PlatformFilterProxyModel *>(m_filterProxyModel)->manualReset();

    if (!lastCategory.isEmpty())
        for (QStandardItem *item : std::as_const(m_categoryItems)) {
            if (item->data(Qt::UserRole) == lastCategory)
                idx = m_filterProxyModel->mapFromSource(m_model->indexFromItem(item));
    }
    if (!idx.isValid())
        idx = m_filterProxyModel->index(0,0, m_filterProxyModel->index(0,0));

    m_templateCategoryView->setCurrentIndex(idx);

    // We need to ensure that the category has default focus
    m_templateCategoryView->setFocus(Qt::NoFocusReason);

    for (int row = 0; row < m_filterProxyModel->rowCount(); ++row)
        m_templateCategoryView->setExpanded(m_filterProxyModel->index(row, 0), true);

    // Ensure that item description is visible on first show
    currentItemChanged(m_filterProxyModel->index(0, 0, m_templatesView->rootIndex()));

    updateOkButton();
    show();
}

Id NewDialogWidget::selectedPlatform() const
{
    const int index = m_comboBox->currentIndex();
    return Id::fromSetting(m_comboBox->itemData(index));
}

bool NewDialogWidget::event(QEvent *event)
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

NewDialogWidget::~NewDialogWidget()
{
}

IWizardFactory *NewDialogWidget::currentWizardFactory() const
{
    QModelIndex index = m_filterProxyModel->mapToSource(m_templatesView->currentIndex());
    return factoryOfItem(m_model->itemFromIndex(index));
}

void NewDialogWidget::addItem(QStandardItem *topLevelCategoryItem, IWizardFactory *factory)
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

    QStandardItem *wizardItem = new QStandardItem(factory->icon(), factory->displayName());
    wizardItem->setData(QVariant::fromValue(WizardFactoryContainer(factory, 0)), Qt::UserRole);
    wizardItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    categoryItem->appendRow(wizardItem);

}

void NewDialogWidget::currentCategoryChanged(const QModelIndex &index)
{
    if (index.parent() != m_model->invisibleRootItem()->index()) {
        QModelIndex sourceIndex = m_filterProxyModel->mapToSource(index);
        sourceIndex = m_filterProxyModel->mapFromSource(sourceIndex);
        m_templatesView->setRootIndex(sourceIndex);
        // Focus the first item by default
        m_templatesView->setCurrentIndex(
            m_filterProxyModel->index(0, 0, m_templatesView->rootIndex()));
    }
}

void NewDialogWidget::currentItemChanged(const QModelIndex &index)
{
    QModelIndex sourceIndex = m_filterProxyModel->mapToSource(index);
    QStandardItem* cat = (m_model->itemFromIndex(sourceIndex));
    if (const IWizardFactory *wizard = factoryOfItem(cat)) {
        QString desciption = wizard->description();
        QStringList displayNamesForSupportedPlatforms;
        const QSet<Id> platforms = wizard->supportedPlatforms();
        for (const Id platform : platforms)
            displayNamesForSupportedPlatforms << IWizardFactory::displayNameForPlatform(platform);
        Utils::sort(displayNamesForSupportedPlatforms);
        if (!Qt::mightBeRichText(desciption))
            desciption.replace(QLatin1Char('\n'), QLatin1String("<br>"));
        desciption += QLatin1String("<br><br><b>");
        if (wizard->flags().testFlag(IWizardFactory::PlatformIndependent))
            desciption += Tr::tr("Platform independent") + QLatin1String("</b>");
        else
            desciption += Tr::tr("Supported Platforms")
                    + QLatin1String("</b>: <ul>")
                    + "<li>" + displayNamesForSupportedPlatforms.join("</li><li>") + "</li>"
                    + QLatin1String("</ul>");

        m_templateDescription->setHtml(desciption);

        if (!wizard->descriptionImage().isEmpty()) {
            m_imageLabel->setVisible(true);
            m_imageLabel->setPixmap(wizard->descriptionImage());
        } else {
            m_imageLabel->setVisible(false);
        }

    } else {
        m_templateDescription->clear();
    }
    updateOkButton();
}

void NewDialogWidget::saveState()
{
    const QModelIndex filterIdx = m_templateCategoryView->currentIndex();
    const QModelIndex idx = m_filterProxyModel->mapToSource(filterIdx);
    QStandardItem *currentItem = m_model->itemFromIndex(idx);
    if (currentItem)
        ICore::settings()->setValue(LAST_CATEGORY_KEY, currentItem->data(Qt::UserRole));
    ICore::settings()->setValueWithDefault(LAST_PLATFORM_KEY, m_comboBox->currentData().toString());
}

static void runWizard(IWizardFactory *wizard, const FilePath &defaultLocation, Id platform,
                      const QVariantMap &variables)
{
    const FilePath path = wizard->runPath(defaultLocation);
    wizard->runWizard(path, ICore::dialogParent(), platform, variables);
}

void NewDialogWidget::accept()
{
    saveState();
    if (m_templatesView->currentIndex().isValid()) {
        IWizardFactory *wizard = currentWizardFactory();
        if (QTC_GUARD(wizard)) {
            QMetaObject::invokeMethod(wizard, std::bind(&runWizard, wizard, m_defaultLocation,
                                      selectedPlatform(), m_extraVariables), Qt::QueuedConnection);
        }
    }
    QDialog::accept();
}

void NewDialogWidget::reject()
{
    saveState();
    QDialog::reject();
}

void NewDialogWidget::updateOkButton()
{
    m_okButton->setEnabled(currentWizardFactory() != nullptr);
}

void NewDialogWidget::setSelectedPlatform(int /*platform*/)
{
    //The static cast allows us to keep PlatformFilterProxyModel anonymous
    static_cast<PlatformFilterProxyModel *>(m_filterProxyModel)->setPlatform(selectedPlatform());
}

} // Core::Internal
