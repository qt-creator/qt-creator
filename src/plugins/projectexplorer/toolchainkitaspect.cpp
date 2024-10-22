// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolchainkitaspect.h"

#include "devicesupport/idevice.h"
#include "kit.h"
#include "kitaspect.h"
#include "kitaspects.h"
#include "kitmanager.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "toolchainmanager.h"
#include "toolchainoptionspage.h"

#include <utils/guard.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/stringutils.h>

#include <QComboBox>
#include <QGridLayout>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class ToolchainListModel : public TreeModel<ToolchainTreeItem>
{
public:
    ToolchainListModel(const Kit &kit, const LanguageCategory &category, QObject *parent)
        : TreeModel(parent)
        , m_kit(kit)
        , m_category(category)
    {
        reset();
    }

    void reset()
    {
        clear();

        const Toolchains ltcList = ToolchainManager::toolchains(
            [this](const Toolchain *tc) { return m_category.contains(tc->language()); });
        IDeviceConstPtr device = BuildDeviceKitAspect::device(&m_kit);

        const QList<Toolchain *> toolchainsForBuildDevice
            = Utils::filtered(ltcList, [device](Toolchain *tc) {
                  return tc->compilerCommand().isSameDevice(device->rootPath());
              });
        const QList<ToolchainBundle> bundlesForBuildDevice = ToolchainBundle::collectBundles(
            toolchainsForBuildDevice, ToolchainBundle::HandleMissing::CreateAndRegister);
        for (const ToolchainBundle &b : bundlesForBuildDevice)
            rootItem()->appendChild(new ToolchainTreeItem(b));
        rootItem()->appendChild(new ToolchainTreeItem);
    }

private:
    const Kit &m_kit;
    const LanguageCategory m_category;
};

class ToolchainSortModel : public SortModel
{
public:
    ToolchainSortModel(QObject *parent) : SortModel(parent) {}

    void reset() { static_cast<ToolchainListModel *>(sourceModel())->reset(); }

private:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override
    {
        const auto source = static_cast<ToolchainListModel *>(sourceModel());
        const ToolchainTreeItem *item1 = source->itemForIndex(source_left);
        const ToolchainTreeItem *item2 = source->itemForIndex(source_right);
        QTC_ASSERT(item1 && item2, return false);
        if (!item1->bundle)
            return false;
        if (!item2->bundle)
            return true;
        if (item1->bundle->type() != item2->bundle->type()) {
            return caseFriendlyCompare(
                       item1->bundle->typeDisplayName(), item2->bundle->typeDisplayName())
                   < 0;
        }
        return SortModel::lessThan(source_left, source_right);
    }
};

