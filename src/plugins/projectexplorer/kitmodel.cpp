// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "kitmodel.h"

#include "kit.h"
#include "kitmanagerconfigwidget.h"
#include "kitmanager.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QBoxLayout>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class KitNode : public TreeItem
{
public:
    KitNode(Kit *k, KitModel *m, QBoxLayout *parentLayout)
        : m_kit(k), m_model(m), m_parentLayout(parentLayout)
    {}

    ~KitNode() override { delete m_widget; }

    Kit *kit() const { return m_kit; }

    QVariant data(int, int role) const override
    {
        if (role == Qt::FontRole) {
            QFont f = QApplication::font();
            if (isDirty())
                f.setBold(!f.bold());
            if (isDefaultKit())
                f.setItalic(f.style() != QFont::StyleItalic);
            return f;
        }
        if (role == Qt::DisplayRole) {
            QString baseName = displayName();
            if (isDefaultKit())
                //: Mark up a kit as the default one.
                baseName = Tr::tr("%1 (default)").arg(baseName);
            return baseName;
        }

        if (role == Qt::DecorationRole)
            return displayIcon();

        if (role == Qt::ToolTipRole)
            return widget()->validityMessage();

        return {};
    }

    bool isDirty() const
    {
        if (m_widget)
            return m_widget->isDirty();
        return false;
    }

    QIcon displayIcon() const
    {
        if (m_widget)
            return m_widget->displayIcon();
        QTC_ASSERT(m_kit, return {});
        return m_kit->displayIcon();
    }

    QString displayName() const
    {
        if (m_widget)
            return m_widget->displayName();
        QTC_ASSERT(m_kit, return {});
        return m_kit->displayName();
    }

    bool isDefaultKit() const
    {
        return m_isDefaultKit;
    }

    bool isRegistering() const
    {
        if (m_widget)
            return m_widget->isRegistering();
        return false;
    }

    void setIsDefaultKit(bool on)
    {
        if (m_isDefaultKit == on)
            return;
        m_isDefaultKit = on;
        if (m_widget)
            emit m_widget->dirty();
    }

    KitManagerConfigWidget *widget() const
    {
        const_cast<KitNode *>(this)->ensureWidget();
        return m_widget;
    }

    void setHasUniqueName(bool on)
    {
        m_hasUniqueName = on;
    }

private:
    void ensureWidget()
    {
        if (m_widget)
            return;

        m_widget = new KitManagerConfigWidget(m_kit, m_isDefaultKit, m_hasUniqueName);

        QObject::connect(m_widget, &KitManagerConfigWidget::dirty, m_model, [this] { update(); });

        QObject::connect(m_widget, &KitManagerConfigWidget::isAutoDetectedChanged, m_model, [this] {
            TreeItem *oldParent = parent();
            TreeItem *newParent =
                m_model->rootItem()->childAt(m_widget->workingCopy()->isAutoDetected() ? 0 : 1);
            if (oldParent && oldParent != newParent) {
                m_model->takeItem(this);
                newParent->appendChild(this);
            }
        });
        m_parentLayout->addWidget(m_widget);
    }

    Kit *m_kit = m_kit;
    KitModel *m_model = nullptr;
    KitManagerConfigWidget *m_widget = nullptr;
    QBoxLayout *m_parentLayout = nullptr;
    bool m_isDefaultKit = false;
    bool m_hasUniqueName = true;
};

// --------------------------------------------------------------------------
// KitModel
// --------------------------------------------------------------------------

KitModel::KitModel(QBoxLayout *parentLayout, QObject *parent)
    : TreeModel<TreeItem, TreeItem, KitNode>(parent),
      m_parentLayout(parentLayout)
{
    setHeader(QStringList(Tr::tr("Name")));
    m_autoRoot = new StaticTreeItem({ProjectExplorer::Constants::msgAutoDetected()},
                                    {ProjectExplorer::Constants::msgAutoDetectedToolTip()});
    m_manualRoot = new StaticTreeItem(ProjectExplorer::Constants::msgManual());
    rootItem()->appendChild(m_autoRoot);
    rootItem()->appendChild(m_manualRoot);

    const QList<Kit *> kits = KitManager::sortKits(KitManager::kits());
    for (Kit *k : kits)
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
    return n ? n->widget()->workingCopy() : nullptr;
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
    return m_defaultNode && m_defaultNode->widget()->workingCopy() == k;
}

