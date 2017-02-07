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

#include "kitmodel.h"

#include "kit.h"
#include "kitmanagerconfigwidget.h"
#include "kitmanager.h"

#include <utils/utilsicons.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QLayout>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class KitNode : public TreeItem
{
public:
    KitNode(Kit *k)
    {
        widget = KitManager::createConfigWidget(k);
        if (widget) {
            if (k && k->isAutoDetected())
                widget->makeStickySubWidgetsReadOnly();
            widget->setVisible(false);
        }
    }

    ~KitNode()
    {
        delete widget;
    }

    QVariant data(int, int role) const
    {
        if (widget) {
            if (role == Qt::FontRole) {
                QFont f = QApplication::font();
                if (widget->isDirty())
                    f.setBold(!f.bold());
                if (widget->isDefaultKit())
                    f.setItalic(f.style() != QFont::StyleItalic);
                return f;
            }
            if (role == Qt::DisplayRole) {
                QString baseName = widget->displayName();
                if (widget->isDefaultKit())
                    //: Mark up a kit as the default one.
                    baseName = KitModel::tr("%1 (default)").arg(baseName);
                return baseName;
            }
            if (role == Qt::DecorationRole) {
                if (!widget->isValid()) {
                    static const QIcon errorIcon(Utils::Icons::CRITICAL.icon());
                    return errorIcon;
                }
                if (widget->hasWarning()) {
                    static const QIcon warningIcon(Utils::Icons::WARNING.icon());
                    return warningIcon;
                }
                return widget->icon();
            }
            if (role == Qt::ToolTipRole) {
                return widget->validityMessage();
            }
        }
        return QVariant();
    }

    KitManagerConfigWidget *widget;
};

// --------------------------------------------------------------------------
// KitModel
// --------------------------------------------------------------------------

KitModel::KitModel(QBoxLayout *parentLayout, QObject *parent)
    : TreeModel<TreeItem, TreeItem, KitNode>(parent),
      m_parentLayout(parentLayout)
{
    setHeader(QStringList(tr("Name")));
    m_autoRoot = new StaticTreeItem(tr("Auto-detected"));
    m_manualRoot = new StaticTreeItem(tr("Manual"));
    rootItem()->appendChild(m_autoRoot);
    rootItem()->appendChild(m_manualRoot);

    foreach (Kit *k, KitManager::sortKits(KitManager::kits()))
        addKit(k);

    changeDefaultKit();

    connect(KitManager::instance(), &KitManager::kitAdded,
            this, &KitModel::addKit);
    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &KitModel::updateKit);
    connect(KitManager::instance(), &KitManager::unmanagedKitUpdated,
            this, &KitModel::updateKit);
    connect(KitManager::instance(), &KitManager::kitRemoved,
            this, &KitModel::removeKit);
    connect(KitManager::instance(), &KitManager::defaultkitChanged,
            this, &KitModel::changeDefaultKit);
}

Kit *KitModel::kit(const QModelIndex &index)
{
    KitNode *n = kitNode(index);
    return n ? n->widget->workingCopy() : 0;
}

KitNode *KitModel::kitNode(const QModelIndex &index)
{
    TreeItem *n = itemForIndex(index);
    return (n && n->level() == 2) ? static_cast<KitNode *>(n) : nullptr;
}

QModelIndex KitModel::indexOf(Kit *k) const
{
    KitNode *n = findWorkingCopy(k);
    return n ? indexForItem(n) : QModelIndex();
}

void KitModel::setDefaultKit(const QModelIndex &index)
{
    if (KitNode *n = kitNode(index))
        setDefaultNode(n);
}

bool KitModel::isDefaultKit(Kit *k) const
{
    return m_defaultNode && m_defaultNode->widget->workingCopy() == k;
}

KitManagerConfigWidget *KitModel::widget(const QModelIndex &index)
{
    KitNode *n = kitNode(index);
    return n ? n->widget : 0;
}

void KitModel::isAutoDetectedChanged()
{
    auto w = qobject_cast<KitManagerConfigWidget *>(sender());
    int idx = -1;
    idx = Utils::indexOf(*m_manualRoot, [w](TreeItem *node) {
        return static_cast<KitNode *>(node)->widget == w;
    });
    TreeItem *oldParent = nullptr;
    TreeItem *newParent = w->workingCopy()->isAutoDetected() ? m_autoRoot : m_manualRoot;
    if (idx != -1) {
        oldParent = m_manualRoot;
    } else {
        idx = Utils::indexOf(*m_autoRoot, [w](TreeItem *node) {
            return static_cast<KitNode *>(node)->widget == w;
        });
        if (idx != -1) {
            oldParent = m_autoRoot;
        }
    }

    if (oldParent && oldParent != newParent) {
        beginMoveRows(indexForItem(oldParent), idx, idx, indexForItem(newParent), newParent->childCount());
        TreeItem *n = oldParent->childAt(idx);
        takeItem(n);
        newParent->appendChild(n);
        endMoveRows();
    }
}

