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

#include "toolchainoptionspage.h"
#include "toolchain.h"
#include "abi.h"
#include "projectexplorerconstants.h"
#include "projectexplorericons.h"
#include "toolchainconfigwidget.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
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
    ToolChainTreeItem(QStackedWidget *parentWidget, ToolChain *tc, bool c) :
        toolChain(tc), changed(c)
    {
        widget = tc->createConfigurationWidget().release();
        if (widget) {
            parentWidget->addWidget(widget);
            if (tc->isAutoDetected())
                widget->makeReadOnly();
            QObject::connect(widget, &ToolChainConfigWidget::dirty,
                             [this] {
                changed = true;
                update();
            });
        }
    }

    QVariant data(int column, int role) const override
    {
        switch (role) {
            case Qt::DisplayRole:
                if (column == 0)
                    return toolChain->displayName();
                return toolChain->typeDisplayName();
            case Qt::FontRole: {
                QFont font;
                font.setBold(changed);
                return font;
             }
            case Qt::ToolTipRole:
                if (!toolChain->isValid())
                    return ToolChainOptionsPage::tr("This toolchain is invalid.");
                return ToolChainOptionsPage::tr("<nobr><b>ABI:</b> %1").arg(
                    changed ? ToolChainOptionsPage::tr("not up-to-date")
                            : toolChain->targetAbi().toString());
            case Qt::DecorationRole:
                return column == 0 && !toolChain->isValid()
                        ? Utils::Icons::CRITICAL.icon() : QVariant();
        }
        return QVariant();
    }

    ToolChain *toolChain;
    ToolChainConfigWidget *widget;
    bool changed;
};