class ToolchainKitAspectImpl final : public KitAspect
{
public:
    ToolchainKitAspectImpl(Kit *k, const KitAspectFactory *factory) : KitAspect(k, factory)
    {
        m_mainWidget = createSubWidget<QWidget>();
        m_mainWidget->setContentsMargins(0, 0, 0, 0);

        auto layout = new QGridLayout(m_mainWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setColumnStretch(1, 2);

        const QList<LanguageCategory> languageCategories = sorted(
            ToolchainManager::languageCategories(),
            [](const LanguageCategory &l1, const LanguageCategory &l2) {
                return ToolchainManager::displayNameOfLanguageCategory(l1)
                < ToolchainManager::displayNameOfLanguageCategory(l2);
            });
        QTC_ASSERT(!languageCategories.isEmpty(), return);
        int row = 0;
        for (const LanguageCategory &lc : std::as_const(languageCategories)) {
            layout->addWidget(
                new QLabel(ToolchainManager::displayNameOfLanguageCategory(lc) + ':'), row, 0);
            auto cb = new QComboBox;
            cb->setSizePolicy(QSizePolicy::Ignored, cb->sizePolicy().verticalPolicy());
            cb->setToolTip(factory->description());
            setWheelScrollingWithoutFocusBlocked(cb);

            const auto model = new ToolchainListModel(*kit(), lc, this);
            const auto sortModel = new ToolchainSortModel(this);
            sortModel->setSourceModel(model);
            cb->setModel(sortModel);

            m_languageComboboxMap.insert(lc, cb);
            layout->addWidget(cb, row, 1);
            ++row;

            connect(cb, &QComboBox::currentIndexChanged, this, [this, lc](int idx) {
                currentToolchainChanged(lc, idx);
            });
        }

        refresh();

        setManagingPage(Constants::TOOLCHAIN_SETTINGS_PAGE_ID);
    }

    ~ToolchainKitAspectImpl() override
    {
        delete m_mainWidget;
    }

private:
    void addToInnerLayout(Layouting::Layout &builder) override
    {
        addMutableAction(m_mainWidget);
        builder.addItem(m_mainWidget);
    }

    void refresh() override
    {
        IDeviceConstPtr device = BuildDeviceKitAspect::device(kit());

        const GuardLocker locker(m_ignoreChanges);
        for (auto it = m_languageComboboxMap.cbegin(); it != m_languageComboboxMap.cend(); ++it) {
            const LanguageCategory lc = it.key();
            const Toolchains ltcList = ToolchainManager::toolchains(
                [lc](const Toolchain *tc) { return lc.contains(tc->language()); });

            QComboBox *cb = *it;
            static_cast<ToolchainSortModel *>(cb->model())->reset();
            cb->model()->sort(0);
            cb->setEnabled(cb->count() > 1 && !m_isReadOnly);

            Id currentBundleId;
            for (const Id lang : lc) {
                if (Toolchain * const currentTc = ToolchainKitAspect::toolchain(kit(), lang)) {
                    currentBundleId = currentTc->bundleId();
                    break;
                }
            }
            cb->setCurrentIndex(indexOf(cb, currentBundleId));
        }
    }

    void makeReadOnly() override
    {
        m_isReadOnly = true;
        for (QComboBox *cb : std::as_const(m_languageComboboxMap))
            cb->setEnabled(false);
    }

    void currentToolchainChanged(const LanguageCategory &languageCategory, int idx)
    {
        if (m_ignoreChanges.isLocked() || idx < 0)
            return;

        const QAbstractItemModel *const model
            = m_languageComboboxMap.value(languageCategory)->model();
        const Id bundleId = Id::fromSetting(
            model->data(model->index(idx, 0), ToolchainTreeItem::BundleIdRole));

        const Toolchains bundleTcs = ToolchainManager::toolchains(
            [bundleId](const Toolchain *tc) { return tc->bundleId() == bundleId; });
        for (const Id lang : languageCategory) {
            Toolchain *const tc = Utils::findOrDefault(bundleTcs, [lang](const Toolchain *tc) {
                return tc->language() == lang;
            });
            if (tc)
                ToolchainKitAspect::setToolchain(kit(), tc);
            else
                ToolchainKitAspect::clearToolchain(kit(), lang);
        }
    }

    int indexOf(QComboBox *cb, Id bundleId)
    {
        return cb->findData(bundleId.toSetting(), ToolchainTreeItem::BundleIdRole);
    }

    QWidget *m_mainWidget = nullptr;
    QHash<LanguageCategory, QComboBox *> m_languageComboboxMap;
    Guard m_ignoreChanges;
    bool m_isReadOnly = false;
};

class ToolchainKitAspectFactory : public KitAspectFactory
{
public:
    ToolchainKitAspectFactory();

private:
    Tasks validate(const Kit *k) const override;
    void fix(Kit *k) override;
    void setup(Kit *k) override;