void KitModel::validateKitNames()
{
    QHash<QString, int> nameHash;
    forItemsAtLevel<2>([&nameHash](KitNode *n) {
        const QString displayName = n->widget->displayName();
        if (nameHash.contains(displayName))
            ++nameHash[displayName];
        else
            nameHash.insert(displayName, 1);
    });

    forItemsAtLevel<2>([&nameHash](KitNode *n) {
        const QString displayName = n->widget->displayName();
        n->widget->setHasUniqueName(nameHash.value(displayName) == 1);
    });
}

void KitModel::apply()
{
    // Add/update dirty nodes before removing kits. This ensures the right kit ends up as default.
    forItemsAtLevel<2>([](KitNode *n) {
        if (n->widget->isDirty()) {
            n->widget->apply();
            n->update();
        }
    });

    // Remove unused kits:
    foreach (KitNode *n, m_toRemoveList)
        n->widget->removeKit();

    layoutChanged(); // Force update.
}

void KitModel::markForRemoval(Kit *k)
{
    KitNode *node = findWorkingCopy(k);
    if (!node)
        return;

    if (node == m_defaultNode) {
        TreeItem *newDefault = m_autoRoot->firstChild();
        if (!newDefault)
            newDefault = m_manualRoot->firstChild();
        setDefaultNode(static_cast<KitNode *>(newDefault));
    }

    if (node == m_defaultNode)
        setDefaultNode(findItemAtLevel<2>([node](KitNode *kn) { return kn != node; }));

    takeItem(node);
    if (node->widget->configures(0))
        delete node;
    else
        m_toRemoveList.append(node);
}

Kit *KitModel::markForAddition(Kit *baseKit)
{
    KitNode *node = createNode(0);
    m_manualRoot->appendChild(node);
    Kit *k = node->widget->workingCopy();
    KitGuard g(k);
    if (baseKit) {
        k->copyFrom(baseKit);
        k->setAutoDetected(false); // Make sure we have a manual kit!
        k->setSdkProvided(false);
        k->setUnexpandedDisplayName(tr("Clone of %1").arg(k->unexpandedDisplayName()));
    } else {
        k->setup();
    }

    if (!m_defaultNode)
        setDefaultNode(node);

    return k;
}

KitNode *KitModel::findWorkingCopy(Kit *k) const
{
    return findItemAtLevel<2>([k](KitNode *n) { return n->widget->workingCopy() == k; });
}

KitNode *KitModel::createNode(Kit *k)
{
    auto node = new KitNode(k);
    m_parentLayout->addWidget(node->widget);
    connect(node->widget, &KitManagerConfigWidget::dirty, [this, node] {
        if (m_autoRoot->indexOf(node) != -1 || m_manualRoot->indexOf(node) != -1)
            node->update();
    });
    connect(node->widget, &KitManagerConfigWidget::isAutoDetectedChanged,
            this, &KitModel::isAutoDetectedChanged);

    return node;
}

void KitModel::setDefaultNode(KitNode *node)
{
    if (m_defaultNode) {
        m_defaultNode->widget->setIsDefaultKit(false);
        m_defaultNode->update();
    }
    m_defaultNode = node;
    if (m_defaultNode) {
        m_defaultNode->widget->setIsDefaultKit(true);
        m_defaultNode->update();
    }
}

void KitModel::addKit(Kit *k)
{
    for (TreeItem *n : *m_manualRoot) {
        // Was added by us
        if (static_cast<KitNode *>(n)->widget->configures(k))
            return;
    }

    TreeItem *parent = k->isAutoDetected() ? m_autoRoot : m_manualRoot;
    parent->appendChild(createNode(k));

    validateKitNames();
    emit kitStateChanged();
}

void KitModel::updateKit(Kit *)
{
    validateKitNames();
    emit kitStateChanged();
}

void KitModel::removeKit(Kit *k)
{
    QList<KitNode *> nodes = m_toRemoveList;
    foreach (KitNode *n, nodes) {
        if (n->widget->configures(k)) {
            m_toRemoveList.removeOne(n);
            if (m_defaultNode == n)
                m_defaultNode = 0;
            delete n;
            return;
        }
    }

    KitNode *node = findItemAtLevel<2>([k](KitNode *n) {
        return n->widget->configures(k);
    });

    if (node == m_defaultNode)
        setDefaultNode(findItemAtLevel<2>([node](KitNode *kn) { return kn != node; }));

    destroyItem(node);

    validateKitNames();
    emit kitStateChanged();
}

void KitModel::changeDefaultKit()
{
    Kit *defaultKit = KitManager::defaultKit();
    KitNode *node = findItemAtLevel<2>([defaultKit](KitNode *n) {
        return n->widget->configures(defaultKit);
    });
    setDefaultNode(node);
}

} // namespace Internal
} // namespace ProjectExplorer
