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

#include "toolchainoptionspage.h"
#include "toolchain.h"
#include "abi.h"
#include "projectexplorerconstants.h"
#include "toolchainconfigwidget.h"
#include "toolchainmanager.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/detailswidget.h>
#include <utils/qtcassert.h>

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

namespace ProjectExplorer {
namespace Internal {

class ToolChainNode
{
public:
    explicit ToolChainNode(ToolChainNode *p, ToolChain *tc = 0, bool c = false) :
        parent(p), toolChain(tc), changed(c)
    {
        if (p)
            p->childNodes.append(this);
        widget = tc ? tc->configurationWidget() : 0;
        if (widget && tc->isAutoDetected())
            widget->makeReadOnly();
    }

    ~ToolChainNode()
    {
        // Do not delete tool chain, we do not own it.

        for (int i = childNodes.size(); --i >= 0; ) {
            ToolChainNode *child = childNodes.at(i);
            child->parent = 0;
            delete child;
        }

        if (parent)
            parent->childNodes.removeOne(this);
    }

    ToolChainNode *parent;
    QList<ToolChainNode *> childNodes;
    ToolChain *toolChain;
    ToolChainConfigWidget *widget;
    bool changed;
};

// --------------------------------------------------------------------------
// ToolChainModel
// --------------------------------------------------------------------------

ToolChainModel::ToolChainModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    connect(ToolChainManager::instance(), SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
            this, SLOT(addToolChain(ProjectExplorer::ToolChain*)));
    connect(ToolChainManager::instance(), SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SLOT(removeToolChain(ProjectExplorer::ToolChain*)));

    m_root = new ToolChainNode(0);
    m_autoRoot = new ToolChainNode(m_root);
    m_manualRoot = new ToolChainNode(m_root);

    foreach (ToolChain *tc, ToolChainManager::instance()->toolChains()) {
        addToolChain(tc);
    }
}

ToolChainModel::~ToolChainModel()
{
    delete m_root;
}

QModelIndex ToolChainModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row >= 0 && row < m_root->childNodes.count())
            return createIndex(row, column, m_root->childNodes.at(row));
    }
    ToolChainNode *node = static_cast<ToolChainNode *>(parent.internalPointer());
    if (row < node->childNodes.count() && column < 2)
        return createIndex(row, column, static_cast<void *>(node->childNodes.at(row)));
    else
        return QModelIndex();
}

QModelIndex ToolChainModel::index(const QModelIndex &topIdx, ToolChain *tc) const
{
    ToolChainNode *current = m_root;
    if (topIdx.isValid())
        current = static_cast<ToolChainNode *>(topIdx.internalPointer());
    QTC_ASSERT(current, return QModelIndex());

    if (current->toolChain == tc)
        return topIdx;

    for (int i = 0; i < current->childNodes.count(); ++i) {
        QModelIndex result = index(index(current->childNodes.at(i)), tc);
        if (result.isValid())
            return result;
    }
    return QModelIndex();
}

QModelIndex ToolChainModel::parent(const QModelIndex &idx) const
{
    ToolChainNode *node = static_cast<ToolChainNode *>(idx.internalPointer());
    if (node->parent == m_root)
        return QModelIndex();
    return index(node->parent);
}

int ToolChainModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_root->childNodes.count();
    ToolChainNode *node = static_cast<ToolChainNode *>(parent.internalPointer());
    return node->childNodes.count();
}

int ToolChainModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant ToolChainModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    ToolChainNode *node = static_cast<ToolChainNode *>(index.internalPointer());
    QTC_ASSERT(node, return QVariant());
    if (node == m_autoRoot && index.column() == 0 && role == Qt::DisplayRole)
        return tr("Auto-detected");
    if (node == m_manualRoot && index.column() == 0 && role == Qt::DisplayRole)
        return tr("Manual");
    if (node->toolChain) {
        if (role == Qt::FontRole) {
            QFont f = QApplication::font();
            if (node->changed)
                f.setBold(true);
            return f;
        }
        if (role == Qt::DisplayRole) {
            if (index.column() == 0)
                return node->toolChain->displayName();
            return node->toolChain->typeDisplayName();
        }
        if (role == Qt::ToolTipRole) {
            return tr("<nobr><b>ABI:</b> %1")
                    .arg(node->changed ? tr("not up-to-date") : node->toolChain->targetAbi().toString());
        }
    }
    return QVariant();
}

