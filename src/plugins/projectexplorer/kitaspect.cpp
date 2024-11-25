// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "kitaspect.h"

#include "kit.h"
#include "kitaspects.h"
#include "projectexplorertr.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/treemodel.h>

#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

using namespace Utils;

namespace ProjectExplorer {

namespace {
class KitAspectSortModel : public SortModel
{
public:
    using SortModel::SortModel;

private:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override
    {
        const auto getValue = [&](const QModelIndex &index, KitAspect::ItemRole role) {
            return sourceModel()->data(index, role);
        };

        // Criterion 1: "None" comes last.
        if (getValue(source_left, KitAspect::IsNoneRole).toBool())
            return false;
        if (getValue(source_right, KitAspect::IsNoneRole).toBool())
            return true;

        // Criterion 2: "Quality", i.e. how likely is the respective entry to be usable.
        if (const int qual1 = getValue(source_left, KitAspect::QualityRole).toInt(),
            qual2 = getValue(source_right, KitAspect::QualityRole).toInt();
            qual1 != qual2) {
            return qual1 > qual2;
        }

        // Criterion 3: Name.
        return SortModel::lessThan(source_left, source_right);
    }
};

class KitAspectFactories
{
public:
    void onKitsLoaded() const
    {
        for (KitAspectFactory *factory : m_aspectList)
            factory->onKitsLoaded();
    }

    void addKitAspect(KitAspectFactory *factory)
    {
        QTC_ASSERT(!m_aspectList.contains(factory), return);
        m_aspectList.append(factory);
        m_aspectListIsSorted = false;
    }

    void removeKitAspect(KitAspectFactory *factory)
    {
        int removed = m_aspectList.removeAll(factory);
        QTC_CHECK(removed == 1);
    }

    const QList<KitAspectFactory *> kitAspectFactories()
    {
        if (!m_aspectListIsSorted) {
            Utils::sort(m_aspectList, [](const KitAspectFactory *a, const KitAspectFactory *b) {
                return a->priority() > b->priority();
            });
            m_aspectListIsSorted = true;
        }
        return m_aspectList;
    }