class DetectionSettingsDialog : public QDialog
{
public:
    DetectionSettingsDialog(const ToolchainDetectionSettings &settings, QWidget *parent)
        : QDialog(parent)
    {
        setWindowTitle(ToolChainOptionsPage::tr("Toolchain Auto-detection Settings"));
        const auto layout = new QVBoxLayout(this);
        m_detectX64AsX32CheckBox.setText(ToolChainOptionsPage::tr("Detect x86_64 GCC compilers "
                                                                  "as x86_64 and x86"));
        m_detectX64AsX32CheckBox.setToolTip(ToolChainOptionsPage::tr("If checked, Qt Creator will "
            "set up two instances of each x86_64 compiler:\nOne for the native x86_64 target, "
            "and one for a plain x86 target.\nEnable this if you plan to create 32-bit x86 "
            "binaries without using a dedicated cross compiler."));
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
        m_detectionSettings = ToolChainManager::detectionSettings();
        m_factories = Utils::filtered(ToolChainFactory::allToolChainFactories(),
                    [](ToolChainFactory *factory) { return factory->canCreate();});

        m_model.setHeader({ToolChainOptionsPage::tr("Name"), ToolChainOptionsPage::tr("Type")});
        auto autoRoot = new StaticTreeItem(ToolChainOptionsPage::tr("Auto-detected"));
        auto manualRoot = new StaticTreeItem(ToolChainOptionsPage::tr("Manual"));

        foreach (const Utils::Id &l, ToolChainManager::allLanguages()) {
            const QString dn = ToolChainManager::displayNameOfLanguageId(l);
            auto autoNode = new StaticTreeItem(dn);
            auto manualNode = new StaticTreeItem(dn);

            autoRoot->appendChild(autoNode);
            manualRoot->appendChild(manualNode);

            m_languageMap.insert(l, qMakePair(autoNode, manualNode));
        }

        m_model.rootItem()->appendChild(autoRoot);
        m_model.rootItem()->appendChild(manualRoot);

        m_toolChainView = new QTreeView(this);
        m_toolChainView->setUniformRowHeights(true);
        m_toolChainView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_toolChainView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_toolChainView->setModel(&m_model);
        m_toolChainView->header()->setStretchLastSection(false);
        m_toolChainView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_toolChainView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_toolChainView->expandAll();

        m_addButton = new QPushButton(ToolChainOptionsPage::tr("Add"), this);
        auto addMenu = new QMenu;
        foreach (ToolChainFactory *factory, m_factories) {
            QList<Utils::Id> languages = Utils::toList(factory->supportedLanguages());
            if (languages.isEmpty())
                continue;

            if (languages.count() == 1) {
                addMenu->addAction(createAction(factory->displayName(), factory, languages.at(0)));
            } else {
                Utils::sort(languages, [](const Utils::Id &l1, const Utils::Id &l2) {
                                return ToolChainManager::displayNameOfLanguageId(l1) < ToolChainManager::displayNameOfLanguageId(l2);
                            });
                auto subMenu = addMenu->addMenu(factory->displayName());
                foreach (const Utils::Id &l, languages)
                    subMenu->addAction(createAction(ToolChainManager::displayNameOfLanguageId(l), factory, l));
            }
        }
        m_addButton->setMenu(addMenu);

        m_cloneButton = new QPushButton(ToolChainOptionsPage::tr("Clone"), this);
        connect(m_cloneButton, &QAbstractButton::clicked, [this] { cloneToolChain(); });

        m_delButton = new QPushButton(ToolChainOptionsPage::tr("Remove"), this);

        m_removeAllButton = new QPushButton(ToolChainOptionsPage::tr("Remove All"), this);
        connect(m_removeAllButton, &QAbstractButton::clicked, this,
                [this] {
            QList<ToolChainTreeItem *> itemsToRemove;
            m_model.forAllItems([&itemsToRemove](TreeItem *item) {
                if (item->level() != 3)
                    return;
                const auto tcItem = static_cast<ToolChainTreeItem *>(item);
                if (tcItem->toolChain->detection() != ToolChain::AutoDetectionFromSdk)
                    itemsToRemove << tcItem;
            });
            for (ToolChainTreeItem * const tcItem : qAsConst(itemsToRemove))
                markForRemoval(tcItem);
        });

        m_redetectButton = new QPushButton(ToolChainOptionsPage::tr("Re-detect"), this);
        connect(m_redetectButton, &QAbstractButton::clicked,
                this, &ToolChainOptionsWidget::redetectToolchains);

        m_detectionSettingsButton = new QPushButton(
                    ToolChainOptionsPage::tr("Auto-detection Settings..."), this);
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

        foreach (ToolChain *tc, ToolChainManager::toolChains())
            insertToolChain(tc);

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

        connect(ToolChainManager::instance(), &ToolChainManager::toolChainAdded,
                this, &ToolChainOptionsWidget::addToolChain);
        connect(ToolChainManager::instance(), &ToolChainManager::toolChainRemoved,
                this, &ToolChainOptionsWidget::removeToolChain);

        connect(m_toolChainView->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &ToolChainOptionsWidget::toolChainSelectionChanged);
        connect(ToolChainManager::instance(), &ToolChainManager::toolChainsChanged,
                this, &ToolChainOptionsWidget::toolChainSelectionChanged);

        connect(m_delButton, &QAbstractButton::clicked, [this] {
            if (ToolChainTreeItem *item = currentTreeItem())
                markForRemoval(item);
        });

        updateState();
    }

    void toolChainSelectionChanged();
    void updateState();
    void createToolChain(ToolChainFactory *factory, const Utils::Id &language);
    void cloneToolChain();
    ToolChainTreeItem *currentTreeItem();

    void markForRemoval(ToolChainTreeItem *item);
    ToolChainTreeItem *insertToolChain(ProjectExplorer::ToolChain *tc, bool changed = false); // Insert directly into model
    void addToolChain(ProjectExplorer::ToolChain *);
    void removeToolChain(ProjectExplorer::ToolChain *);

    StaticTreeItem *parentForToolChain(ToolChain *tc);
    QAction *createAction(const QString &name, ToolChainFactory *factory, Utils::Id language)
    {
        auto action = new QAction(name, nullptr);
        connect(action, &QAction::triggered, [this, factory, language] { createToolChain(factory, language); });
        return action;
    }

    void redetectToolchains();

    void apply();

 private:
    TreeModel<TreeItem, ToolChainTreeItem> m_model;
    QList<ToolChainFactory *> m_factories;
    QTreeView *m_toolChainView;
    DetailsWidget *m_container;
    QStackedWidget *m_widgetStack;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
    QPushButton *m_removeAllButton;
    QPushButton *m_redetectButton;
    QPushButton *m_detectionSettingsButton;

    QHash<Utils::Id, QPair<StaticTreeItem *, StaticTreeItem *>> m_languageMap;

    QList<ToolChainTreeItem *> m_toAddList;
    QList<ToolChainTreeItem *> m_toRemoveList;

    ToolchainDetectionSettings m_detectionSettings;
};