Qt::ItemFlags ToolChainModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    ToolChainNode *node = static_cast<ToolChainNode *>(index.internalPointer());
    Q_ASSERT(node);
    if (!node->toolChain)
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ToolChainModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return section == 0 ? tr("Name") : tr("Type");
    return QVariant();
}

ToolChain *ToolChainModel::toolChain(const QModelIndex &index)
{
    if (!index.isValid())
        return 0;
    ToolChainNode *node = static_cast<ToolChainNode *>(index.internalPointer());
    Q_ASSERT(node);
    return node->toolChain;
}

int ToolChainModel::manualToolChains() const
{
    return m_manualRoot->childNodes.count();
}

ToolChainConfigWidget *ToolChainModel::widget(const QModelIndex &index)
{
    if (!index.isValid())
        return 0;
    ToolChainNode *node = static_cast<ToolChainNode *>(index.internalPointer());
    Q_ASSERT(node);
    return node->widget;
}

bool ToolChainModel::isDirty() const
{
    foreach (ToolChainNode *n, m_manualRoot->childNodes) {
        if (n->changed)
            return true;
    }
    return false;
}

bool ToolChainModel::isDirty(ToolChain *tc) const
{
    foreach (ToolChainNode *n, m_manualRoot->childNodes) {
        if (n->toolChain == tc && n->changed)
            return true;
    }
    return false;
}

void ToolChainModel::setDirty()
{
    ToolChainConfigWidget *w = qobject_cast<ToolChainConfigWidget *>(sender());
    foreach (ToolChainNode *n, m_manualRoot->childNodes) {
        if (n->widget == w) {
            n->changed = true;
            emit dataChanged(index(n, 0), index(n, columnCount(QModelIndex())));
        }
    }
}

void ToolChainModel::apply()
{
    // Remove unused tool chains:
    QList<ToolChainNode *> nodes = m_toRemoveList;
    foreach (ToolChainNode *n, nodes) {
        Q_ASSERT(!n->parent);
        ToolChainManager::instance()->deregisterToolChain(n->toolChain);
    }
    Q_ASSERT(m_toRemoveList.isEmpty());

    // Update tool chains:
    foreach (ToolChainNode *n, m_manualRoot->childNodes) {
        Q_ASSERT(n);
        if (n->changed) {
            Q_ASSERT(n->toolChain);
            if (n->widget)
                n->widget->apply();
            n->changed = false;

            emit dataChanged(index(n, 0), index(n, columnCount(QModelIndex())));
        }
    }

    // Add new (and already updated) tool chains
    QStringList removedTcs;
    nodes = m_toAddList;
    foreach (ToolChainNode *n, nodes) {
        if (!ToolChainManager::instance()->registerToolChain(n->toolChain))
            removedTcs << n->toolChain->displayName();
    }
    //
    foreach (ToolChainNode *n, m_toAddList) {
        markForRemoval(n->toolChain);
    }
    qDeleteAll(m_toAddList);

    if (removedTcs.count() == 1) {
        QMessageBox::warning(0,
                             tr("Duplicate Compilers Detected"),
                             tr("The following compiler was already configured:<br>"
                                "&nbsp;%1<br>"
                                "It was not configured again.")
                             .arg(removedTcs.at(0)));

    } else if (!removedTcs.isEmpty()) {
        QMessageBox::warning(0,
                             tr("Duplicate Compilers Detected"),
                             tr("The following compilers were already configured:<br>"
                                "&nbsp;%1<br>"
                                "They were not configured again.")
                             .arg(removedTcs.join(QLatin1String(",<br>&nbsp;"))));
    }
}

void ToolChainModel::markForRemoval(ToolChain *tc)
{
    ToolChainNode *node = 0;
    foreach (ToolChainNode *n, m_manualRoot->childNodes) {
        if (n->toolChain == tc) {
            node = n;
            break;
        }
    }
    if (node) {
        emit beginRemoveRows(index(m_manualRoot), m_manualRoot->childNodes.indexOf(node), m_manualRoot->childNodes.indexOf(node));
        m_manualRoot->childNodes.removeOne(node);
        node->parent = 0;
        if (m_toAddList.contains(node)) {
            delete node->toolChain;
            node->toolChain = 0;
            m_toAddList.removeOne(node);
            delete node;
        } else {
            m_toRemoveList.append(node);
        }
        emit endRemoveRows();
    }
}

void ToolChainModel::markForAddition(ToolChain *tc)
{
    int pos = m_manualRoot->childNodes.size();
    emit beginInsertRows(index(m_manualRoot), pos, pos);

    ToolChainNode *node = createNode(m_manualRoot, tc, true);
    m_toAddList.append(node);

    emit endInsertRows();
}

