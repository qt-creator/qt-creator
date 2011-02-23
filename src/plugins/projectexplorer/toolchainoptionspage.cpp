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

typedef QSharedPointer<ToolChainNode> ToolChainNodePtr;

class ToolChainNode
{
public:
    explicit ToolChainNode(ToolChain *tc = 0, bool c = false) :
         parent(0), toolChain(tc), changed(c)
    {
        widget = tc ? tc->configurationWidget() : 0;
        if (widget) {
            widget->setEnabled(tc ? !tc->isAutoDetected() : false);
            widget->setVisible(false);
        }
    }

    ~ToolChainNode()
    {
        // Do not delete toolchain, we do not own it.
        delete widget;
    }

    void addChild(const ToolChainNodePtr &c)
    {
        c->parent = this;
        childNodes.push_back(c);
    }

    ToolChainNode *parent;
    QString newName;
    QList<ToolChainNodePtr> childNodes;
    ToolChain *toolChain;
    ToolChainConfigWidget *widget;
    bool changed;
};

static int indexOfToolChain(const QList<ToolChainNodePtr> &l, const ToolChain *tc)
{
    const int count = l.size();
    for (int i = 0; i < count ; i++)
    if (l.at(i)->toolChain == tc)
        return i;
    return -1;
}

static int indexOfToolChainNode(const QList<ToolChainNodePtr> &l, const ToolChainNode *node)
{
    const int count = l.size();
    for (int i = 0; i < count ; i++)
    if (l.at(i).data() == node)
        return i;
    return -1;
}

// --------------------------------------------------------------------------
// ToolChainModel
// --------------------------------------------------------------------------

ToolChainModel::ToolChainModel(QObject *parent) :
    QAbstractItemModel(parent), m_autoRoot(new ToolChainNode), m_manualRoot(new ToolChainNode)
{
    connect(ToolChainManager::instance(), SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
            this, SLOT(addToolChain(ProjectExplorer::ToolChain*)));
    connect(ToolChainManager::instance(), SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SLOT(removeToolChain(ProjectExplorer::ToolChain*)));

    foreach (ToolChain *tc, ToolChainManager::instance()->toolChains()) {
        if (tc->isAutoDetected()) {
            m_autoRoot->addChild(ToolChainNodePtr(new ToolChainNode(tc)));
       } else {
            ToolChainNodePtr node(new ToolChainNode(tc));
            m_manualRoot->addChild(node);
            if (node->widget)
                connect(node->widget, SIGNAL(dirty(ProjectExplorer::ToolChain*)),
                        this, SLOT(setDirty(ProjectExplorer::ToolChain*)));
        }
    }
}

ToolChainModel::~ToolChainModel()
{
}

QModelIndex ToolChainModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row == 0)
            return createIndex(0, 0, static_cast<void *>(m_autoRoot.data()));
        else
            return createIndex(1, 0, static_cast<void *>(m_manualRoot.data()));
    }
    ToolChainNode *node = static_cast<ToolChainNode *>(parent.internalPointer());
    if (row < node->childNodes.count() && column < 2)
        return createIndex(row, column, static_cast<void *>(node->childNodes.at(row).data()));
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
    foreach (const ToolChainNodePtr &n, m_manualRoot->childNodes) {
        if (n->changed)
            return true;
    }
    return false;
}

bool ToolChainModel::isDirty(ToolChain *tc) const
{
    foreach (const ToolChainNodePtr &n, m_manualRoot->childNodes) {
        if (n->toolChain == tc && n->changed)
            return true;
    }
    return false;
}

void ToolChainModel::setDirty(ToolChain *tc)
{
    foreach (const ToolChainNodePtr &n, m_manualRoot->childNodes) {
        if (n->toolChain == tc) {
            n->changed = true;
            emit dataChanged(index(n.data(), 0), index(n.data(), columnCount(QModelIndex())));
        }
    }
}

void ToolChainModel::apply()
{
    // Remove unused ToolChains:
    QList<ToolChainNodePtr> nodes = m_toRemoveList;
    foreach (const ToolChainNodePtr &n, nodes) {
        ToolChainManager::instance()->deregisterToolChain(n->toolChain);
    }
    QTC_ASSERT(m_toRemoveList.isEmpty(), /*  */ );

    // Update toolchains:
    foreach (const ToolChainNodePtr &n, m_manualRoot->childNodes) {
        Q_ASSERT(n.data());
        if (n->changed) {
            Q_ASSERT(n->toolChain);
            if (!n->newName.isEmpty())
                n->toolChain->setDisplayName(n->newName);
            if (n->widget)
                n->widget->apply();
            n->changed = false;

            emit dataChanged(index(n.data(), 0), index(n.data(), columnCount(QModelIndex())));
        }
    }

    // Add new (and already updated) toolchains
    nodes = m_toAddList;
    foreach (const ToolChainNodePtr &n, nodes) {
        ToolChainManager::instance()->registerToolChain(n->toolChain);
    }
    QTC_ASSERT(m_toAddList.isEmpty(), qDebug() <<  m_toAddList.front()->toolChain; );
}