    KitAspect *createKitAspect(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    void addToBuildEnvironment(const Kit *k, Environment &env) const override;
    void addToRunEnvironment(const Kit *, Environment &) const override {}

    void addToMacroExpander(Kit *kit, MacroExpander *expander) const override;
    QList<OutputLineParser *> createOutputParsers(const Kit *k) const override;
    QSet<Id> availableFeatures(const Kit *k) const override;

    void onKitsLoaded() override;

    void toolChainUpdated(Toolchain *tc);
    void toolChainsDeregistered();
};

ToolchainKitAspectFactory::ToolchainKitAspectFactory()
{
    setId(ToolchainKitAspect::id());
    setDisplayName(Tr::tr("Compiler"));
    setDescription(Tr::tr("The compiler to use for building.<br>"
                          "Make sure the compiler will produce binaries compatible "
                          "with the target device, Qt version and other libraries used."));
    setPriority(30000);
}

Tasks ToolchainKitAspectFactory::validate(const Kit *k) const
{
    Tasks result;

    const QList<Toolchain*> tcList = ToolchainKitAspect::toolChains(k);
    if (tcList.isEmpty()) {
        result << BuildSystemTask(Task::Warning, ToolchainKitAspect::msgNoToolchainInTarget());
    } else {
        QSet<Abi> targetAbis;
        for (const Toolchain *tc : tcList) {
            targetAbis.insert(tc->targetAbi());
            result << tc->validateKit(k);
        }
        if (targetAbis.count() != 1) {
            result << BuildSystemTask(Task::Error,
                                      Tr::tr("Compilers produce code for different ABIs: %1")
                                          .arg(Utils::transform<QList>(targetAbis, &Abi::toString).join(", ")));
        }
    }
    return result;
}

void ToolchainKitAspectFactory::fix(Kit *k)
{
    QTC_ASSERT(ToolchainManager::isLoaded(), return);
    const QList<Id> languages = ToolchainManager::allLanguages();
    for (const Id l : languages) {
        const QByteArray tcId = ToolchainKitAspect::toolchainId(k, l);
        if (!tcId.isEmpty() && !ToolchainManager::findToolchain(tcId)) {
            qWarning("Tool chain set up in kit \"%s\" for \"%s\" not found.",
                     qPrintable(k->displayName()),
                     qPrintable(ToolchainManager::displayNameOfLanguageId(l)));
            ToolchainKitAspect::clearToolchain(k, l); // make sure to clear out no longer known tool chains
        }
    }
}

static Id findLanguage(const QString &ls)
{
    QString lsUpper = ls.toUpper();
    return Utils::findOrDefault(ToolchainManager::allLanguages(),
                                [lsUpper](Id l) { return lsUpper == l.toString().toUpper(); });
}

using LanguageAndAbi = std::pair<Id, Abi>;
using LanguagesAndAbis = QList<LanguageAndAbi>;

static void setToolchainsFromAbis(Kit *k, const LanguagesAndAbis &abisByLanguage)
{
    if (abisByLanguage.isEmpty())
        return;

    // First transform languages into categories, so we can work on the bundle level.
    // Obviously, we assume that the caller does not specify different ABIs for
    // languages from the same category.
    const QList<LanguageCategory> allCategories = ToolchainManager::languageCategories();
    QHash<LanguageCategory, Abi> abisByCategory;
    for (const LanguageAndAbi &langAndAbi : abisByLanguage) {
        const auto category
            = Utils::findOrDefault(allCategories, [&langAndAbi](const LanguageCategory &cat) {
                  return cat.contains(langAndAbi.first);
              });
        QTC_ASSERT(!category.isEmpty(), continue);
        abisByCategory.insert(category, langAndAbi.second);
    }

    // Get bundles.
    const QList<ToolchainBundle> bundles = ToolchainBundle::collectBundles(
        ToolchainBundle::HandleMissing::CreateAndRegister);

    // Set a matching bundle for each LanguageCategory/Abi pair, if possible.
    for (auto it = abisByCategory.cbegin(); it != abisByCategory.cend(); ++it) {
        const QList<ToolchainBundle> matchingBundles
            = Utils::filtered(bundles, [&it](const ToolchainBundle &b) {
                  return b.factory()->languageCategory() == it.key() && b.targetAbi() == it.value();
              });

        if (matchingBundles.isEmpty()) {
            for (const Id language : it.key())
                ToolchainKitAspect::clearToolchain(k, language);
            continue;
        }

        const auto bestBundle = std::min_element(
            matchingBundles.begin(), matchingBundles.end(), &ToolchainManager::isBetterToolchain);
        ToolchainKitAspect::setBundle(k, *bestBundle);
    }
}

static void setMissingToolchainsToHostAbi(Kit *k, const QList<Id> &languageBlacklist)
{
    LanguagesAndAbis abisByLanguage;
    for (const Id lang : ToolchainManager::allLanguages()) {
        if (languageBlacklist.contains(lang) || ToolchainKitAspect::toolchain(k, lang))
            continue;
        abisByLanguage.emplaceBack(lang, Abi::hostAbi());
    }
    setToolchainsFromAbis(k, abisByLanguage);
}

static void setupForSdkKit(Kit *k)
{
    const Store value = storeFromVariant(k->value(ToolchainKitAspect::id()));
    bool lockToolchains = !value.isEmpty();

    // The installer provides two kinds of entries for toolchains:
    //   a) An actual toolchain id, for e.g. Boot2Qt where the installer ships the toolchains.
    //   b) An ABI string, for Desktop Qt. In this case, it is our responsibility to find
    //      a matching toolchain on the host system.
    LanguagesAndAbis abisByLanguage;
    for (auto i = value.constBegin(); i != value.constEnd(); ++i) {
        const Id lang = findLanguage(stringFromKey(i.key()));

        if (!lang.isValid()) {
            lockToolchains = false;
            continue;
        }

        const QByteArray id = i.value().toByteArray();
        if (ToolchainManager::findToolchain(id))
            continue;

        // No toolchain with this id exists. Check whether it's an ABI string.
        lockToolchains = false;
        const Abi abi = Abi::fromString(QString::fromUtf8(id));
        if (!abi.isValid())
            continue;

        abisByLanguage.emplaceBack(lang, abi);
    }
    setToolchainsFromAbis(k, abisByLanguage);
    setMissingToolchainsToHostAbi(k, Utils::transform(abisByLanguage, &LanguageAndAbi::first));

    k->setSticky(ToolchainKitAspect::id(), lockToolchains);
}

static void setupForNonSdkKit(Kit *k)
{
    setMissingToolchainsToHostAbi(k, {});
    k->setSticky(ToolchainKitAspect::id(), false);
}

void ToolchainKitAspectFactory::setup(Kit *k)
{
    QTC_ASSERT(ToolchainManager::isLoaded(), return);
    QTC_ASSERT(k, return);

    if (k->isSdkProvided())
        setupForSdkKit(k);
    else
        setupForNonSdkKit(k);
}

KitAspect *ToolchainKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new ToolchainKitAspectImpl(k, this);
}

QString ToolchainKitAspectFactory::displayNamePostfix(const Kit *k) const
{
    Toolchain *tc = ToolchainKitAspect::cxxToolchain(k);
    return tc ? tc->displayName() : QString();
}

KitAspectFactory::ItemList ToolchainKitAspectFactory::toUserOutput(const Kit *k) const
{
    Toolchain *tc = ToolchainKitAspect::cxxToolchain(k);
    return {{Tr::tr("Compiler"), tc ? tc->displayName() : Tr::tr("None")}};
}

void ToolchainKitAspectFactory::addToBuildEnvironment(const Kit *k, Environment &env) const
{
    Toolchain *tc = ToolchainKitAspect::cxxToolchain(k);
    if (tc)
        tc->addToEnvironment(env);
}

void ToolchainKitAspectFactory::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    QTC_ASSERT(kit, return);