KitManagerConfigWidget *KitModel::widget(const QModelIndex &index)
{
    KitNode *n = kitNode(index);
    return n ? n->widget() : nullptr;
}

void KitModel::validateKitNames()
{
    QHash<QString, int> nameHash;
    forItemsAtLevel<2>([&nameHash](KitNode *n) {
        const QString displayName = n->displayName();
        if (nameHash.contains(displayName))
            ++nameHash[displayName];
        else
            nameHash.insert(displayName, 1);
    });

    forItemsAtLevel<2>([&nameHash](KitNode *n) {
        const QString displayName = n->displayName();
        n->setHasUniqueName(nameHash.value(displayName) == 1);
    });
}

void KitModel::apply()
{
    // Add/update dirty nodes before removing kits. This ensures the right kit ends up as default.
    forItemsAtLevel<2>([](KitNode *n) {
        if (n->isDirty()) {
            n->widget()->apply();
            n->update();
        }
    });

    // Remove unused kits:
    const QList<KitNode *> removeList = m_toRemoveList;
    for (KitNode *n : removeList)
        KitManager::deregisterKit(n->kit());

    emit layoutChanged(); // Force update.
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
    if (node->kit() == nullptr)
        delete node;
    else
        m_toRemoveList.append(node);
    validateKitNames();
}

Kit *KitModel::markForAddition(Kit *baseKit)
{
    const QString newName = newKitName(baseKit ? baseKit->unexpandedDisplayName() : QString());
    KitNode *node = createNode(nullptr);
    m_manualRoot->appendChild(node);
    Kit *k = node->widget()->workingCopy();
    KitGuard g(k);
    if (baseKit) {
        k->copyFrom(baseKit);
        k->setAutoDetected(false); // Make sure we have a manual kit!
        k->setSdkProvided(false);
    } else {
        k->setup();
    }
    k->setUnexpandedDisplayName(newName);

    if (!m_defaultNode)
        setDefaultNode(node);

    return k;
}

void KitModel::updateVisibility()
{
    forItemsAtLevel<2>([](const TreeItem *ti) {
        static_cast<const KitNode *>(ti)->widget()->updateVisibility();
    });
}

QString KitModel::newKitName(const QString &sourceName) const
{
    QList<Kit *> allKits;
    forItemsAtLevel<2>([&allKits](const TreeItem *ti) {
        allKits << static_cast<const KitNode *>(ti)->widget()->workingCopy();
    });
    return Kit::newKitName(sourceName, allKits);
}

KitNode *KitModel::findWorkingCopy(Kit *k) const
{
    return findItemAtLevel<2>([k](KitNode *n) { return n->widget()->workingCopy() == k; });
}

KitNode *KitModel::createNode(Kit *k)
{
    auto node = new KitNode(k, this, m_parentLayout);
    return node;
}

void KitModel::setDefaultNode(KitNode *node)
{
    if (m_defaultNode) {
        m_defaultNode->setIsDefaultKit(false);
        m_defaultNode->update();
    }
    m_defaultNode = node;
    if (m_defaultNode) {
        m_defaultNode->setIsDefaultKit(true);
        m_defaultNode->update();
    }
}

void KitModel::addKit(Kit *k)
{
    for (TreeItem *n : *m_manualRoot) {
        // Was added by us
        if (static_cast<KitNode *>(n)->isRegistering())
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
    for (KitNode *n : std::as_const(nodes)) {
        if (n->kit() == k) {
            m_toRemoveList.removeOne(n);
            if (m_defaultNode == n)
                m_defaultNode = nullptr;
            delete n;
            validateKitNames();
            return;
        }
    }

    KitNode *node = findItemAtLevel<2>([k](KitNode *n) {
        return n->kit() == k;
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
        return n->kit() == defaultKit;
    });
    setDefaultNode(node);
}

} // namespace Internal
} // namespace ProjectExplorer
