/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "kitmodel.h"

#include "kit.h"
#include "kitconfigwidget.h"
#include "kitmanager.h"

#include <utils/qtcassert.h>

#include <QApplication>
#include <QLayout>
#include <QMessageBox>

namespace ProjectExplorer {
namespace Internal {

class KitNode
{
public:
    explicit KitNode(KitNode *pn, Kit *k = 0, bool c = false) :
        parent(pn), kit(k), changed(c)
    {
        if (pn)
            pn->childNodes.append(this);
        widget = KitManager::instance()->createConfigWidget(k);
        if (widget) {
            if (k && k->isAutoDetected())
                widget->makeReadOnly();
            widget->setVisible(false);
        }
    }

    ~KitNode()
    {
        if (parent)
            parent->childNodes.removeOne(this);

        // deleting a child removes it from childNodes
        // so operate on a temporary list
        QList<KitNode *> tmp = childNodes;
        qDeleteAll(tmp);
        Q_ASSERT(childNodes.isEmpty());
    }

    KitNode *parent;
    QString newName;
    QList<KitNode *> childNodes;
    Kit *kit;
    KitConfigWidget *widget;
    bool changed;
};

// --------------------------------------------------------------------------
// KitModel
// --------------------------------------------------------------------------

KitModel::KitModel(QBoxLayout *parentLayout, QObject *parent) :
    QAbstractItemModel(parent),
    m_parentLayout(parentLayout),
    m_defaultNode(0)
{
    Q_ASSERT(m_parentLayout);

    connect(KitManager::instance(), SIGNAL(kitAdded(ProjectExplorer::Kit*)),
            this, SLOT(addKit(ProjectExplorer::Kit*)));
    connect(KitManager::instance(), SIGNAL(kitRemoved(ProjectExplorer::Kit*)),
            this, SLOT(removeKit(ProjectExplorer::Kit*)));
    connect(KitManager::instance(), SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(updateKit(ProjectExplorer::Kit*)));
    connect(KitManager::instance(), SIGNAL(defaultkitChanged()),
            this, SLOT(changeDefaultKit()));

    m_root = new KitNode(0);
    m_autoRoot = new KitNode(m_root);
    m_manualRoot = new KitNode(m_root);

    foreach (Kit *k, KitManager::instance()->kits())
        addKit(k);

    changeDefaultKit();
}

KitModel::~KitModel()
{
    delete m_root;
}

QModelIndex KitModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row >= 0 && row < m_root->childNodes.count())
            return createIndex(row, column, m_root->childNodes.at(row));
    }
    KitNode *node = static_cast<KitNode *>(parent.internalPointer());
    if (row < node->childNodes.count() && column == 0)
        return createIndex(row, column, node->childNodes.at(row));
    else
        return QModelIndex();
}

QModelIndex KitModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QModelIndex();
    KitNode *node = static_cast<KitNode *>(idx.internalPointer());
    if (node->parent == m_root)
        return QModelIndex();
    return index(node->parent);
}

int KitModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_root->childNodes.count();
    KitNode *node = static_cast<KitNode *>(parent.internalPointer());
    return node->childNodes.count();
}

int KitModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant KitModel::data(const QModelIndex &index, int role) const
{
    static QIcon warningIcon(":/projectexplorer/images/compile_warning.png");

    if (!index.isValid() || index.column() != 0)
        return QVariant();

    KitNode *node = static_cast<KitNode *>(index.internalPointer());
    QTC_ASSERT(node, return QVariant());
    if (node == m_autoRoot && role == Qt::DisplayRole)
        return tr("Auto-detected");
    if (node == m_manualRoot && role == Qt::DisplayRole)
        return tr("Manual");
    if (node->kit) {
        if (role == Qt::FontRole) {
            QFont f = QApplication::font();
            if (node->changed)
                f.setBold(!f.bold());
            if (node == m_defaultNode)
                f.setItalic(f.style() != QFont::StyleItalic);
            return f;
        } else if (role == Qt::DisplayRole) {
            QString baseName = node->newName.isEmpty() ? node->kit->displayName() : node->newName;
            if (node == m_defaultNode)
                //: Mark up a kit as the default one.
                baseName = tr("%1 (default)").arg(baseName);
            return baseName;
        } else if (role == Qt::EditRole) {
            return node->newName.isEmpty() ? node->kit->displayName() : node->newName;
        } else if (role == Qt::DecorationRole) {
            return node->kit->isValid() ? QIcon() : warningIcon;
        } else if (role == Qt::ToolTipRole) {
            return node->kit->toHtml();
        }
    }
    return QVariant();
}

