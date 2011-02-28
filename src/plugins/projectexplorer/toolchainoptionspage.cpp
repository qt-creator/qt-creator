/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "toolchainoptionspage.h"

#include "projectexplorerconstants.h"
#include "toolchainconfigwidget.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QtCore/QSignalMapper>
#include <QtCore/QTextStream>
#include <QtGui/QAction>
#include <QtGui/QItemSelectionModel>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>

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
        if (widget) {
            widget->setEnabled(tc ? !tc->isAutoDetected() : false);
            widget->setVisible(false);
        }
    }

    ~ToolChainNode()
    {
        if (parent)
            parent->childNodes.removeOne(this);

        qDeleteAll(childNodes);
        // Do not delete toolchain, we do not own it.

        Q_ASSERT(childNodes.isEmpty());
    }

    ToolChainNode *parent;
    QString newName;
    QList<ToolChainNode *> childNodes;
    ToolChain *toolChain;
    ToolChainConfigWidget *widget;
    bool changed;
};

// --------------------------------------------------------------------------
// ToolChainModel
// --------------------------------------------------------------------------

ToolChainModel::ToolChainModel(QWidget *configWidgetParent, QObject *parent) :
    QAbstractItemModel(parent),
    m_configWidgetParent(configWidgetParent)
{
    Q_ASSERT(m_configWidgetParent);

    connect(ToolChainManager::instance(), SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
            this, SLOT(addToolChain(ProjectExplorer::ToolChain*)));
    connect(ToolChainManager::instance(), SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SLOT(removeToolChain(ProjectExplorer::ToolChain*)));

    m_autoRoot = new ToolChainNode(0);
    m_manualRoot = new ToolChainNode(0);

    foreach (ToolChain *tc, ToolChainManager::instance()->toolChains()) {
        addToolChain(tc);
    }
}

ToolChainModel::~ToolChainModel()
{
    delete m_autoRoot;
    delete m_manualRoot;
}

QModelIndex ToolChainModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row == 0)
            return createIndex(0, 0, static_cast<void *>(m_autoRoot));
        else
            return createIndex(1, 0, static_cast<void *>(m_manualRoot));
    }
    ToolChainNode *node = static_cast<ToolChainNode *>(parent.internalPointer());
    if (row < node->childNodes.count() && column < 2)
        return createIndex(row, column, static_cast<void *>(node->childNodes.at(row)));
    else
        return QModelIndex();
}

QModelIndex ToolChainModel::parent(const QModelIndex &idx) const
{
    ToolChainNode *node = static_cast<ToolChainNode *>(idx.internalPointer());
    if (node->parent == 0)
        return QModelIndex();
    return index(node->parent);
}

int ToolChainModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 2;
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
    Q_ASSERT(node);
    if (node == m_autoRoot && index.column() == 0 && role == Qt::DisplayRole)
        return tr("Auto-detected");
    if (node == m_manualRoot && index.column() == 0 && role == Qt::DisplayRole)
        return tr("Manual");
    if (node->toolChain) {
        if (role == Qt::FontRole) {
            QFont f = QApplication::font();
            if (node->changed) {
                f.setBold(true);
            }
            return f;
        }
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if (index.column() == 0) {
                return node->newName.isEmpty() ?
                    node->toolChain->displayName() : node->newName;
            }
            return node->toolChain->typeName();
        }
        if (role == Qt::ToolTipRole) {
            return tr("<nobr><b>ABI:</b> %1")
                    .arg(node->changed ? tr("not up-to-date") : node->toolChain->targetAbi().toString());
        }
    }
    return QVariant();
}

bool ToolChainModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    ToolChainNode *node = static_cast<ToolChainNode *>(index.internalPointer());
    Q_ASSERT(node);
    if (index.column() != 0 || !node->toolChain || role != Qt::EditRole)
        return false;
    node->newName = value.toString();
    if (!node->newName.isEmpty() && node->newName != node->toolChain->displayName())
        node->changed = true;
    return true;
}

Qt::ItemFlags ToolChainModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    ToolChainNode *node = static_cast<ToolChainNode *>(index.internalPointer());
    Q_ASSERT(node);
    if (!node->toolChain)
        return Qt::ItemIsEnabled;

    if (node->toolChain->isAutoDetected())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    else if (index.column() == 0)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    else
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

void ToolChainModel::setDirty(ToolChain *tc)
{
    foreach (ToolChainNode *n, m_manualRoot->childNodes) {
        if (n->toolChain == tc) {
            n->changed = true;
            emit dataChanged(index(n, 0), index(n, columnCount(QModelIndex())));
        }
    }
}

void ToolChainModel::apply()
{
    // Remove unused ToolChains:
    QList<ToolChainNode *> nodes = m_toRemoveList;
    foreach (ToolChainNode *n, nodes) {
        Q_ASSERT(!n->parent);
        ToolChainManager::instance()->deregisterToolChain(n->toolChain);
    }
    Q_ASSERT(m_toRemoveList.isEmpty());

    // Update toolchains:
    foreach (ToolChainNode *n, m_manualRoot->childNodes) {
        Q_ASSERT(n);
        if (n->changed) {
            Q_ASSERT(n->toolChain);
            if (!n->newName.isEmpty()) {
                n->toolChain->setDisplayName(n->newName);
                n->newName.clear();
            }
            if (n->widget)
                n->widget->apply();
            n->changed = false;

            emit dataChanged(index(n, 0), index(n, columnCount(QModelIndex())));
        }
    }

    // Add new (and already updated) toolchains
    nodes = m_toAddList;
    foreach (ToolChainNode *n, nodes) {
        ToolChainManager::instance()->registerToolChain(n->toolChain);
    }
    //
    foreach (ToolChainNode *n, m_toAddList) {
        markForRemoval(n->toolChain);
    }
    qDeleteAll(m_toAddList);
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
        } else {
            node->parent = 0;
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
    if (!node->parent)
        return index(node == m_autoRoot ? 0 : 1, column, QModelIndex());
    else
        return index(node->parent->childNodes.indexOf(node), column, index(node->parent));
}

