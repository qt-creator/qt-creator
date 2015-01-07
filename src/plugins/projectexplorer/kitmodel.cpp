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

#include "kitmodel.h"

#include "kit.h"
#include "kitmanagerconfigwidget.h"
#include "kitmanager.h"

#include <coreplugin/coreconstants.h>
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
        static QIcon warningIcon(QString::fromLatin1(Core::Constants::ICON_WARNING));
        static QIcon errorIcon(QString::fromLatin1(Core::Constants::ICON_ERROR));

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
                if (!widget->isValid())
                    return errorIcon;
                if (widget->hasWarning())
                    return warningIcon;
                return QIcon();
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

KitModel::KitModel(QBoxLayout *parentLayout, QObject *parent) :
    TreeModel(parent),
    m_parentLayout(parentLayout),
    m_defaultNode(0),
    m_keepUnique(true)
{
    auto root = new TreeItem(QStringList(tr("Name")));
    m_autoRoot = new TreeItem(QStringList(tr("Auto-detected")));
    m_manualRoot = new TreeItem(QStringList(tr("Manual")));
    root->appendChild(m_autoRoot);
    root->appendChild(m_manualRoot);
    setRootItem(root);

    foreach (Kit *k, KitManager::sortedKits())
        addKit(k);

    changeDefaultKit();

    connect(KitManager::instance(), SIGNAL(kitAdded(ProjectExplorer::Kit*)),
            this, SLOT(addKit(ProjectExplorer::Kit*)));
    connect(KitManager::instance(), SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(updateKit(ProjectExplorer::Kit*)));
    connect(KitManager::instance(), SIGNAL(unmanagedKitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(updateKit(ProjectExplorer::Kit*)));
    connect(KitManager::instance(), SIGNAL(kitRemoved(ProjectExplorer::Kit*)),
            this, SLOT(removeKit(ProjectExplorer::Kit*)));
    connect(KitManager::instance(), SIGNAL(defaultkitChanged()),
            this, SLOT(changeDefaultKit()));
}

Kit *KitModel::kit(const QModelIndex &index)
{
    TreeItem *n = itemFromIndex(index);
    return n && n->level() == 2 ? static_cast<KitNode *>(n)->widget->workingCopy() : 0;
}

QModelIndex KitModel::indexOf(Kit *k) const
{
    KitNode *n = findWorkingCopy(k);
    return indexFromItem(n);
}

void KitModel::setDefaultKit(const QModelIndex &index)
{
    TreeItem *n = itemFromIndex(index);
    if (n && n->level() == 2)
        setDefaultNode(static_cast<KitNode *>(n));
}

bool KitModel::isDefaultKit(const QModelIndex &index)
{
    TreeItem *n = itemFromIndex(index);
    return n && n->level() == 2 && n == m_defaultNode;
}

KitManagerConfigWidget *KitModel::widget(const QModelIndex &index)
{
    if (!index.isValid())
        return 0;
    KitNode *node = static_cast<KitNode *>(index.internalPointer());
    return node->widget;
}

bool KitModel::isDirty() const
{
    return Utils::anyOf(m_manualRoot->children(), [](TreeItem *n) {
        return static_cast<KitNode *>(n)->widget->isDirty();
    });
}

bool KitModel::isDirty(Kit *k) const
{
    KitNode *n = findWorkingCopy(k);
    return n ? n->widget->isDirty() : false;
}

void KitModel::validateKitNames()
{
    QHash<QString, int> nameHash;
    foreach (TreeItem *group, rootItem()->children()) {
        foreach (TreeItem *item, group->children()) {
            KitNode *n = static_cast<KitNode *>(item);
            const QString displayName = n->widget->displayName();
            if (nameHash.contains(displayName))
                ++nameHash[displayName];
            else
                nameHash.insert(displayName, 1);
        }
    }

    foreach (TreeItem *group, rootItem()->children()) {
        foreach (TreeItem *item, group->children()) {
            KitNode *n = static_cast<KitNode *>(item);
            const QString displayName = n->widget->displayName();
            n->widget->setHasUniqueName(nameHash.value(displayName) == 1);
        }
    }
}

void KitModel::apply()
{
    // Remove unused kits:
    QList<KitNode *> nodes = m_toRemoveList;
    foreach (KitNode *n, nodes)
        n->widget->removeKit();

    // Update kits:
    foreach (TreeItem *group, rootItem()->children()) {
        //QList<TreeItems *> nodes = rootItem()->children(); // These can be dirty due to being made default!
        //nodes.append(m_manualRoot->children());
        foreach (TreeItem *item, group->children()) {
            KitNode *n = static_cast<KitNode *>(item);
            Q_ASSERT(n->widget);
            if (n->widget->isDirty()) {
                n->widget->apply();
                updateItem(item);
            }
        }
    }
}

void KitModel::markForRemoval(Kit *k)
{
    KitNode *node = findWorkingCopy(k);
    if (!node)
        return;

    if (node == m_defaultNode) {
        TreeItem *newDefault = 0;
        if (!m_autoRoot->children().isEmpty())
            newDefault = m_autoRoot->children().at(0);
        else if (!m_manualRoot->children().isEmpty())
            newDefault = m_manualRoot->children().at(0);
        setDefaultNode(static_cast<KitNode *>(newDefault));
    }

    removeItem(node);
    if (node->widget->configures(0))
        delete node;
    else
        m_toRemoveList.append(node);
}

Kit *KitModel::markForAddition(Kit *baseKit)
{
    KitNode *node = createNode(0);
    appendItem(m_manualRoot, node);
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
    foreach (TreeItem *group, rootItem()->children()) {
        foreach (TreeItem *item, group->children()) {
            KitNode *n = static_cast<KitNode *>(item);
            if (n->widget->workingCopy() == k)
                return n;
        }
    }
    return 0;
}

KitNode *KitModel::createNode(Kit *k)
{
    KitNode *node = new KitNode(k);
    m_parentLayout->addWidget(node->widget);
    connect(node->widget, &KitManagerConfigWidget::dirty, [this, node] { updateItem(node); });
    return node;
}

void KitModel::setDefaultNode(KitNode *node)
{
    if (m_defaultNode) {
        m_defaultNode->widget->setIsDefaultKit(false);
        updateItem(m_defaultNode);
    }
    m_defaultNode = node;
    if (m_defaultNode) {
        m_defaultNode->widget->setIsDefaultKit(true);
        updateItem(m_defaultNode);
    }
}

void KitModel::addKit(Kit *k)
{
    foreach (TreeItem *n, m_manualRoot->children()) {
        // Was added by us
        if (static_cast<KitNode *>(n)->widget->configures(k))
            return;
    }

    TreeItem *parent = k->isAutoDetected() ? m_autoRoot : m_manualRoot;
    appendItem(parent, createNode(k));

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

    KitNode *node = 0;
    foreach (TreeItem *group, rootItem()->children()) {
        foreach (TreeItem *item, group->children()) {
            KitNode *n = static_cast<KitNode *>(item);
            if (n->widget->configures(k)) {
                node = n;
                break;
            }
        }
    }

    if (m_defaultNode == node)
        m_defaultNode = 0;
    removeItem(node);
    delete node;

    validateKitNames();
    emit kitStateChanged();
}

void KitModel::changeDefaultKit()
{
    Kit *defaultKit = KitManager::defaultKit();
    foreach (TreeItem *group, rootItem()->children()) {
        foreach (TreeItem *item, group->children()) {
            KitNode *n = static_cast<KitNode *>(item);
            if (n->widget->configures(defaultKit)) {
                setDefaultNode(n);
                return;
            }
        }
    }
}

} // namespace Internal
} // namespace ProjectExplorer
