// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "kitaspect.h"

#include "devicesupport/devicekitaspects.h"
#include "devicesupport/idevice.h"
#include "kit.h"
#include "kitmanager.h"
#include "projectexplorertr.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/guard.h>
#include <utils/layoutbuilder.h>
#include <utils/treemodel.h>

#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

#include <utility>

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
        const auto getValues = [&]<typename T>(KitAspect::ItemRole role, const T & /* dummy */) {
            return std::make_pair(
                sourceModel()->data(source_left, role).value<T>(),
                sourceModel()->data(source_right, role).value<T>());
        };

        // Criterion 1: "None" comes last.
        if (getValue(source_left, KitAspect::IsNoneRole).toBool())
            return false;
        if (getValue(source_right, KitAspect::IsNoneRole).toBool())
            return true;

        // Criterion 2: "Type", which is is the name of some category by which the entries
        //              are supposed to get grouped together.
        if (const auto [type1, type2] = getValues(KitAspect::TypeRole, QString()); type1 != type2)
            return type1 < type2;

        // Criterion 3: "Quality", i.e. how likely is the respective entry to be usable.
        if (const auto [qual1, qual2] = getValues(KitAspect::QualityRole, int()); qual1 != qual2)
            return qual1 > qual2;

        // Criterion 4: Name.
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
    Private(Kit *kit, const KitAspectFactory *f)
        : kit(kit)
        , factory(f)
    {}

    Kit *kit;
    const KitAspectFactory * const factory;
    QAction *mutableAction = nullptr;
    Utils::Id managingPageId;
    QPushButton *manageButton = nullptr;
    Utils::Guard ignoreChanges;
    QList<KitAspect *> aspectsToEmbed;

    struct ListAspect
    {
        ListAspect(const ListAspectSpec &spec, QComboBox *comboBox)
            : spec(spec)
            , comboBox(comboBox)
        {}
        ListAspectSpec spec;
        QComboBox *comboBox;
    };
    QList<ListAspect> listAspects;

    bool readOnly = false;
};

KitAspect::KitAspect(Kit *k, const KitAspectFactory *factory)
    : d(new Private(k, factory))
{
    connect(KitManager::instance(), &KitManager::kitRemoved, this, [this](Kit *k) {
        if (k == d->kit)
            d->kit = nullptr;
    });

    const Id id = factory->id();
    d->mutableAction = new QAction(Tr::tr("Mark as Mutable"));
    d->mutableAction->setCheckable(true);
    d->mutableAction->setChecked(k->isMutable(id));
    d->mutableAction->setEnabled(!k->isSticky(id));
    connect(d->mutableAction, &QAction::toggled, this, [this, id] {
        if (Kit *k = kit())
            k->setMutable(id, d->mutableAction->isChecked());
    });
}

KitAspect::~KitAspect()
{
    delete d->mutableAction;
    delete d;
}

void KitAspect::refresh()
{
    if (d->listAspects.isEmpty() || d->ignoreChanges.isLocked())
        return;
    const GuardLocker locker(d->ignoreChanges);

    Kit *k = kit();
    if (!k)
        return;

    for (const Private::ListAspect &la : std::as_const(d->listAspects)) {
        la.spec.resetModel();
        la.comboBox->model()->sort(0);
        const QVariant itemId = la.spec.getter(*k);
        int idx = la.comboBox->findData(itemId, IdRole);
        if (idx == -1)
            idx = la.comboBox->count() - 1;
        la.comboBox->setCurrentIndex(idx);
        la.comboBox->setEnabled(!d->readOnly && la.comboBox->count() > 1);
    }
}

void KitAspect::makeStickySubWidgetsReadOnly()
{
    auto kit = this->kit();
    QTC_ASSERT(kit, return);

    if (!kit->isSticky(d->factory->id()))
        return;

    if (d->manageButton)
        d->manageButton->setEnabled(false);

    d->readOnly = true;
    makeReadOnly();
}

void KitAspect::makeReadOnly()
{
    for (const Private::ListAspect &la : std::as_const(d->listAspects))
        la.comboBox->setEnabled(false);
}

void KitAspect::addToInnerLayout(Layouting::Layout &layout)
{
    addListAspectsToLayout(layout);
}

