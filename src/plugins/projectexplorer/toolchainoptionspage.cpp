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

#include "toolchainoptionspage.h"
#include "toolchain.h"
#include "abi.h"
#include "projectexplorerconstants.h"
#include "toolchainconfigwidget.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QTextStream>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class ToolChainTreeItem : public TreeItem
{
public:
    ToolChainTreeItem(ToolChain *tc, bool c) :
        toolChain(tc), changed(c)
    {
        widget = tc->configurationWidget();
        if (widget) {
            if (tc->isAutoDetected())
                widget->makeReadOnly();
            QObject::connect(widget, &ToolChainConfigWidget::dirty, [this] {
                changed = true;
                update();
            });
        }
    }

    QVariant data(int column, int role) const
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
                return ToolChainOptionsPage::tr("<nobr><b>ABI:</b> %1").arg(
                    changed ? ToolChainOptionsPage::tr("not up-to-date")
                            : toolChain->targetAbi().toString());
        }
        return QVariant();
    }

    ToolChain *toolChain;
    ToolChainConfigWidget *widget;
    bool changed;
};

// --------------------------------------------------------------------------
// ToolChainOptionsWidget
// --------------------------------------------------------------------------

class ToolChainOptionsWidget : public QWidget
{
public:
    ToolChainOptionsWidget()
    {
        m_factories = ExtensionSystem::PluginManager::getObjects<ToolChainFactory>(
                    [](ToolChainFactory *factory) { return factory->canCreate();});

        m_model.setHeader(QStringList() << ToolChainOptionsPage::tr("Name") << ToolChainOptionsPage::tr("Type"));
        m_autoRoot = new TreeItem(QStringList() << ToolChainOptionsPage::tr("Auto-detected") << QString());
        m_manualRoot = new TreeItem(QStringList() << ToolChainOptionsPage::tr("Manual") << QString());
        m_model.rootItem()->appendChild(m_autoRoot);
        m_model.rootItem()->appendChild(m_manualRoot);
        foreach (ToolChain *tc, ToolChainManager::toolChains()) {
            TreeItem *parent = tc->isAutoDetected() ? m_autoRoot : m_manualRoot;
            parent->appendChild(new ToolChainTreeItem(tc, false));
        }

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
            QAction *action = new QAction(addMenu);
            action->setText(factory->displayName());
            connect(action, &QAction::triggered, [this, factory] { createToolChain(factory); });
            addMenu->addAction(action);
        }
        m_addButton->setMenu(addMenu);

        m_cloneButton = new QPushButton(ToolChainOptionsPage::tr("Clone"), this);
        connect(m_cloneButton, &QAbstractButton::clicked, [this] { createToolChain(0); });

        m_delButton = new QPushButton(ToolChainOptionsPage::tr("Remove"), this);

        m_container = new DetailsWidget(this);
        m_container->setState(DetailsWidget::NoSummary);
        m_container->setVisible(false);

        auto buttonLayout = new QVBoxLayout;
        buttonLayout->setSpacing(6);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->addWidget(m_addButton);
        buttonLayout->addWidget(m_cloneButton);
        buttonLayout->addWidget(m_delButton);
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
    void createToolChain(ToolChainFactory *factory);
    ToolChainTreeItem *currentTreeItem();

    void markForRemoval(ToolChainTreeItem *item);
    void addToolChain(ProjectExplorer::ToolChain *);
    void removeToolChain(ProjectExplorer::ToolChain *);

    void apply();

 public:
    TreeModel m_model;
    QList<ToolChainFactory *> m_factories;
    QTreeView *m_toolChainView;
    DetailsWidget *m_container;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;

    TreeItem *m_autoRoot;
    TreeItem *m_manualRoot;

    QList<ToolChainTreeItem *> m_toAddList;
    QList<ToolChainTreeItem *> m_toRemoveList;
};

void ToolChainOptionsWidget::markForRemoval(ToolChainTreeItem *item)
{
    m_model.removeItem(item);
    if (m_toAddList.contains(item)) {
        delete item->toolChain;
        item->toolChain = 0;
        m_toAddList.removeOne(item);
        delete item;
    } else {
        m_toRemoveList.append(item);
    }
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

    TreeItem *parent = m_model.rootItem()->child(tc->isAutoDetected() ? 0 : 1);
    parent->appendChild(new ToolChainTreeItem(tc, false));

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

    TreeItem *parent = m_model.rootItem()->child(tc->isAutoDetected() ? 0 : 1);
    foreach (ToolChainTreeItem *item, m_model.treeLevelItems<ToolChainTreeItem *>(1, parent)) {
        if (item->toolChain == tc) {
            m_model.removeItem(item);
            delete item;
            break;
        }
    }

    updateState();
}

void ToolChainOptionsWidget::toolChainSelectionChanged()
{
    ToolChainTreeItem *item = currentTreeItem();
    QWidget *oldWidget = m_container->takeWidget(); // Prevent deletion.
    if (oldWidget)
        oldWidget->setVisible(false);

    QWidget *currentTcWidget = item ? item->widget : 0;

    m_container->setWidget(currentTcWidget);
    m_container->setVisible(currentTcWidget != 0);
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
    foreach (ToolChainTreeItem *item, m_model.treeLevelItems<ToolChainTreeItem *>(1, m_manualRoot)) {
        if (item->changed) {
            Q_ASSERT(item->toolChain);
            if (item->widget)
                item->widget->apply();
            item->changed = false;
            item->update();
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
}

void ToolChainOptionsWidget::createToolChain(ToolChainFactory *factory)
{
    ToolChain *tc = 0;

    if (factory) {
        // Clone.
        QTC_CHECK(factory->canCreate());
        tc = factory->create();
    } else {
        // Copy.
        ToolChainTreeItem *current = currentTreeItem();
        if (!current)
            return;
        tc = current->toolChain->clone();
    }

    if (!tc)
        return;

    ToolChainTreeItem *item = new ToolChainTreeItem(tc, true);
    m_toAddList.append(item);

    m_manualRoot->appendChild(item);

    m_toolChainView->setCurrentIndex(m_model.indexFromItem(item));
}

void ToolChainOptionsWidget::updateState()
{
    bool canCopy = false;
    bool canDelete = false;
    if (ToolChainTreeItem *item = currentTreeItem()) {
        ToolChain *tc = item->toolChain;
        canCopy = tc->isValid() && tc->canClone();
        canDelete = tc->detection() != ToolChain::AutoDetection;
    }

    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
}

ToolChainTreeItem *ToolChainOptionsWidget::currentTreeItem()
{
    QModelIndex index = m_toolChainView->currentIndex();
    TreeItem *item = m_model.itemFromIndex(index);
    return item && item->level() == 2 ? static_cast<ToolChainTreeItem *>(item) : 0;
}

// --------------------------------------------------------------------------
// ToolChainOptionsPage
// --------------------------------------------------------------------------

ToolChainOptionsPage::ToolChainOptionsPage()
{
    setId(Constants::TOOLCHAIN_SETTINGS_PAGE_ID);
    setDisplayName(tr("Compilers"));
    setCategory(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
        Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

QWidget *ToolChainOptionsPage::widget()
{
    if (!m_widget)
        m_widget = new ToolChainOptionsWidget;
    return m_widget;
}

void ToolChainOptionsPage::apply()
{
    if (m_widget)
        m_widget->apply();
}

void ToolChainOptionsPage::finish()
{
    delete m_widget;
    m_widget = 0;
}

} // namespace Internal
} // namespace ProjectExplorer