    // Compatibility with Qt Creator < 4.2:
    expander->registerVariable("Compiler:Name", Tr::tr("Compiler"),
                               [kit] {
                                   const Toolchain *tc = ToolchainKitAspect::cxxToolchain(kit);
                                   return tc ? tc->displayName() : Tr::tr("None");
                               });

    expander->registerVariable("Compiler:Executable", Tr::tr("Path to the compiler executable"),
                               [kit] {
                                   const Toolchain *tc = ToolchainKitAspect::cxxToolchain(kit);
                                   return tc ? tc->compilerCommand().path() : QString();
                               });

    // After 4.2
    expander->registerPrefix("Compiler:Name", Tr::tr("Compiler for different languages"),
                             [kit](const QString &ls) {
                                 const Toolchain *tc = ToolchainKitAspect::toolchain(kit, findLanguage(ls));
                                 return tc ? tc->displayName() : Tr::tr("None");
                             });
    expander->registerPrefix("Compiler:Executable", Tr::tr("Compiler executable for different languages"),
                             [kit](const QString &ls) {
                                 const Toolchain *tc = ToolchainKitAspect::toolchain(kit, findLanguage(ls));
                                 return tc ? tc->compilerCommand().path() : QString();
                             });
}

QList<OutputLineParser *> ToolchainKitAspectFactory::createOutputParsers(const Kit *k) const
{
    for (const Id langId : {Id(Constants::CXX_LANGUAGE_ID), Id(Constants::C_LANGUAGE_ID)}) {
        if (const Toolchain * const tc = ToolchainKitAspect::toolchain(k, langId))
            return tc->createOutputParsers();
    }
    return {};
}

QSet<Id> ToolchainKitAspectFactory::availableFeatures(const Kit *k) const
{
    QSet<Id> result;
    for (Toolchain *tc : ToolchainKitAspect::toolChains(k))
        result.insert(tc->typeId().withPrefix("ToolChain."));
    return result;
}

void ToolchainKitAspectFactory::onKitsLoaded()
{
    for (Kit *k : KitManager::kits())
        fix(k);

    connect(ToolchainManager::instance(), &ToolchainManager::toolchainsDeregistered,
            this, &ToolchainKitAspectFactory::toolChainsDeregistered);
    connect(ToolchainManager::instance(), &ToolchainManager::toolchainUpdated,
            this, &ToolchainKitAspectFactory::toolChainUpdated);
}

