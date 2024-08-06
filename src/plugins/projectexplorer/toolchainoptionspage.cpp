// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolchainoptionspage.h"

#include "abi.h"
#include "devicesupport/devicemanager.h"
#include "kitoptionspage.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "toolchain.h"
#include "toolchainconfigwidget.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/guard.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QTextStream>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class ToolChainTreeItem : public TreeItem
{
public:
    ToolChainTreeItem(QStackedWidget *parentWidget, const ToolchainBundle &bundle, bool c) :
        bundle(bundle), changed(c), m_parentWidget(parentWidget)
    {}

    QVariant data(int column, int role) const override
    {
        switch (role) {
            case Qt::DisplayRole:
                if (column == 0)
                    return bundle.displayName();
                return bundle.typeDisplayName();
            case Qt::FontRole: {
                QFont font;
                font.setBold(changed);
                return font;
             }
            case Qt::ToolTipRole: {
                QString toolTip;
                const ToolchainBundle::Valid validity = bundle.validity();
                if (validity != ToolchainBundle::Valid::None) {
                    toolTip = Tr::tr("<nobr><b>ABI:</b> %1").arg(
                                changed ? Tr::tr("not up-to-date")
                                        : bundle.targetAbi().toString());
                    if (validity == ToolchainBundle::Valid::Some)
                        toolTip.append("<br/>").append(
                            Tr::tr("Not all compilers are set up correctly."));
                } else {
                    toolTip = Tr::tr("This toolchain is invalid.");
                }
                return QVariant("<div style=\"white-space:pre\">" + toolTip + "</div>");
            }
            case Qt::DecorationRole:
                if (column == 0) {
                    switch (bundle.validity()) {
                    case ToolchainBundle::Valid::All:
                        break;
                    case ToolchainBundle::Valid::Some:
                        return Utils::Icons::WARNING.icon();
                    case ToolchainBundle::Valid::None:
                        return Utils::Icons::CRITICAL.icon();
                    }
                }
                return QVariant();
        }
        return {};
    }

    ToolchainConfigWidget *widget()
    {
        if (!m_widget) {
           m_widget = bundle.factory()->createConfigurationWidget(bundle).release();
           if (m_widget) {
                m_parentWidget->addWidget(m_widget);
                if (bundle.isAutoDetected())
                    m_widget->makeReadOnly();
                QObject::connect(m_widget, &ToolchainConfigWidget::dirty,
                                 [this] {
                    changed = true;
                    update();
                });
            }
        }
        return m_widget;
    }

    ToolchainBundle bundle;
    bool changed;

private:
    ToolchainConfigWidget *m_widget = nullptr;
    QStackedWidget *m_parentWidget = nullptr;
};

class DetectionSettingsDialog : public QDialog
{
public:
    DetectionSettingsDialog(const ToolchainDetectionSettings &settings, QWidget *parent)
        : QDialog(parent)
    {
        setWindowTitle(Tr::tr("Toolchain Auto-detection Settings"));
        const auto layout = new QVBoxLayout(this);
        m_detectX64AsX32CheckBox.setText(Tr::tr("Detect x86_64 GCC compilers "
                                                "as x86_64 and x86"));
        m_detectX64AsX32CheckBox.setToolTip(
            Tr::tr("If checked, %1 will "
                   "set up two instances of each x86_64 compiler:\nOne for the native x86_64 "
                   "target, and "
                   "one for a plain x86 target.\nEnable this if you plan to create 32-bit x86 "
                   "binaries "
                   "without using a dedicated cross compiler.")
                .arg(QGuiApplication::applicationDisplayName()));
        m_detectX64AsX32CheckBox.setChecked(settings.detectX64AsX32);
        layout->addWidget(&m_detectX64AsX32CheckBox);
        const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttonBox);
    }

    ToolchainDetectionSettings settings() const
    {
        ToolchainDetectionSettings s;
        s.detectX64AsX32 = m_detectX64AsX32CheckBox.isChecked();
        return s;
    }

