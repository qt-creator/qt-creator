/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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

#include <utils/detailswidget.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalMapper>
#include <QSpacerItem>
#include <QTextStream>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class ToolChainNode : public Utils::TreeItem
{
public:
    ToolChainNode(ToolChain *tc, bool c) :
        toolChain(tc), changed(c)
    {
        widget = tc->configurationWidget();
        if (widget && tc->isAutoDetected())
            widget->makeReadOnly();
    }

    int columnCount() const { return 2; }

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
                return ToolChainModel::tr("<nobr><b>ABI:</b> %1").arg(
                    changed ? ToolChainModel::tr("not up-to-date")
                            : toolChain->targetAbi().toString());
        }
        return QVariant();
    }

    ToolChain *toolChain;
    ToolChainConfigWidget *widget;
    bool changed;
};

// --------------------------------------------------------------------------
// ToolChainModel
// --------------------------------------------------------------------------

ToolChainModel::ToolChainModel(QObject *parent) :
    TreeModel(parent)
{
    connect(ToolChainManager::instance(), SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
            this, SLOT(addToolChain(ProjectExplorer::ToolChain*)));
    connect(ToolChainManager::instance(), SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SLOT(removeToolChain(ProjectExplorer::ToolChain*)));

    auto root = new TreeItem(QStringList() << tr("Name") << tr("Type"));
    m_autoRoot = new TreeItem(QStringList() << tr("Auto-detected") << QString());
    m_manualRoot = new TreeItem(QStringList() << tr("Manual") << QString());
    root->appendChild(m_autoRoot);
    root->appendChild(m_manualRoot);
    setRootItem(root);

    foreach (ToolChain *tc, ToolChainManager::toolChains())
        addToolChain(tc);
}

QModelIndex ToolChainModel::index(ToolChain *tc) const
{
    foreach (TreeItem *group, rootItem()->children()) {
        foreach (TreeItem *item, group->children())
            if (static_cast<ToolChainNode *>(item)->toolChain == tc)
                return indexFromItem(item);
    }
    return QModelIndex();
}

ToolChain *ToolChainModel::toolChain(const QModelIndex &index)
{
    TreeItem *item = itemFromIndex(index);
    return item && item->level() == 2 ? static_cast<ToolChainNode *>(item)->toolChain : 0;
}

ToolChainConfigWidget *ToolChainModel::widget(const QModelIndex &index)
{
    TreeItem *item = itemFromIndex(index);
    return item && item->level() == 2 ? static_cast<ToolChainNode *>(item)->widget : 0;
}

bool ToolChainModel::isDirty() const
{
    return Utils::anyOf(m_manualRoot->children(), [](TreeItem *n) {
        return static_cast<ToolChainNode *>(n)->changed;
    });
}

bool ToolChainModel::isDirty(ToolChain *tc) const
{
    return Utils::anyOf(m_manualRoot->children(), [tc](TreeItem *item) -> bool {
        ToolChainNode *n = static_cast<ToolChainNode *>(item);
        return n->toolChain == tc && n->changed;
    });
}

void ToolChainModel::setDirty()
{
    ToolChainConfigWidget *w = qobject_cast<ToolChainConfigWidget *>(sender());
    foreach (TreeItem *item, m_manualRoot->children()) {
        ToolChainNode *n = static_cast<ToolChainNode *>(item);
        if (n->widget == w) {
            n->changed = true;
            updateItem(item);
        }
    }
}

void ToolChainModel::apply()
{
    // Remove unused tool chains:
    QList<ToolChainNode *> nodes = m_toRemoveList;
    foreach (ToolChainNode *n, nodes)
        ToolChainManager::deregisterToolChain(n->toolChain);

    Q_ASSERT(m_toRemoveList.isEmpty());

    // Update tool chains:
    foreach (TreeItem *item, m_manualRoot->children()) {
        ToolChainNode *n = static_cast<ToolChainNode *>(item);
        if (n->changed) {
            Q_ASSERT(n->toolChain);
            if (n->widget)
                n->widget->apply();
            n->changed = false;
            updateItem(item);
        }
    }

    // Add new (and already updated) tool chains
    QStringList removedTcs;
    nodes = m_toAddList;
    foreach (ToolChainNode *n, nodes) {
        if (!ToolChainManager::registerToolChain(n->toolChain))
            removedTcs << n->toolChain->displayName();
    }
    //
    foreach (ToolChainNode *n, m_toAddList) {
        markForRemoval(n->toolChain);
    }
    qDeleteAll(m_toAddList);

    if (removedTcs.count() == 1) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             tr("Duplicate Compilers Detected"),
                             tr("The following compiler was already configured:<br>"
                                "&nbsp;%1<br>"
                                "It was not configured again.")
                             .arg(removedTcs.at(0)));

    } else if (!removedTcs.isEmpty()) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             tr("Duplicate Compilers Detected"),
                             tr("The following compilers were already configured:<br>"
                                "&nbsp;%1<br>"
                                "They were not configured again.")
                             .arg(removedTcs.join(QLatin1String(",<br>&nbsp;"))));
    }
}

