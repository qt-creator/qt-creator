// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "kitaspect.h"

#include "kit.h"
#include "kitaspects.h"
#include "projectexplorertr.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/guard.h>
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

class KitAspect::Private
{
public:
    Private(Kit *k, const KitAspectFactory *f) : kit(k), factory(f) {}

    Kit * const kit;
    const KitAspectFactory * const factory;
    QAction *mutableAction = nullptr;
    Utils::Id managingPageId;
    QPushButton *manageButton = nullptr;
    QComboBox *comboBox = nullptr;
    std::optional<ListAspectSpec> listAspectSpec;
    Utils::Guard ignoreChanges;
};

KitAspect::KitAspect(Kit *kit, const KitAspectFactory *factory)
    : d(new Private(kit, factory))
{
    const Id id = factory->id();
    d->mutableAction = new QAction(Tr::tr("Mark as Mutable"));
    d->mutableAction->setCheckable(true);
    d->mutableAction->setChecked(d->kit->isMutable(id));
    d->mutableAction->setEnabled(!d->kit->isSticky(id));
    connect(d->mutableAction, &QAction::toggled, this, [this, id] {
        d->kit->setMutable(id, d->mutableAction->isChecked());
    });
}

KitAspect::~KitAspect()
{
    delete d->mutableAction;
    delete d;
}

void KitAspect::refresh()
{
    if (!d->listAspectSpec || d->ignoreChanges.isLocked())
        return;
    const GuardLocker locker(d->ignoreChanges);
    d->listAspectSpec->resetModel();
    d->comboBox->model()->sort(0);
    const QVariant itemId = d->listAspectSpec->getter(*kit());
    d->comboBox->setCurrentIndex(d->comboBox->findData(itemId, IdRole));
}

void KitAspect::makeStickySubWidgetsReadOnly()
{
    if (!d->kit->isSticky(d->factory->id()))
        return;

    if (d->manageButton)
        d->manageButton->setEnabled(false);

    makeReadOnly();
}

void KitAspect::makeReadOnly()
{
    if (d->comboBox)
        d->comboBox->setEnabled(false);
}

void KitAspect::addToInnerLayout(Layouting::Layout &parentItem)
{
    if (d->comboBox) {
        addMutableAction(d->comboBox);
        parentItem.addItem(d->comboBox);
    }
}

void KitAspect::setListAspectSpec(ListAspectSpec &&listAspectSpec)
{
    d->listAspectSpec = std::move(listAspectSpec);

    d->comboBox = createSubWidget<QComboBox>();
    d->comboBox->setSizePolicy(QSizePolicy::Ignored, d->comboBox->sizePolicy().verticalPolicy());
    d->comboBox->setEnabled(true);
    const auto sortModel = new KitAspectSortModel(this);
    sortModel->setSourceModel(d->listAspectSpec->model);
    d->comboBox->setModel(sortModel);

    refresh();

    const auto updateTooltip = [this] {
        d->comboBox->setToolTip(
            d->comboBox->itemData(d->comboBox->currentIndex(), Qt::ToolTipRole).toString());
    };
    updateTooltip();
    connect(d->comboBox, &QComboBox::currentIndexChanged, this, [this, updateTooltip] {
        if (d->ignoreChanges.isLocked())
            return;
        updateTooltip();
        d->listAspectSpec->setter(
            *kit(), d->comboBox->itemData(d->comboBox->currentIndex(), IdRole));
    });
    connect(d->listAspectSpec->model, &QAbstractItemModel::modelAboutToBeReset,
            this, [this] { d->ignoreChanges.lock(); });
    connect(d->listAspectSpec->model, &QAbstractItemModel::modelReset,
            this, [this] { d->ignoreChanges.unlock(); });
}

void KitAspect::addToLayoutImpl(Layouting::Layout &layout)
{
    auto label = createSubWidget<QLabel>(d->factory->displayName() + ':');
    label->setToolTip(d->factory->description());
    connect(label, &QLabel::linkActivated, this, [this](const QString &link) {
        emit labelLinkActivated(link);
    });

    layout.addItem(label);
    addToInnerLayout(layout);
    if (d->managingPageId.isValid()) {
        d->manageButton = createSubWidget<QPushButton>(msgManage());
        connect(d->manageButton, &QPushButton::clicked, [this] {
            Core::ICore::showOptionsDialog(d->managingPageId, settingsPageItemToPreselect());
        });
        layout.addItem(d->manageButton);
    }
    layout.addItem(Layouting::br);
}

void KitAspect::addMutableAction(QWidget *child)
{
    QTC_ASSERT(child, return);
    if (factory()->id() == RunDeviceKitAspect::id())
        return;
    child->addAction(d->mutableAction);
    child->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void KitAspect::setManagingPage(Utils::Id pageId) { d->managingPageId = pageId; }

QString KitAspect::msgManage() { return Tr::tr("Manage..."); }
Kit *KitAspect::kit() const { return d->kit; }
const KitAspectFactory *KitAspect::factory() const { return d->factory; }
QAction *KitAspect::mutableAction() const { return d->mutableAction; }

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