bool KitModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    KitNode *node = static_cast<KitNode *>(index.internalPointer());
    Q_ASSERT(node);
    if (index.column() != 0 || !node->kit || role != Qt::EditRole)
        return false;
    node->newName = value.toString();
    if (!node->newName.isEmpty() && node->newName != node->kit->displayName())
        node->changed = true;
    return true;
}

Qt::ItemFlags KitModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    KitNode *node = static_cast<KitNode *>(index.internalPointer());
    Q_ASSERT(node);
    if (!node->kit)
        return Qt::ItemIsEnabled;

    if (node->kit->isAutoDetected())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QVariant KitModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return tr("Name");
    return QVariant();
}

Kit *KitModel::kit(const QModelIndex &index)
{
    if (!index.isValid())
        return 0;
    KitNode *node = static_cast<KitNode *>(index.internalPointer());
    Q_ASSERT(node);
    return node->kit;
}

QModelIndex KitModel::indexOf(Kit *k) const
{
    KitNode *n = find(k);
    return n ? index(n) : QModelIndex();
}

void KitModel::setDefaultKit(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    KitNode *node = static_cast<KitNode *>(index.internalPointer());
    Q_ASSERT(node);
    if (node->kit)
        setDefaultNode(node);
}

bool KitModel::isDefaultKit(const QModelIndex &index)
{
    return m_defaultNode == static_cast<KitNode *>(index.internalPointer());
}

KitConfigWidget *KitModel::widget(const QModelIndex &index)
{
    if (!index.isValid())
        return 0;
    KitNode *node = static_cast<KitNode *>(index.internalPointer());
    Q_ASSERT(node);
    return node->widget;
}

bool KitModel::isDirty() const
{
    foreach (KitNode *n, m_manualRoot->childNodes) {
        if (n->changed)
            return true;
    }
    return false;
}

bool KitModel::isDirty(Kit *k) const
{
    KitNode *n = find(k);
    return n ? !n->changed : false;
}

void KitModel::setDirty()
{
    KitConfigWidget *w = qobject_cast<KitConfigWidget *>(sender());
    foreach (KitNode *n, m_manualRoot->childNodes) {
        if (n->widget == w) {
            n->changed = true;
            emit dataChanged(index(n, 0), index(n, columnCount(QModelIndex())));
        }
    }
}

void KitModel::apply()
{
    // Remove unused kits:
    QList<KitNode *> nodes = m_toRemoveList;
    foreach (KitNode *n, nodes) {
        Q_ASSERT(!n->parent);
        KitManager::instance()->deregisterKit(n->kit);
    }
    Q_ASSERT(m_toRemoveList.isEmpty());

    // Update kits:
    foreach (KitNode *n, m_manualRoot->childNodes) {
        Q_ASSERT(n);
        Q_ASSERT(n->kit);
        if (n->changed) {
            KitManager::instance()->blockSignals(true);
            if (!n->newName.isEmpty()) {
                n->kit->setDisplayName(n->newName);
                n->newName.clear();
            }
            if (n->widget)
                n->widget->apply();
            n->changed = false;

            KitManager::instance()->blockSignals(false);
            KitManager::instance()->notifyAboutUpdate(n->kit);
            emit dataChanged(index(n, 0), index(n, columnCount(QModelIndex())));
        }
    }

    // Add new (and already updated) kits
    QStringList removedSts;
    nodes = m_toAddList;
    foreach (KitNode *n, nodes) {
        if (!KitManager::instance()->registerKit(n->kit))
            removedSts << n->kit->displayName();
    }

    foreach (KitNode *n, m_toAddList)
        markForRemoval(n->kit);

    if (removedSts.count() == 1) {
        QMessageBox::warning(0,
                             tr("Duplicate Kit Detected"),
                             tr("The kit<br>&nbsp;%1<br>"
                                " was already configured. It was not configured again.")
                             .arg(removedSts.at(0)));

    } else if (!removedSts.isEmpty()) {
        QMessageBox::warning(0,
                             tr("Duplicate Kits Detected"),
                             tr("The following kits were already configured:<br>"
                                "&nbsp;%1<br>"
                                "They were not configured again.")
                             .arg(removedSts.join(QLatin1String(",<br>&nbsp;"))));
    }

    // Set default kit:
    if (m_defaultNode)
        KitManager::instance()->setDefaultKit(m_defaultNode->kit);
}