void ToolChainModel::markForRemoval(ToolChain *tc)
{
    foreach (TreeItem *item, m_manualRoot->children()) {
        ToolChainNode *node = static_cast<ToolChainNode *>(item);
        if (node->toolChain == tc) {
            removeItem(node);
            if (m_toAddList.contains(node)) {
                delete node->toolChain;
                node->toolChain = 0;
                m_toAddList.removeOne(node);
                delete node;
            } else {
                m_toRemoveList.append(node);
            }
        }
    }
}

void ToolChainModel::markForAddition(ToolChain *tc)
{
    ToolChainNode *node = createNode(tc, true);
    appendItem(m_manualRoot, node);
    m_toAddList.append(node);
}

ToolChainNode *ToolChainModel::createNode(ToolChain *tc, bool changed)
{
    ToolChainNode *node = new ToolChainNode(tc, changed);
    if (node->widget)
        connect(node->widget, &ToolChainConfigWidget::dirty, this, &ToolChainModel::setDirty);
    return node;
}

void ToolChainModel::addToolChain(ToolChain *tc)
{
    foreach (ToolChainNode *n, m_toAddList) {
        if (n->toolChain == tc) {
            // do not delete n: Still used elsewhere!
            m_toAddList.removeOne(n);
            return;
        }
    }

    TreeItem *parent = rootItem()->child(tc->isAutoDetected() ? 0 : 1);
    appendItem(parent, createNode(tc, false));

    emit toolChainStateChanged();
}

void ToolChainModel::removeToolChain(ToolChain *tc)
{
    foreach (ToolChainNode *n, m_toRemoveList) {
        if (n->toolChain == tc) {
            m_toRemoveList.removeOne(n);
            delete n;
            return;
        }
    }

    TreeItem *parent = rootItem()->child(tc->isAutoDetected() ? 0 : 1);
    foreach (TreeItem *item, parent->children()) {
        ToolChainNode *node = static_cast<ToolChainNode *>(item);
        if (node->toolChain == tc) {
            removeItem(node);
            delete node;
            break;
        }
    }

    emit toolChainStateChanged();
}

// --------------------------------------------------------------------------
// ToolChainOptionsPage
// --------------------------------------------------------------------------