QModelIndex ToolChainModel::index(ToolChainNode *node, int column) const
{
    if (node == m_root)
        return QModelIndex();
    if (node->parent == m_root)
        return index(m_root->childNodes.indexOf(node), column, QModelIndex());
    else
        return index(node->parent->childNodes.indexOf(node), column, index(node->parent));
}

ToolChainNode *ToolChainModel::createNode(ToolChainNode *parent, ToolChain *tc, bool changed)
{
    ToolChainNode *node = new ToolChainNode(parent, tc, changed);
    if (node->widget)
        connect(node->widget, SIGNAL(dirty()), this, SLOT(setDirty()));
    return node;
}

void ToolChainModel::addToolChain(ToolChain *tc)
{
    QList<ToolChainNode *> nodes = m_toAddList;
    foreach (ToolChainNode *n, nodes) {
        if (n->toolChain == tc) {
            m_toAddList.removeOne(n);
            // do not delete n: Still used elsewhere!
            return;
        }
    }

    ToolChainNode *parent = m_manualRoot;
    if (tc->isAutoDetected())
        parent = m_autoRoot;
    int row = parent->childNodes.count();

    beginInsertRows(index(parent), row, row);
    createNode(parent, tc, false);
    endInsertRows();

    emit toolChainStateChanged();
}

void ToolChainModel::removeToolChain(ToolChain *tc)
{
    QList<ToolChainNode *> nodes = m_toRemoveList;
    foreach (ToolChainNode *n, nodes) {
        if (n->toolChain == tc) {
            m_toRemoveList.removeOne(n);
            delete n;
            return;
        }
    }

    ToolChainNode *parent = m_manualRoot;
    if (tc->isAutoDetected())
        parent = m_autoRoot;
    int row = 0;
    ToolChainNode *node = 0;
    foreach (ToolChainNode *current, parent->childNodes) {
        if (current->toolChain == tc) {
            node = current;
            break;
        }
        ++row;
    }

    beginRemoveRows(index(parent), row, row);
    parent->childNodes.removeAt(row);
    delete node;
    endRemoveRows();

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

QWidget *ToolChainOptionsPage::createPage(QWidget *parent)
{
    // Actual page setup:
    m_configWidget = new QWidget(parent);

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
    m_toolChainView->header()->setResizeMode(0, QHeaderView::ResizeToContents);
    m_toolChainView->header()->setResizeMode(1, QHeaderView::Stretch);
    m_toolChainView->expandAll();

    m_selectionModel = m_toolChainView->selectionModel();
    connect(m_selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(toolChainSelectionChanged()));
    connect(ToolChainManager::instance(), SIGNAL(toolChainsChanged()),
            this, SLOT(toolChainSelectionChanged()));

    // Get toolchainfactories:
    m_factories = ExtensionSystem::PluginManager::getObjects<ToolChainFactory>();

    // Set up add menu:
    QMenu *addMenu = new QMenu(m_addButton);
    QSignalMapper *mapper = new QSignalMapper(addMenu);
    connect(mapper, SIGNAL(mapped(QObject*)), this, SLOT(createToolChain(QObject*)));

    foreach (ToolChainFactory *factory, m_factories) {
        if (factory->canCreate()) {
            QAction *action = new QAction(addMenu);
            action->setText(factory->displayName());
            connect(action, SIGNAL(triggered()), mapper, SLOT(map()));
            mapper->setMapping(action, static_cast<QObject *>(factory));

            addMenu->addAction(action);
        }
    }
    connect(m_cloneButton, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(m_cloneButton, static_cast<QObject *>(0));

    m_addButton->setMenu(addMenu);

    connect(m_delButton, SIGNAL(clicked()), this, SLOT(removeToolChain()));

    // setup keywords:
    if (m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        QTextStream stream(&m_searchKeywords);
        stream << tr("Compilers");
        foreach (ToolChainFactory *f, m_factories)
            stream << sep << f->displayName();

        m_searchKeywords.remove(QLatin1Char('&'));
    }

    updateState();

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

    // delete by settingsdialog;
    m_configWidget = 0;

    // children of m_configWidget
    m_model = 0;
    m_container = 0;
    m_selectionModel = 0;
    m_toolChainView = 0;
    m_addButton = 0;
    m_cloneButton = 0;
    m_delButton = 0;
}

bool ToolChainOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
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
    } if (!tc)
        return;
    m_model->markForAddition(tc);

    QModelIndex newIdx = m_model->index(QModelIndex(), tc);
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
        canDelete = !tc->isAutoDetected();
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