void KitAspect::addListAspectSpec(const ListAspectSpec &listAspectSpec)
{
    const auto comboBox = createSubWidget<QComboBox>();
    const auto sortModel = new KitAspectSortModel(this);
    sortModel->setSourceModel(listAspectSpec.model);
    comboBox->setModel(sortModel);
    comboBox->setMinimumContentsLength(15);
    comboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    d->listAspects.emplaceBack(listAspectSpec, comboBox);

    refresh();

    const auto updateTooltip = [comboBox] {
        comboBox->setToolTip(
            comboBox->itemData(comboBox->currentIndex(), Qt::ToolTipRole).toString());
    };
    updateTooltip();
    connect(comboBox, &QComboBox::currentIndexChanged,
        this, [this, listAspectSpec, comboBox, updateTooltip] {
            if (d->ignoreChanges.isLocked())
                return;
            updateTooltip();

            if (Kit *k = kit())
                listAspectSpec.setter(*k, comboBox->itemData(comboBox->currentIndex(), IdRole));
        });
    connect(listAspectSpec.model, &QAbstractItemModel::modelAboutToBeReset,
            this, [this] { d->ignoreChanges.lock(); });
    connect(listAspectSpec.model, &QAbstractItemModel::modelReset,
            this, [this] { d->ignoreChanges.unlock(); });
}

QList<QComboBox *> KitAspect::comboBoxes() const
{
    return Utils::transform(d->listAspects, &Private::ListAspect::comboBox);
}

void KitAspect::addLabelToLayout(Layouting::Layout &layout)
{
    auto label = createSubWidget<QLabel>(d->factory->displayName() + ':');
    label->setToolTip(d->factory->description());
    connect(label, &QLabel::linkActivated, this, [this](const QString &link) {
        emit labelLinkActivated(link);
    });

    layout.addItem(label);
}

void KitAspect::addListAspectsToLayout(Layouting::Layout &layout)
{
    for (const Private::ListAspect &la : std::as_const(d->listAspects)) {
        addMutableAction(la.comboBox);
        layout.addItem(la.comboBox);
    }
}

void KitAspect::addManageButtonToLayout(Layouting::Layout &layout)
{
    if (d->managingPageId.isValid()) {
        d->manageButton = createSubWidget<QPushButton>(msgManage());
        connect(d->manageButton, &QPushButton::clicked, [this] {
            Core::ICore::showOptionsDialog(d->managingPageId, settingsPageItemToPreselect());
        });
        layout.addItem(d->manageButton);
    }
}