void KitModel::markForRemoval(Kit *k)
{
    KitNode *node = find(k);
    if (!node)
        return;

    if (node == m_defaultNode) {
        KitNode *newDefault = 0;
        if (!m_autoRoot->childNodes.isEmpty())
            newDefault = m_autoRoot->childNodes.at(0);
        else if (!m_manualRoot->childNodes.isEmpty())
            newDefault = m_manualRoot->childNodes.at(0);
        setDefaultNode(newDefault);
    }

    beginRemoveRows(index(m_manualRoot), m_manualRoot->childNodes.indexOf(node), m_manualRoot->childNodes.indexOf(node));
    m_manualRoot->childNodes.removeOne(node);
    node->parent = 0;
    if (m_toAddList.contains(node)) {
        delete node->kit;
        node->kit = 0;
        m_toAddList.removeOne(node);
        delete node;
    } else {
        m_toRemoveList.append(node);
    }
    endRemoveRows();
}

void KitModel::markForAddition(Kit *k)
{
    int pos = m_manualRoot->childNodes.size();
    beginInsertRows(index(m_manualRoot), pos, pos);

    KitNode *node = createNode(m_manualRoot, k, true);
    m_toAddList.append(node);

    if (!m_defaultNode)
        setDefaultNode(node);

    endInsertRows();
}

QModelIndex KitModel::index(KitNode *node, int column) const
{
    if (node->parent == 0) // is root (or was marked for deletion)
        return QModelIndex();
    else if (node->parent == m_root)
        return index(m_root->childNodes.indexOf(node), column, QModelIndex());
    else
        return index(node->parent->childNodes.indexOf(node), column, index(node->parent));
}

KitNode *KitModel::find(Kit *k) const
{
    foreach (KitNode *n, m_autoRoot->childNodes) {
        if (n->kit == k)
            return n;
    }
    foreach (KitNode *n, m_manualRoot->childNodes) {
        if (n->kit == k)
            return n;
    }
    return 0;
}

KitNode *KitModel::createNode(KitNode *parent, Kit *k, bool changed)
{
    KitNode *node = new KitNode(parent, k, changed);
    if (node->widget) {
        node->widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_parentLayout->addWidget(node->widget, 10);
        connect(node->widget, SIGNAL(dirty()),
                this, SLOT(setDirty()));
    }
    return node;
}

void KitModel::setDefaultNode(KitNode *node)
{
    if (m_defaultNode) {
        QModelIndex idx = index(m_defaultNode);
        if (idx.isValid())
            emit dataChanged(idx, idx);
    }
    m_defaultNode = node;
    if (m_defaultNode) {
        QModelIndex idx = index(m_defaultNode);
        if (idx.isValid())
            emit dataChanged(idx, idx);
    }
}

void KitModel::addKit(Kit *k)
{
    QList<KitNode *> nodes = m_toAddList;
    foreach (KitNode *n, nodes) {
        if (n->kit == k) {
            m_toAddList.removeOne(n);
            // do not delete n: Still used elsewhere!
            return;
        }
    }

    KitNode *parent = m_manualRoot;
    if (k->isAutoDetected())
        parent = m_autoRoot;
    int row = parent->childNodes.count();

    beginInsertRows(index(parent), row, row);
    createNode(parent, k, false);
    endInsertRows();

    emit kitStateChanged();
}

void KitModel::removeKit(Kit *k)
{
    QList<KitNode *> nodes = m_toRemoveList;
    foreach (KitNode *n, nodes) {
        if (n->kit == k) {
            m_toRemoveList.removeOne(n);
            delete n;
            return;
        }
    }

    KitNode *parent = m_manualRoot;
    if (k->isAutoDetected())
        parent = m_autoRoot;
    int row = 0;
    KitNode *node = 0;
    foreach (KitNode *current, parent->childNodes) {
        if (current->kit == k) {
            node = current;
            break;
        }
        ++row;
    }

    beginRemoveRows(index(parent), row, row);
    parent->childNodes.removeAt(row);
    delete node;
    endRemoveRows();

    emit kitStateChanged();
}

void KitModel::updateKit(Kit *k)
{
    KitNode *n = find(k);
    // This can happen if Qt Versions and kits are removed simultaneously.
    if (!n)
        return;
    if (n->widget)
        n->widget->discard();
    QModelIndex idx = index(n);
    emit dataChanged(idx, idx);
}

void KitModel::changeDefaultKit()
{
    setDefaultNode(find(KitManager::instance()->defaultKit()));
}

} // namespace Internal
} // namespace ProjectExplorer