    // Sorted by priority, in descending order...
    QList<KitAspectFactory *> m_aspectList;
    // ... if this here is set:
    bool m_aspectListIsSorted = true;
};

static KitAspectFactories &kitAspectFactoriesStorage()
{
    static KitAspectFactories theKitAspectFactories;
    return theKitAspectFactories;
}

} // namespace

KitAspect::KitAspect(Kit *kit, const KitAspectFactory *factory)
    : m_kit(kit), m_factory(factory)
{
    const Id id = factory->id();
    m_mutableAction = new QAction(Tr::tr("Mark as Mutable"));
    m_mutableAction->setCheckable(true);
    m_mutableAction->setChecked(m_kit->isMutable(id));
    m_mutableAction->setEnabled(!m_kit->isSticky(id));
    connect(m_mutableAction, &QAction::toggled, this, [this, id] {
        m_kit->setMutable(id, m_mutableAction->isChecked());
    });
}

KitAspect::~KitAspect()
{
    delete m_mutableAction;
}

void KitAspect::refresh()
{
    if (!m_listAspectSpec || m_ignoreChanges.isLocked())
        return;
    const GuardLocker locker(m_ignoreChanges);
    m_listAspectSpec->resetModel();
    m_comboBox->model()->sort(0);
    const QVariant itemId = m_listAspectSpec->getter(*kit());
    m_comboBox->setCurrentIndex(m_comboBox->findData(itemId, IdRole));
}

void KitAspect::makeStickySubWidgetsReadOnly()
{
    if (!m_kit->isSticky(m_factory->id()))
        return;

    if (m_manageButton)
        m_manageButton->setEnabled(false);

    makeReadOnly();
}

void KitAspect::makeReadOnly()
{
    if (m_comboBox)
        m_comboBox->setEnabled(false);
}

void KitAspect::addToInnerLayout(Layouting::Layout &parentItem)
{
    if (m_comboBox) {
        addMutableAction(m_comboBox);
        parentItem.addItem(m_comboBox);
    }
}

void KitAspect::setListAspectSpec(ListAspectSpec &&listAspectSpec)
{
    m_listAspectSpec = std::move(listAspectSpec);

    m_comboBox = createSubWidget<QComboBox>();
    const auto sortModel = new KitAspectSortModel(this);
    sortModel->setSourceModel(m_listAspectSpec->model);
    m_comboBox->setModel(sortModel);

    refresh();

    const auto updateTooltip = [this] {
        m_comboBox->setToolTip(
            m_comboBox->itemData(m_comboBox->currentIndex(), Qt::ToolTipRole).toString());
    };
    updateTooltip();
    connect(m_comboBox, &QComboBox::currentIndexChanged, this, [this, updateTooltip] {
        if (m_ignoreChanges.isLocked())
            return;
        updateTooltip();
        m_listAspectSpec->setter(
            *kit(), m_comboBox->itemData(m_comboBox->currentIndex(), IdRole));
    });
    connect(m_listAspectSpec->model, &QAbstractItemModel::modelAboutToBeReset,
            this, [this] { m_ignoreChanges.lock(); });
    connect(m_listAspectSpec->model, &QAbstractItemModel::modelReset,
            this, [this] { m_ignoreChanges.unlock(); });
}

void KitAspect::addToLayoutImpl(Layouting::Layout &layout)
{
    auto label = createSubWidget<QLabel>(m_factory->displayName() + ':');
    label->setToolTip(m_factory->description());
    connect(label, &QLabel::linkActivated, this, [this](const QString &link) {
        emit labelLinkActivated(link);
    });

    layout.addItem(label);
    addToInnerLayout(layout);
    if (m_managingPageId.isValid()) {
        m_manageButton = createSubWidget<QPushButton>(msgManage());
        connect(m_manageButton, &QPushButton::clicked, [this] {
            Core::ICore::showOptionsDialog(m_managingPageId, settingsPageItemToPreselect());
        });
        layout.addItem(m_manageButton);
    }
    layout.addItem(Layouting::br);
}

void KitAspect::addMutableAction(QWidget *child)
{
    QTC_ASSERT(child, return);
    if (factory()->id() == DeviceKitAspect::id())
        return;
    child->addAction(m_mutableAction);
    child->setContextMenuPolicy(Qt::ActionsContextMenu);
}

QString KitAspect::msgManage()
{
    return Tr::tr("Manage...");
}

KitAspectFactory::KitAspectFactory()
{
    kitAspectFactoriesStorage().addKitAspect(this);
}

KitAspectFactory::~KitAspectFactory()
{
    kitAspectFactoriesStorage().removeKitAspect(this);
}

int KitAspectFactory::weight(const Kit *k) const
{
    return k->value(id()).isValid() ? 1 : 0;
}

void KitAspectFactory::addToBuildEnvironment(const Kit *k, Environment &env) const
{
    Q_UNUSED(k)
    Q_UNUSED(env)
}

void KitAspectFactory::addToRunEnvironment(const Kit *k, Environment &env) const
{
    Q_UNUSED(k)
    Q_UNUSED(env)
}

QList<OutputLineParser *> KitAspectFactory::createOutputParsers(const Kit *k) const
{
    Q_UNUSED(k)
    return {};
}

QString KitAspectFactory::displayNamePostfix(const Kit *k) const
{
    Q_UNUSED(k)
    return {};
}

QSet<Id> KitAspectFactory::supportedPlatforms(const Kit *k) const
{
    Q_UNUSED(k)
    return {};
}

QSet<Id> KitAspectFactory::availableFeatures(const Kit *k) const
{
    Q_UNUSED(k)
    return {};
}

void KitAspectFactory::addToMacroExpander(Kit *k, MacroExpander *expander) const
{
    Q_UNUSED(k)
    Q_UNUSED(expander)
}

void KitAspectFactory::notifyAboutUpdate(Kit *k)
{
    if (k)
        k->kitUpdated();
}

void KitAspectFactory::handleKitsLoaded()
{
    kitAspectFactoriesStorage().onKitsLoaded();
}

const QList<KitAspectFactory *> KitAspectFactory::kitAspectFactories()
{
    return kitAspectFactoriesStorage().kitAspectFactories();
}

} // namespace ProjectExplorer