ToolChainNode *ToolChainModel::createNode(ToolChainNode *parent, ToolChain *tc, bool changed)
{
    ToolChainNode *node = new ToolChainNode(parent, tc, changed);
    if (node->widget) {
        m_configWidgetParent->layout()->addWidget(node->widget);
        connect(node->widget, SIGNAL(dirty(ProjectExplorer::ToolChain*)),
                this, SLOT(setDirty(ProjectExplorer::ToolChain*)));
    }
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
    m_ui(0), m_cloneAction(0), m_model(0), m_selectionModel(0), m_currentTcWidget(0)
{ }

QString ToolChainOptionsPage::id() const
{
    return QLatin1String(Constants::TOOLCHAIN_SETTINGS_PAGE_ID);
}

QString ToolChainOptionsPage::displayName() const
{
    return tr("Toolchains");
}

QString ToolChainOptionsPage::category() const
{
    return QLatin1String(Constants::TOOLCHAIN_SETTINGS_CATEGORY);
}

QString ToolChainOptionsPage::displayCategory() const
{
    return tr("Toolchains");
}

QIcon ToolChainOptionsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Constants::ICON_TOOLCHAIN_SETTINGS_CATEGORY));
}

QWidget *ToolChainOptionsPage::createPage(QWidget *parent)
{
    // Actual page setup:
    m_configWidget = new QWidget(parent);

    m_currentTcWidget = 0;

    m_ui = new Ui::ToolChainOptionsPage;
    m_ui->setupUi(m_configWidget);

    Q_ASSERT(!m_model);
    m_model = new ToolChainModel(m_configWidget);
    connect(m_model, SIGNAL(toolChainStateChanged()), this, SLOT(updateState()));

    m_ui->toolChainView->setModel(m_model);
    m_ui->toolChainView->header()->setResizeMode(0, QHeaderView::ResizeToContents);
    m_ui->toolChainView->header()->setResizeMode(1, QHeaderView::ResizeToContents);
    m_ui->toolChainView->expandAll();

    m_selectionModel = m_ui->toolChainView->selectionModel();
    connect(m_selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(toolChainSelectionChanged(QModelIndex,QModelIndex)));

    // Get toolchainfactories:
    m_factories = ExtensionSystem::PluginManager::instance()->getObjects<ToolChainFactory>();

    // Set up add menu:
    QMenu *addMenu = new QMenu(m_ui->addButton);
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
    m_cloneAction = new QAction(addMenu);
    m_cloneAction->setText(tr("Clone ..."));
    connect(m_cloneAction, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(m_cloneAction, static_cast<QObject *>(0));

    if (!addMenu->isEmpty())
        addMenu->addSeparator();
    addMenu->addAction(m_cloneAction);
    m_ui->addButton->setMenu(addMenu);

    connect(m_ui->delButton, SIGNAL(clicked()), this, SLOT(removeToolChain()));

    // setup keywords:
    if (m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        QTextStream stream(&m_searchKeywords);
        stream << tr("Toolchains");
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
    if (m_model) {
        m_model->deleteLater();
        m_model = 0;
    }
}

bool ToolChainOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

void ToolChainOptionsPage::toolChainSelectionChanged(const QModelIndex &current,
                                                     const QModelIndex &previous)
{
    Q_UNUSED(previous);
    if (m_currentTcWidget)
        m_currentTcWidget->setVisible(false);

    m_currentTcWidget = m_model->widget(current);

    if (m_currentTcWidget)
        m_currentTcWidget->setVisible(true);

    updateState();
}

void ToolChainOptionsPage::createToolChain(QObject *factoryObject)
{
    ToolChain *tc = 0;

    ToolChainFactory *factory = static_cast<ToolChainFactory *>(factoryObject);
    if (!factory) {
        // Copy current item!
        ToolChain *oldTc = m_model->toolChain(m_selectionModel->currentIndex());
        Q_ASSERT(oldTc);
        tc = oldTc->clone();
    } else {
        Q_ASSERT(factory->canCreate());
        tc = factory->create();
    } if (!tc)
        return;
    m_model->markForAddition(tc);
}

void ToolChainOptionsPage::removeToolChain()
{
    ToolChain *tc = m_model->toolChain(m_selectionModel->currentIndex());
    Q_ASSERT(tc && !tc->isAutoDetected());
    m_model->markForRemoval(tc);
}

void ToolChainOptionsPage::updateState()
{

    bool canCopy = false;
    bool canDelete = false;
    ToolChain *tc = m_model->toolChain(m_selectionModel->currentIndex());
    if (tc) {
        canCopy = tc->isValid() && tc->canClone();
        canDelete = !tc->isAutoDetected();
    }

    m_cloneAction->setEnabled(canCopy);
    m_ui->delButton->setEnabled(canDelete);
}

} // namespace Internal
} // namespace ProjectExplorer