private:
    QCheckBox m_detectX64AsX32CheckBox;
};

// --------------------------------------------------------------------------
// ToolChainOptionsWidget
// --------------------------------------------------------------------------

class ToolChainOptionsWidget final : public Core::IOptionsPageWidget
{
public:
    ToolChainOptionsWidget()
    {
        m_detectionSettings = ToolchainManager::detectionSettings();
        m_factories = Utils::filtered(ToolchainFactory::allToolchainFactories(),
                    [](ToolchainFactory *factory) { return factory->canCreate();});

        m_model.setHeader({Tr::tr("Name"), Tr::tr("Type")});
        auto autoRoot = new StaticTreeItem({ProjectExplorer::Constants::msgAutoDetected()},
                                           {ProjectExplorer::Constants::msgAutoDetectedToolTip()});
        auto manualRoot = new StaticTreeItem(ProjectExplorer::Constants::msgManual());

        for (const LanguageCategory &category : ToolchainManager::languageCategories()) {
            const QString dn = ToolchainManager::displayNameOfLanguageCategory(category);
            auto autoNode = new StaticTreeItem(dn);
            auto manualNode = new StaticTreeItem(dn);

            autoRoot->appendChild(autoNode);
            manualRoot->appendChild(manualNode);

            m_languageMap.insert(category, {autoNode, manualNode});
        }

        m_model.rootItem()->appendChild(autoRoot);
        m_model.rootItem()->appendChild(manualRoot);

        m_toolChainView = new QTreeView(this);
        m_toolChainView->setUniformRowHeights(true);
        m_toolChainView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_toolChainView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_sortModel.setSourceModel(&m_model);
        m_sortModel.setSortedCategories({Constants::msgAutoDetected(), Constants::msgManual()});
        m_toolChainView->setModel(&m_sortModel);
        m_toolChainView->setSortingEnabled(true);
        m_toolChainView->sortByColumn(0, Qt::AscendingOrder);
        m_toolChainView->header()->setStretchLastSection(false);
        m_toolChainView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_toolChainView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_toolChainView->expandAll();

        m_addButton = new QPushButton(Tr::tr("Add"), this);
        auto addMenu = new QMenu(this);
        for (ToolchainFactory *factory : std::as_const(m_factories)) {
            QList<Utils::Id> languages = factory->supportedLanguages();
            if (languages.isEmpty())
                continue;

            addMenu->addAction(createAction(factory->displayName(), factory, languages));
        }
        m_addButton->setMenu(addMenu);
        if (HostOsInfo::isMacHost())
            m_addButton->setStyleSheet("text-align:center;");

        m_cloneButton = new QPushButton(Tr::tr("Clone"), this);
        connect(m_cloneButton, &QAbstractButton::clicked, this, [this] { cloneToolchains(); });

        m_delButton = new QPushButton(Tr::tr("Remove"), this);

        m_removeAllButton = new QPushButton(Tr::tr("Remove All"), this);
        connect(m_removeAllButton, &QAbstractButton::clicked, this,
                [this] {
            QList<ToolChainTreeItem *> itemsToRemove;
            m_model.forAllItems([&itemsToRemove](TreeItem *item) {
                if (item->level() != 3)
                    return;
                const auto tcItem = static_cast<ToolChainTreeItem *>(item);
                if (!tcItem->bundle.isSdkProvided())
                    itemsToRemove << tcItem;
            });
            for (ToolChainTreeItem * const tcItem : std::as_const(itemsToRemove))
                markForRemoval(tcItem);
        });

        m_redetectButton = new QPushButton(Tr::tr("Re-detect"), this);
        connect(m_redetectButton, &QAbstractButton::clicked,
                this, &ToolChainOptionsWidget::redetectToolchains);

        m_detectionSettingsButton = new QPushButton(Tr::tr("Auto-detection Settings..."), this);
        connect(m_detectionSettingsButton, &QAbstractButton::clicked, this,
                [this] {
            DetectionSettingsDialog dlg(m_detectionSettings, this);
            if (dlg.exec() == QDialog::Accepted)
                m_detectionSettings = dlg.settings();
        });

        m_container = new DetailsWidget(this);
        m_container->setState(DetailsWidget::NoSummary);
        m_container->setVisible(false);

        m_widgetStack = new QStackedWidget;
        m_container->setWidget(m_widgetStack);

        const QList<ToolchainBundle> bundles = ToolchainBundle::collectBundles();
        for (const ToolchainBundle &b : bundles) {
            ToolchainManager::registerToolchains(b.createdToolchains());
            insertBundle(b);
        }

        auto buttonLayout = new QVBoxLayout;
        buttonLayout->setSpacing(6);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->addWidget(m_addButton);
        buttonLayout->addWidget(m_cloneButton);
        buttonLayout->addWidget(m_delButton);
        buttonLayout->addWidget(m_removeAllButton);
        buttonLayout->addWidget(m_redetectButton);
        buttonLayout->addWidget(m_detectionSettingsButton);
        buttonLayout->addItem(new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        auto verticalLayout = new QVBoxLayout;
        verticalLayout->addWidget(m_toolChainView);
        verticalLayout->addWidget(m_container);

        auto horizontalLayout = new QHBoxLayout(this);
        horizontalLayout->addLayout(verticalLayout);
        horizontalLayout->addLayout(buttonLayout);

        connect(ToolchainManager::instance(), &ToolchainManager::toolchainsRegistered,
                this, &ToolChainOptionsWidget::handleToolchainsRegistered);
        connect(ToolchainManager::instance(), &ToolchainManager::toolchainsDeregistered,
                this, &ToolChainOptionsWidget::handleToolchainsDeregistered);

        connect(m_toolChainView->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &ToolChainOptionsWidget::toolChainSelectionChanged);
        connect(ToolchainManager::instance(), &ToolchainManager::toolchainsChanged,
                this, &ToolChainOptionsWidget::toolChainSelectionChanged);

        connect(m_delButton, &QAbstractButton::clicked, this, [this] {
            if (ToolChainTreeItem *item = currentTreeItem())
                markForRemoval(item);
        });

        updateState();
    }

    void toolChainSelectionChanged();
    void updateState();
    void createToolchains(ToolchainFactory *factory, const QList<Id> &languages);
    void cloneToolchains();
    ToolChainTreeItem *currentTreeItem();

    void markForRemoval(ToolChainTreeItem *item);
    ToolChainTreeItem *insertBundle(const ToolchainBundle &bundle, bool changed = false); // Insert directly into model
    void handleToolchainsRegistered(const Toolchains &toolchains);
    void handleToolchainsDeregistered(const Toolchains &toolchains);

    StaticTreeItem *rootItem(const LanguageCategory &languageCategory, bool autoDetected);
    StaticTreeItem *parentForBundle(const ToolchainBundle &bundle);
    StaticTreeItem *parentForToolchain(const Toolchain &tc);
    QAction *createAction(const QString &name, ToolchainFactory *factory, const QList<Id> &languages)
    {
        auto action = new QAction(name, this);
        connect(action, &QAction::triggered, this,
                [this, factory, languages] { createToolchains(factory, languages); });
        return action;
    }

    void redetectToolchains();

    void apply() final;

 private:
    TreeModel<TreeItem, ToolChainTreeItem> m_model;
    KitSettingsSortModel m_sortModel;
    QList<ToolchainFactory *> m_factories;
    QTreeView *m_toolChainView;
    DetailsWidget *m_container;
    QStackedWidget *m_widgetStack;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
    QPushButton *m_removeAllButton;
    QPushButton *m_redetectButton;
    QPushButton *m_detectionSettingsButton;

    QHash<LanguageCategory, QPair<StaticTreeItem *, StaticTreeItem *>> m_languageMap;

    using AddRemoveList = QList<ToolChainTreeItem *>;
    AddRemoveList m_toAddList;
    AddRemoveList m_toRemoveList;
    Guard m_registerGuard;
    Guard m_deregisterGuard;

    ToolchainDetectionSettings m_detectionSettings;
};

void ToolChainOptionsWidget::markForRemoval(ToolChainTreeItem *item)
{
    m_model.takeItem(item);
    if (const auto it = std::find(m_toAddList.begin(), m_toAddList.end(), item);
        it != m_toAddList.end()) {
        item->bundle.deleteToolchains();
        m_toAddList.erase(it);
        delete item;
    } else {
        m_toRemoveList.append(item);
    }
}

ToolChainTreeItem *ToolChainOptionsWidget::insertBundle(
    const ToolchainBundle &bundle, bool changed)
{
    StaticTreeItem *parent = parentForBundle(bundle);
    auto item = new ToolChainTreeItem(m_widgetStack, bundle, changed);
    parent->appendChild(item);

    return item;
}

void ToolChainOptionsWidget::handleToolchainsRegistered(const Toolchains &toolchains)
{
    if (m_registerGuard.isLocked())
        return;
    GuardLocker locker(m_registerGuard);

    if (const auto it = std::find_if(
            m_toAddList.begin(),
            m_toAddList.end(),
            [&toolchains](ToolChainTreeItem * const item) {
                return item->bundle.bundleId() == toolchains.first()->bundleId();
            });
        it != m_toAddList.end()) {
        if ((*it)->bundle.toolchains().size() == toolchains.size())
            m_toAddList.erase(it);
        return;
    }

    const QList<ToolchainBundle> bundles = ToolchainBundle::collectBundles(toolchains);
    for (const ToolchainBundle &bundle : bundles) {
        ToolchainManager::registerToolchains(bundle.createdToolchains());
        insertBundle(bundle);
    }
    updateState();
}

void ToolChainOptionsWidget::handleToolchainsDeregistered(const Toolchains &toolchains)
{
    if (m_deregisterGuard.isLocked())
        return;
    GuardLocker locker(m_deregisterGuard);

    if (const auto it = std::find_if(
            m_toRemoveList.begin(),
            m_toRemoveList.end(),
            [&toolchains](const ToolChainTreeItem *item) {
                return item->bundle.toolchains() == toolchains;
            });
        it != m_toRemoveList.end()) {
        ToolChainTreeItem * const item = *it;
        m_toRemoveList.erase(it);
        delete item;
        return;
    }

    QSet<ToolChainTreeItem *> affectedItems;
    for (Toolchain * const tc : toolchains) {
        StaticTreeItem *parent = parentForToolchain(*tc);
        auto item = static_cast<ToolChainTreeItem *>(
            parent->findChildAtLevel(1, [tc](TreeItem *item) {
                const auto tcItem = static_cast<ToolChainTreeItem *>(item);
                return tcItem->bundle.size() > 0 && tcItem->bundle.bundleId() == tc->bundleId();
            }));
        const bool removed = item->bundle.removeToolchain(tc);
        QTC_CHECK(removed);
        affectedItems << item;
    }

    for (ToolChainTreeItem *item : std::as_const(affectedItems)) {
        ToolchainManager::deregisterToolchains(item->bundle.toolchains());
        item->bundle.clearToolchains();
        m_model.destroyItem(item);
    }

    updateState();
}

StaticTreeItem *ToolChainOptionsWidget::rootItem(
    const LanguageCategory &languageCategory, bool autoDetected)
{
    QPair<StaticTreeItem *, StaticTreeItem *> nodes = m_languageMap.value(languageCategory);
    return autoDetected ? nodes.first : nodes.second;
}

StaticTreeItem *ToolChainOptionsWidget::parentForBundle(const ToolchainBundle &bundle)
{
    return rootItem(bundle.factory()->languageCategory(), bundle.isAutoDetected());
}

StaticTreeItem *ToolChainOptionsWidget::parentForToolchain(const Toolchain &tc)
{
    return rootItem(tc.factory()->languageCategory(), tc.isAutoDetected());
}

void ToolChainOptionsWidget::redetectToolchains()
{
    // The second element is the set of toolchains for the respective bundle that were re-discovered.
    using ItemToCheck = std::pair<ToolChainTreeItem *, Toolchains>;
    QList<ItemToCheck> itemsToRemove;

    Toolchains knownTcs;

    // Step 1: All previously auto-detected items are candidates for removal.
    m_model.forAllItems([&itemsToRemove, &knownTcs](TreeItem *item) {
        if (item->level() != 3)
            return;
        const auto tcItem = static_cast<ToolChainTreeItem *>(item);
        if (tcItem->bundle.isAutoDetected() && !tcItem->bundle.isSdkProvided())
            itemsToRemove << std::make_pair(tcItem, Toolchains());
        else
            knownTcs << tcItem->bundle.toolchains();
    });

    Toolchains toAdd;
    ToolchainManager::resetBadToolchains();

    // Step 2: Re-detect toolchains.
    for (ToolchainFactory *f : ToolchainFactory::allToolchainFactories()) {
        const ToolchainDetector detector(knownTcs, DeviceManager::defaultDesktopDevice(), {});  // FIXME: Pass search paths
        for (Toolchain * const tc : f->autoDetect(detector)) {
            if (knownTcs.contains(tc))
                continue;
            knownTcs << tc;
            const auto matchItem = [&](const ItemToCheck &item) {
                return Utils::contains(item.first->bundle.toolchains(), [&](Toolchain *btc) {
                    return *btc == *tc;
                });
            };
            if (const auto item
                = std::find_if(itemsToRemove.begin(), itemsToRemove.end(), matchItem);
                item != itemsToRemove.end()) {
                item->second << tc;
                continue;
            }
            toAdd << tc;
        }
    }

    // Step 3: Items whose toolchains were all re-discovered are no longer candidates for removal.
    //    Instead, delete the re-discovered toolchains.
    //    Conversely, if not all toolchains of the bundle were re-discovered, we remove the existing
    //    item and the newly discovered toolchains are marked for re-bundling.
    for (const auto &[item, newToolchains] : itemsToRemove) {
        if (item->bundle.toolchains().size() == newToolchains.size()) {
            qDeleteAll(newToolchains);
        } else {
            toAdd << newToolchains;
            markForRemoval(item);
        }
    }

    // Step 4: Create new bundles and add items for them.
    const QList<ToolchainBundle> newBundles = ToolchainBundle::collectBundles(toAdd);
    for (const ToolchainBundle &bundle : newBundles)
        m_toAddList << insertBundle(bundle, true);
}

void ToolChainOptionsWidget::toolChainSelectionChanged()
{
    ToolChainTreeItem *item = currentTreeItem();

    QWidget *currentTcWidget = item ? item->widget() : nullptr;
    if (currentTcWidget)
        m_widgetStack->setCurrentWidget(currentTcWidget);
    m_container->setVisible(currentTcWidget);
    updateState();
}

void ToolChainOptionsWidget::apply()
{
    // Remove unused tool chains:
    const AddRemoveList toRemove = m_toRemoveList;
    for (const ToolChainTreeItem * const item : toRemove)
        ToolchainManager::deregisterToolchains(item->bundle.toolchains());

    Q_ASSERT(m_toRemoveList.isEmpty());

    // Update tool chains:
    for (const QPair<StaticTreeItem *, StaticTreeItem *> &autoAndManual : m_languageMap) {
        for (StaticTreeItem *parent : {autoAndManual.first, autoAndManual.second}) {
            for (TreeItem *item : *parent) {
                auto tcItem = static_cast<ToolChainTreeItem *>(item);
                if (!tcItem->bundle.isAutoDetected() && tcItem->widget() && tcItem->changed)
                    tcItem->widget()->apply();
                tcItem->changed = false;
                tcItem->update();
            }
        }
    }

    // Add new (and already updated) toolchains
    QStringList removedTcs;
    const AddRemoveList toAdd = m_toAddList;
    for (ToolChainTreeItem * const item : toAdd) {
        const Toolchains notRegistered = ToolchainManager::registerToolchains(item->bundle.toolchains());
        removedTcs << Utils::transform(notRegistered, &Toolchain::displayName);
    }
    for (ToolChainTreeItem * const item : std::as_const(m_toAddList)) {
        item->bundle.deleteToolchains();
        delete item;
    }
    m_toAddList.clear();

    if (removedTcs.count() == 1) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             Tr::tr("Duplicate Compilers Detected"),
                             Tr::tr("The following compiler was already configured:<br>"
                                    "&nbsp;%1<br>"
                                    "It was not configured again.")
                                 .arg(removedTcs.at(0)));

    } else if (!removedTcs.isEmpty()) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             Tr::tr("Duplicate Compilers Detected"),
                             Tr::tr("The following compilers were already configured:<br>"
                                    "&nbsp;%1<br>"
                                    "They were not configured again.")
                                 .arg(removedTcs.join(QLatin1String(",<br>&nbsp;"))));
    }
    ToolchainManager::setDetectionSettings(m_detectionSettings);
}