void KitAspect::addToLayoutImpl(Layouting::Layout &layout)
{
    addLabelToLayout(layout);
    addToInnerLayout(layout);
    addManageButtonToLayout(layout);

    layout.flush();
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

void KitAspect::setAspectsToEmbed(const QList<KitAspect *> &aspects)
{
    d->aspectsToEmbed = aspects;
}

QList<KitAspect *> KitAspect::aspectsToEmbed() const
{
    return d->aspectsToEmbed;
}

QString KitAspect::msgManage() { return Tr::tr("Manage..."); }
Kit *KitAspect::kit() const
{
    return d->kit;
}
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

QString KitAspectFactory::moduleForHeader(const Kit *k, const QString &className) const
{
    Q_UNUSED(k)
    Q_UNUSED(className)
    return {};
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

std::optional<Tasking::ExecutableItem> KitAspectFactory::autoDetect(
    Kit *kit,
    const Utils::FilePaths &searchPaths,
    const QString &detectionSource,
    const LogCallback &logCallback) const
{
    Q_UNUSED(kit);
    Q_UNUSED(searchPaths);
    Q_UNUSED(detectionSource);
    Q_UNUSED(logCallback);

    return std::nullopt;
}

std::optional<Tasking::ExecutableItem> KitAspectFactory::removeAutoDetected(
    const QString &detectionSource, const LogCallback &logCallback) const
{
    Q_UNUSED(detectionSource);
    Q_UNUSED(logCallback);
    return std::nullopt;
}

void KitAspectFactory::listAutoDetected(
    const QString &detectionSource, const LogCallback &logCallback) const
{
    Q_UNUSED(detectionSource)
    Q_UNUSED(logCallback)
}

Result<Tasking::ExecutableItem> KitAspectFactory::createAspectFromJson(
    const QString &detectionSource,
    const FilePath &rootPath,
    Kit *kit,
    const QJsonValue &json,
    const LogCallback &logCallback) const
{
    Q_UNUSED(detectionSource);
    Q_UNUSED(kit);
    Q_UNUSED(json);
    Q_UNUSED(logCallback);
    Q_UNUSED(rootPath);
    return ResultError(
        Tr::tr("Kit aspect factory '%1' does not support creating aspects from JSON.")
            .arg(id().toString()));
}

Tasking::Group kitDetectionRecipe(const IDeviceConstPtr &device, const LogCallback &logCallback)
{
    using namespace Tasking;
    using namespace Utils;

    Storage<GroupItems> detectorItems;
    Storage<Kit *> kit;

    const auto setup = [kit, detectorItems, device, logCallback] {
        const auto root = device->rootPath();

        const FilePaths searchPaths
            = Utils::transform(device->systemEnvironment().path(), [&root](const FilePath &path) {
                  return root.withNewPath(path.path());
              });

        const QString detectionSource = device->id().toString();

        logCallback(Tr::tr("Auto detecting Kits for device: %1").arg(device->displayName()));

        *kit = KitManager::registerKit([detectionSource, device](Kit *k) {
            k->setDetectionSource({DetectionSource::FromSystem, detectionSource});
            k->setUnexpandedDisplayName("%{Device:Name}");

            RunDeviceTypeKitAspect::setDeviceTypeId(k, device->type());
            RunDeviceKitAspect::setDevice(k, device);
            BuildDeviceTypeKitAspect::setDeviceTypeId(k, device->type());
            BuildDeviceKitAspect::setDevice(k, device);

            k->setSticky(BuildDeviceKitAspect::id(), true);
            k->setSticky(BuildDeviceTypeKitAspect::id(), true);
        });

        for (const auto &factory : KitAspectFactory::kitAspectFactories()) {
            const auto detector
                = factory->autoDetect(*kit, searchPaths, detectionSource, logCallback);
            if (detector)
                detectorItems->append({*detector});
        }
    };

    const auto setupDetectorTree = [detectorItems](TaskTree &tree) {
        tree.setRecipe(Group(*detectorItems));
    };

    // clang-format off
    return Group {
        kit, detectorItems,
        Sync(setup),
        TaskTreeTask(setupDetectorTree),
        Sync([kit,logCallback] {
            if (!(*kit)->isValid()) {
                KitManager::deregisterKit(*kit);
                return;
            }
            logCallback(Tr::tr("Found Kit: %1").arg((*kit)->displayName()));
        }),
    };
    // clang-format on
}

Tasking::Group removeDetectedKitsRecipe(const IDeviceConstPtr &device, const LogCallback &logCallback)
{
    using namespace Tasking;

    const auto root = device->rootPath();
    const QString detectionSource = device->id().toString();

    GroupItems removerItems{};
    for (const auto &factory : KitAspectFactory::kitAspectFactories()) {
        const auto remover = factory->removeAutoDetected(detectionSource, logCallback);
        if (remover)
            removerItems.append({remover.value()});
    }

    const auto removeKits = [device, detectionSource, logCallback]() {
        logCallback(Tr::tr("Removing kits for device: %1").arg(device->displayName()));

        const auto detectedKits = filtered(KitManager::kits(), [detectionSource](const Kit *k) {
            return k->detectionSource().id == detectionSource;
        });

        for (Kit *kit : detectedKits) {
            logCallback(Tr::tr("Removing kit: %1").arg(kit->displayName()));
            KitManager::deregisterKit(kit);
        }
    };

    // clang-format off
    return Group {
        Sync(removeKits),
        Group {
            parallelIdealThreadCountLimit,
            removerItems,
        }
    };
    // clang-format on
}

void listAutoDetected(const IDeviceConstPtr &device, const LogCallback &logCallback)
{
    const QString detectionSource = device->id().toString();

    for (const auto kit : KitManager::kits()) {
        if (kit->detectionSource().id == detectionSource)
            logCallback(Tr::tr("Kit: %1").arg(kit->displayName()));
    }

    for (const auto &factory : KitAspectFactory::kitAspectFactories())
        factory->listAutoDetected(detectionSource, logCallback);
}

QDebug operator<<(QDebug dbg, const DetectionSource &source)
{
    dbg.nospace() << "DetectionSource(";
    dbg.nospace() << "type: " << source.type;
    if (!source.id.isEmpty())
        dbg.nospace() << ", id: " << source.id;
    dbg.nospace() << ")";
    return dbg;
}

} // namespace ProjectExplorer