void ToolChainOptionsWidget::markForRemoval(ToolChainTreeItem *item)
{
    m_model.takeItem(item);
    if (m_toAddList.contains(item)) {
        delete item->toolChain;
        item->toolChain = nullptr;
        m_toAddList.removeOne(item);
        delete item;
    } else {
        m_toRemoveList.append(item);
    }
}

ToolChainTreeItem *ToolChainOptionsWidget::insertToolChain(ToolChain *tc, bool changed)
{
    StaticTreeItem *parent = parentForToolChain(tc);
    auto item = new ToolChainTreeItem(m_widgetStack, tc, changed);
    parent->appendChild(item);

    return item;
}

void ToolChainOptionsWidget::addToolChain(ToolChain *tc)
{
    foreach (ToolChainTreeItem *n, m_toAddList) {
        if (n->toolChain == tc) {
            // do not delete n: Still used elsewhere!
            m_toAddList.removeOne(n);
            return;
        }
    }

    insertToolChain(tc);

    updateState();
}

void ToolChainOptionsWidget::removeToolChain(ToolChain *tc)
{
    foreach (ToolChainTreeItem *n, m_toRemoveList) {
        if (n->toolChain == tc) {
            m_toRemoveList.removeOne(n);
            delete n;
            return;
        }
    }

    StaticTreeItem *parent = parentForToolChain(tc);
    auto item = parent->findChildAtLevel(1, [tc](TreeItem *item) {
        return static_cast<ToolChainTreeItem *>(item)->toolChain == tc;
    });
    m_model.destroyItem(item);

    updateState();
}

StaticTreeItem *ToolChainOptionsWidget::parentForToolChain(ToolChain *tc)
{
    QPair<StaticTreeItem *, StaticTreeItem *> nodes = m_languageMap.value(tc->language());
    return tc->isAutoDetected() ? nodes.first : nodes.second;
}

void ToolChainOptionsWidget::redetectToolchains()
{
    QList<ToolChainTreeItem *> itemsToRemove;
    QList<ToolChain *> knownTcs;
    m_model.forAllItems([&itemsToRemove, &knownTcs](TreeItem *item) {
        if (item->level() != 3)
            return;
        const auto tcItem = static_cast<ToolChainTreeItem *>(item);
        if (tcItem->toolChain->isAutoDetected()
                && tcItem->toolChain->detection() != ToolChain::AutoDetectionFromSdk) {
            itemsToRemove << tcItem;
        } else {
            knownTcs << tcItem->toolChain;
        }
    });
    QList<ToolChain *> toAdd;
    QSet<ToolChain *> toDelete;
    for (ToolChainFactory *f : ToolChainFactory::allToolChainFactories()) {
        for (ToolChain * const tc : f->autoDetect(knownTcs)) {
            if (knownTcs.contains(tc) || toDelete.contains(tc))
                continue;
            const auto matchItem = [tc](const ToolChainTreeItem *item) {
                return item->toolChain->compilerCommand() == tc->compilerCommand()
                        && item->toolChain->typeId() == tc->typeId()
                        && item->toolChain->language() == tc->language()
                        && item->toolChain->targetAbi() == tc->targetAbi();
            };
            ToolChainTreeItem * const item = findOrDefault(itemsToRemove, matchItem);
            if (item) {
                itemsToRemove.removeOne(item);
                toDelete << tc;
                continue;
            }
            knownTcs << tc;
            toAdd << tc;
        }
    }
    for (ToolChainTreeItem * const tcItem : qAsConst(itemsToRemove))
        markForRemoval(tcItem);
    for (ToolChain * const newTc : qAsConst(toAdd))
        m_toAddList.append(insertToolChain(newTc, true));
    qDeleteAll(toDelete);
}

void ToolChainOptionsWidget::toolChainSelectionChanged()
{
    ToolChainTreeItem *item = currentTreeItem();

    QWidget *currentTcWidget = item ? item->widget : nullptr;
    if (currentTcWidget)
        m_widgetStack->setCurrentWidget(currentTcWidget);
    m_container->setVisible(currentTcWidget);
    updateState();
}