void ToolChainOptionsWidget::createToolchains(ToolchainFactory *factory, const QList<Id> &languages)
{
    QTC_ASSERT(factory, return);
    QTC_ASSERT(factory->canCreate(), return);

    const Id bundleId = Id::generate();
    Toolchains toolchains;
    for (const Id lang : languages) {
        Toolchain *tc = factory->create();
        QTC_ASSERT(tc, return);

        tc->setDetection(Toolchain::ManualDetection);
        tc->setLanguage(lang);
        tc->setBundleId(bundleId);
        toolchains << tc;
    }

    const ToolchainBundle bundle(toolchains);
    ToolChainTreeItem * const item = insertBundle(bundle, true);
    m_toAddList << item;
    m_toolChainView->setCurrentIndex(m_sortModel.mapFromSource(m_model.indexForItem(item)));
}

void ToolChainOptionsWidget::cloneToolchains()
{
    ToolChainTreeItem *current = currentTreeItem();
    if (!current)
        return;

    ToolchainBundle bundle = current->bundle.clone();
    bundle.setDetection(Toolchain::ManualDetection);
    bundle.setDisplayName(Tr::tr("Clone of %1").arg(current->bundle.displayName()));

    ToolChainTreeItem * const item = insertBundle(bundle, true);
    m_toAddList << item;
    m_toolChainView->setCurrentIndex(m_sortModel.mapFromSource(m_model.indexForItem(item)));
}

void ToolChainOptionsWidget::updateState()
{
    bool canCopy = false;
    bool canDelete = false;
    if (ToolChainTreeItem *item = currentTreeItem()) {
        canCopy = item->bundle.validity() != ToolchainBundle::Valid::None;
        canDelete = !item->bundle.isSdkProvided();
    }

    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
}

ToolChainTreeItem *ToolChainOptionsWidget::currentTreeItem()
{
    TreeItem *item = m_model.itemForIndex(m_sortModel.mapToSource(m_toolChainView->currentIndex()));
    return (item && item->level() == 3) ? static_cast<ToolChainTreeItem *>(item) : nullptr;
}

// --------------------------------------------------------------------------
// ToolChainOptionsPage
// --------------------------------------------------------------------------

ToolChainOptionsPage::ToolChainOptionsPage()
{
    setId(Constants::TOOLCHAIN_SETTINGS_PAGE_ID);
    setDisplayName(Tr::tr("Compilers"));
    setCategory(Constants::KITS_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new ToolChainOptionsWidget; });
}

} // namespace Internal
} // namespace ProjectExplorer