ToolChainOptionsPage::ToolChainOptionsPage() :
    m_model(0), m_selectionModel(0), m_toolChainView(0), m_container(0),
    m_addButton(0), m_cloneButton(0), m_delButton(0)
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
    if (!m_configWidget) {
        // Actual page setup:
        m_configWidget = new QWidget;

        m_toolChainView = new QTreeView(m_configWidget);
        m_toolChainView->setUniformRowHeights(true);
        m_toolChainView->header()->setStretchLastSection(false);

        m_addButton = new QPushButton(tr("Add"), m_configWidget);
        m_cloneButton = new QPushButton(tr("Clone"), m_configWidget);
        m_delButton = new QPushButton(tr("Remove"), m_configWidget);

        m_container = new Utils::DetailsWidget(m_configWidget);
        m_container->setState(Utils::DetailsWidget::NoSummary);
        m_container->setVisible(false);

        QVBoxLayout *buttonLayout = new QVBoxLayout();
        buttonLayout->setSpacing(6);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->addWidget(m_addButton);
        buttonLayout->addWidget(m_cloneButton);
        buttonLayout->addWidget(m_delButton);
        buttonLayout->addItem(new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        QVBoxLayout *verticalLayout = new QVBoxLayout();
        verticalLayout->addWidget(m_toolChainView);
        verticalLayout->addWidget(m_container);

        QHBoxLayout *horizontalLayout = new QHBoxLayout(m_configWidget);
        horizontalLayout->addLayout(verticalLayout);
        horizontalLayout->addLayout(buttonLayout);
        Q_ASSERT(!m_model);
        m_model = new ToolChainModel(m_configWidget);

        connect(m_model, SIGNAL(toolChainStateChanged()), this, SLOT(updateState()));

        m_toolChainView->setModel(m_model);
        m_toolChainView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_toolChainView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_toolChainView->expandAll();

        m_selectionModel = m_toolChainView->selectionModel();
        connect(m_selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                this, SLOT(toolChainSelectionChanged()));
        connect(ToolChainManager::instance(), SIGNAL(toolChainsChanged()),
                this, SLOT(toolChainSelectionChanged()));

        // Get toolchainfactories:
        m_factories = ExtensionSystem::PluginManager::getObjects<ToolChainFactory>(
                    [](ToolChainFactory *factory) { return factory->canCreate();});

        // Set up add menu:
        QMenu *addMenu = new QMenu(m_addButton);
        QSignalMapper *mapper = new QSignalMapper(addMenu);
        connect(mapper, SIGNAL(mapped(QObject*)), this, SLOT(createToolChain(QObject*)));

        foreach (ToolChainFactory *factory, m_factories) {
            QAction *action = new QAction(addMenu);
            action->setText(factory->displayName());
            connect(action, SIGNAL(triggered()), mapper, SLOT(map()));
            mapper->setMapping(action, static_cast<QObject *>(factory));

            addMenu->addAction(action);
        }
        connect(m_cloneButton, SIGNAL(clicked()), mapper, SLOT(map()));
        mapper->setMapping(m_cloneButton, static_cast<QObject *>(0));

        m_addButton->setMenu(addMenu);

        connect(m_delButton, SIGNAL(clicked()), this, SLOT(removeToolChain()));
        updateState();
    }
    return m_configWidget;
}

void ToolChainOptionsPage::apply()
{
    if (m_model)
        m_model->apply();
}

void ToolChainOptionsPage::finish()
{
    disconnect(ToolChainManager::instance(), SIGNAL(toolChainsChanged()),
               this, SLOT(toolChainSelectionChanged()));

    delete m_configWidget;

    // children of m_configWidget
    m_model = 0;
    m_container = 0;
    m_selectionModel = 0;
    m_toolChainView = 0;
    m_addButton = 0;
    m_cloneButton = 0;
    m_delButton = 0;
}

void ToolChainOptionsPage::toolChainSelectionChanged()
{
    if (!m_container)
        return;
    QModelIndex current = currentIndex();
    QWidget *oldWidget = m_container->takeWidget(); // Prevent deletion.
    if (oldWidget)
        oldWidget->setVisible(false);
    QWidget *currentTcWidget = current.isValid() ? m_model->widget(current) : 0;
    m_container->setWidget(currentTcWidget);
    m_container->setVisible(currentTcWidget != 0);
    updateState();
}

void ToolChainOptionsPage::createToolChain(QObject *factoryObject)
{
    ToolChain *tc = 0;

    ToolChainFactory *factory = static_cast<ToolChainFactory *>(factoryObject);
    if (!factory) {
        // Copy current item!
        ToolChain *oldTc = m_model->toolChain(currentIndex());
        if (!oldTc)
            return;
        tc = oldTc->clone();
    } else {
        QTC_CHECK(factory->canCreate());
        tc = factory->create();
    }

    if (!tc)
        return;

    m_model->markForAddition(tc);

    QModelIndex newIdx = m_model->index(tc);
    m_selectionModel->select(newIdx,
                             QItemSelectionModel::Clear
                             | QItemSelectionModel::SelectCurrent
                             | QItemSelectionModel::Rows);
}

void ToolChainOptionsPage::removeToolChain()
{
    ToolChain *tc = m_model->toolChain(currentIndex());
    if (!tc)
        return;
    m_model->markForRemoval(tc);
}

void ToolChainOptionsPage::updateState()
{
    if (!m_cloneButton)
        return;

    bool canCopy = false;
    bool canDelete = false;
    ToolChain *tc = m_model->toolChain(currentIndex());
    if (tc) {
        canCopy = tc->isValid() && tc->canClone();
        canDelete = tc->detection() != ToolChain::AutoDetection;
    }

    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
}

QModelIndex ToolChainOptionsPage::currentIndex() const
{
    if (!m_selectionModel)
        return QModelIndex();

    QModelIndexList idxs = m_selectionModel->selectedRows();
    if (idxs.count() != 1)
        return QModelIndex();
    return idxs.at(0);
}

} // namespace Internal
} // namespace ProjectExplorer