void ToolChainOptionsWidget::apply()
{
    // Remove unused tool chains:
    QList<ToolChainTreeItem *> nodes = m_toRemoveList;
    foreach (ToolChainTreeItem *n, nodes)
        ToolChainManager::deregisterToolChain(n->toolChain);

    Q_ASSERT(m_toRemoveList.isEmpty());

    // Update tool chains:
    foreach (const Utils::Id &l, m_languageMap.keys()) {
        const QPair<StaticTreeItem *, StaticTreeItem *> autoAndManual = m_languageMap.value(l);
        for (StaticTreeItem *parent : {autoAndManual.first, autoAndManual.second}) {
            for (TreeItem *item : *parent) {
                auto tcItem = static_cast<ToolChainTreeItem *>(item);
                Q_ASSERT(tcItem->toolChain);
                if (!tcItem->toolChain->isAutoDetected() && tcItem->widget)
                    tcItem->widget->apply();
                tcItem->changed = false;
                tcItem->update();
            }
        }
    }

    // Add new (and already updated) tool chains
    QStringList removedTcs;
    nodes = m_toAddList;
    foreach (ToolChainTreeItem *n, nodes) {
        if (!ToolChainManager::registerToolChain(n->toolChain))
            removedTcs << n->toolChain->displayName();
    }
    //
    foreach (ToolChainTreeItem *n, m_toAddList)
        markForRemoval(n);

    qDeleteAll(m_toAddList);

    if (removedTcs.count() == 1) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             ToolChainOptionsPage::tr("Duplicate Compilers Detected"),
                             ToolChainOptionsPage::tr("The following compiler was already configured:<br>"
                                                      "&nbsp;%1<br>"
                                                      "It was not configured again.")
                                                      .arg(removedTcs.at(0)));

    } else if (!removedTcs.isEmpty()) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             ToolChainOptionsPage::tr("Duplicate Compilers Detected"),
                             ToolChainOptionsPage::tr("The following compilers were already configured:<br>"
                                                      "&nbsp;%1<br>"
                                                      "They were not configured again.")
                                                      .arg(removedTcs.join(QLatin1String(",<br>&nbsp;"))));
    }
    ToolChainManager::setDetectionSettings(m_detectionSettings);
}

void ToolChainOptionsWidget::createToolChain(ToolChainFactory *factory, const Utils::Id &language)
{
    QTC_ASSERT(factory, return);
    QTC_ASSERT(factory->canCreate(), return);
    QTC_ASSERT(language.isValid(), return);

    ToolChain *tc = factory->create();
    if (!tc)
        return;

    tc->setDetection(ToolChain::ManualDetection);
    tc->setLanguage(language);

    auto item = insertToolChain(tc, true);
    m_toAddList.append(item);

    m_toolChainView->setCurrentIndex(m_model.indexForItem(item));
}

void ToolChainOptionsWidget::cloneToolChain()
{
    ToolChainTreeItem *current = currentTreeItem();
    if (!current)
        return;

    ToolChain *tc = current->toolChain->clone();
    if (!tc)
        return;

    tc->setDetection(ToolChain::ManualDetection);
    tc->setDisplayName(QCoreApplication::translate("ProjectExplorer::ToolChain", "Clone of %1")
                        .arg(current->toolChain->displayName()));

    auto item = insertToolChain(tc, true);
    m_toAddList.append(item);

    m_toolChainView->setCurrentIndex(m_model.indexForItem(item));
}

void ToolChainOptionsWidget::updateState()
{
    bool canCopy = false;
    bool canDelete = false;
    if (ToolChainTreeItem *item = currentTreeItem()) {
        ToolChain *tc = item->toolChain;
        canCopy = tc->isValid();
        canDelete = tc->detection() != ToolChain::AutoDetectionFromSdk;
    }

    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
}

ToolChainTreeItem *ToolChainOptionsWidget::currentTreeItem()
{
    QModelIndex index = m_toolChainView->currentIndex();
    TreeItem *item = m_model.itemForIndex(index);
    return (item && item->level() == 3) ? static_cast<ToolChainTreeItem *>(item) : nullptr;
}

// --------------------------------------------------------------------------
// ToolChainOptionsPage
// --------------------------------------------------------------------------

ToolChainOptionsPage::ToolChainOptionsPage()
{
    setId(Constants::TOOLCHAIN_SETTINGS_PAGE_ID);
    setDisplayName(ToolChainOptionsPage::tr("Compilers"));
    setCategory(Constants::KITS_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new ToolChainOptionsWidget; });
}

} // namespace Internal
} // namespace ProjectExplorer