void ToolchainKitAspectFactory::toolChainUpdated(Toolchain *tc)
{
    for (Kit *k : KitManager::kits()) {
        if (ToolchainKitAspect::toolchain(k, tc->language()) == tc)
            notifyAboutUpdate(k);
    }
}

void ToolchainKitAspectFactory::toolChainsDeregistered()
{
    for (Kit *k : KitManager::kits())
        fix(k);
}

const ToolchainKitAspectFactory thsToolChainKitAspectFactory;
} // namespace Internal

Id ToolchainKitAspect::id()
{
    // "PE.Profile.ToolChain" until 4.2
    // "PE.Profile.ToolChains" temporarily before 4.3 (May 2017)
    return "PE.Profile.ToolChainsV3";
}

QByteArray ToolchainKitAspect::toolchainId(const Kit *k, Id language)
{
    QTC_ASSERT(ToolchainManager::isLoaded(), return nullptr);
    if (!k)
        return {};
    Store value = storeFromVariant(k->value(ToolchainKitAspect::id()));
    return value.value(language.toKey(), QByteArray()).toByteArray();
}

Toolchain *ToolchainKitAspect::toolchain(const Kit *k, Id language)
{
    return ToolchainManager::findToolchain(toolchainId(k, language));
}

Toolchain *ToolchainKitAspect::cToolchain(const Kit *k)
{
    return ToolchainManager::findToolchain(toolchainId(k, ProjectExplorer::Constants::C_LANGUAGE_ID));
}

Toolchain *ToolchainKitAspect::cxxToolchain(const Kit *k)
{
    return ToolchainManager::findToolchain(toolchainId(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID));
}

QList<Toolchain *> ToolchainKitAspect::toolChains(const Kit *k)
{
    QTC_ASSERT(k, return {});

    const Store value = storeFromVariant(k->value(ToolchainKitAspect::id()));
    const QList<Toolchain *> tcList
            = transform<QList>(ToolchainManager::allLanguages(), [&value](Id l) {
                return ToolchainManager::findToolchain(value.value(l.toKey()).toByteArray());
            });
    return filtered(tcList, [](Toolchain *tc) { return tc; });
}

void ToolchainKitAspect::setToolchain(Kit *k, Toolchain *tc)
{
    QTC_ASSERT(tc, return);
    QTC_ASSERT(k, return);
    Store result = storeFromVariant(k->value(ToolchainKitAspect::id()));
    result.insert(tc->language().toKey(), tc->id());

    k->setValue(id(), variantFromStore(result));
}

void ToolchainKitAspect::setBundle(Kit *k, const ToolchainBundle &bundle)
{
    bundle.forEach<Toolchain>([k](Toolchain &tc) {
        setToolchain(k, &tc);
    });
}

void ToolchainKitAspect::clearToolchain(Kit *k, Id language)
{
    QTC_ASSERT(language.isValid(), return);
    QTC_ASSERT(k, return);

    Store result = storeFromVariant(k->value(ToolchainKitAspect::id()));
    result.insert(language.toKey(), QByteArray());
    k->setValue(id(), variantFromStore(result));
}

Abi ToolchainKitAspect::targetAbi(const Kit *k)
{
    const QList<Toolchain *> tcList = toolChains(k);
    // Find the best possible ABI for all the tool chains...
    Abi cxxAbi;
    QHash<Abi, int> abiCount;
    for (Toolchain *tc : tcList) {
        Abi ta = tc->targetAbi();
        if (tc->language() == Id(Constants::CXX_LANGUAGE_ID))
            cxxAbi = tc->targetAbi();
        abiCount[ta] = (abiCount.contains(ta) ? abiCount[ta] + 1 : 1);
    }
    QVector<Abi> candidates;
    int count = -1;
    candidates.reserve(tcList.count());
    for (auto i = abiCount.begin(); i != abiCount.end(); ++i) {
        if (i.value() > count) {
            candidates.clear();
            candidates.append(i.key());
            count = i.value();
        } else if (i.value() == count) {
            candidates.append(i.key());
        }
    }

    // Found a good candidate:
    if (candidates.isEmpty())
        return Abi::hostAbi();
    if (candidates.contains(cxxAbi)) // Use Cxx compiler as a tie breaker
        return cxxAbi;
    return candidates.at(0); // Use basically a random Abi...
}

QString ToolchainKitAspect::msgNoToolchainInTarget()
{
    return Tr::tr("No compiler set in kit.");
}

} // namespace ProjectExplorer