void ToolChainModel::discard()
{
    // Remove newly "added" toolchains:
    foreach (const ToolChainNodePtr &n, m_toAddList) {
        const int pos = indexOfToolChainNode(m_manualRoot->childNodes, n.data());
        QTC_ASSERT(pos >= 0, continue ; );
        m_manualRoot->childNodes.removeAt(pos);
        // Clean up Node: We still own the toolchain!
        delete n->toolChain;
        n->toolChain = 0;
    }
    m_toAddList.clear();

    // Add "removed" toolchains again:
    foreach (const ToolChainNodePtr &n, m_toRemoveList) {
        m_manualRoot->addChild(n);
    }
    m_toRemoveList.clear();

    // Reset toolchains:
    foreach (const ToolChainNodePtr &n, m_manualRoot->childNodes) {
        n->newName.clear();
        if (n->widget)
            n->widget->discard();
        n->changed = false;
    }
    reset();
}

void ToolChainModel::markForRemoval(ToolChain *tc)
{
    // Delete newly added item?
    const int tcIndex = indexOfToolChain(m_manualRoot->childNodes, tc);
    if (tcIndex != -1) {
        m_toRemoveList.append(m_manualRoot->childNodes.at(tcIndex));
        emit beginRemoveRows(index(m_manualRoot.data()), tcIndex, tcIndex);
        m_manualRoot->childNodes.removeAt(tcIndex);
        emit endRemoveRows();
    }
}

void ToolChainModel::markForAddition(ToolChain *tc)
{
    const int pos = m_manualRoot->childNodes.size();
    emit beginInsertRows(index(m_manualRoot.data()), pos, pos);

    const ToolChainNodePtr node(new ToolChainNode(tc));
    m_manualRoot->addChild(node);
    node->changed = true;
    m_toAddList.append(node);

    emit endInsertRows();
}

QModelIndex ToolChainModel::index(ToolChainNode *node, int column) const
{
    if (!node->parent)
        return index(node == m_autoRoot.data() ? 0 : 1, column, QModelIndex());
    else {
        const int tcIndex = indexOfToolChainNode(node->parent->childNodes, node);
        QTC_ASSERT(tcIndex != -1, return QModelIndex());
        return index(tcIndex, column, index(node->parent));
    }
}

void ToolChainModel::addToolChain(ToolChain *tc)
{
    const int tcIndex = indexOfToolChain(m_toAddList, tc);
    if (tcIndex != -1) {
        m_toAddList.removeAt(tcIndex);
        return;
    }
    const ToolChainNodePtr parent = tc->isAutoDetected() ? m_autoRoot : m_manualRoot;
    QTC_ASSERT(indexOfToolChain(parent->childNodes, tc) == -1, return ; )

    const int row = parent->childNodes.count();
    beginInsertRows(index(parent.data()), row, row);
    ToolChainNodePtr newNode(new ToolChainNode(tc, true));
    parent->addChild(newNode);
    endInsertRows();

    emit toolChainStateChanged();
}

void ToolChainModel::removeToolChain(ToolChain *tc)
{
    const int tcIndex = indexOfToolChain(m_toRemoveList, tc);
    if (tcIndex != -1)
        m_toRemoveList.removeAt(tcIndex);

    const ToolChainNodePtr parent = tc->isAutoDetected() ? m_autoRoot : m_manualRoot;
    const int row = indexOfToolChain(parent->childNodes, tc);
    if (row != -1) {
        beginRemoveRows(index(parent.data()), row, row);
        parent->childNodes.removeAt(row);
        endRemoveRows();
    }

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

    m_model = new ToolChainModel(m_ui->toolChainView);
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

    return m_configWidget;
}

void ToolChainOptionsPage::apply()
{
    if (m_model)
        m_model->apply();
}

void ToolChainOptionsPage::finish()
{
    if (m_model)
        m_model->discard();
}

bool ToolChainOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

void ToolChainOptionsPage::toolChainSelectionChanged(const QModelIndex &current,
                                                     const QModelIndex &previous)
{
    Q_UNUSED(previous);
    if (m_currentTcWidget) {
        m_configWidget->layout()->removeWidget(m_currentTcWidget);
        m_currentTcWidget->setVisible(false);
    }

    m_currentTcWidget = m_model->widget(current);

    if (m_currentTcWidget) {
        m_configWidget->layout()->addWidget(m_currentTcWidget);
        m_currentTcWidget->setVisible(true);
    }

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
