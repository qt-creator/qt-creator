// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtkitaspect.h"

#include "qtoptionspage.h"
#include "qtparser.h"
#include "qtsupportconstants.h"
#include "qtsupporttr.h"
#include "qttestparser.h"
#include "qtversionmanager.h"

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>
#include <projectexplorer/toolchainmanager.h>

#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <utils/algorithm.h>
#include <utils/buildablehelperlibrary.h>
#include <utils/guard.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QHBoxLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport {
namespace Internal {

class QtVersionListModel : public TreeModel<TreeItem, QtVersionItem>
{
public:
    QtVersionListModel(const Kit &kit, QObject *parent)
        : TreeModel(parent)
        , m_kit(kit)
    {}

    void reset()
    {
        clear();

        if (const IDevice::ConstPtr device = BuildDeviceKitAspect::device(&m_kit)) {
            const FilePath deviceRoot = device->rootPath();
            const QtVersions versionsForBuildDevice = QtVersionManager::versions(
                [&deviceRoot](const QtVersion *qt) {
                    return qt->qmakeFilePath().isSameDevice(deviceRoot);
                });
            for (QtVersion *v : versionsForBuildDevice)
                rootItem()->appendChild(new QtVersionItem(v->uniqueId()));
        }
        rootItem()->appendChild(new QtVersionItem(-1)); // The "No Qt" entry.
    }

private:
    const Kit &m_kit;
};

class QtKitAspectImpl final : public KitAspect
{
public:
    QtKitAspectImpl(Kit *k, const KitAspectFactory *ki) : KitAspect(k, ki)
    {
        setManagingPage(Constants::QTVERSION_SETTINGS_PAGE_ID);

        const auto model = new QtVersionListModel(*k, this);
        auto getter = [](const Kit &k) { return QtKitAspect::qtVersionId(&k); };
        auto setter = [](Kit &k, const QVariant &versionId) {
            QtKitAspect::setQtVersionId(&k, versionId.toInt());
        };
        auto resetModel = [model] { model->reset(); };
        addListAspectSpec({model, std::move(getter), std::move(setter), std::move(resetModel)});

        connect(KitManager::instance(), &KitManager::kitUpdated, this, [this](Kit *k) {
            if (k == kit())
                refresh();
        });
    }

private:
    void addToInnerLayout(Layouting::Layout &layout) override
    {
        if (const QList<KitAspect *> embedded = aspectsToEmbed(); !embedded.isEmpty()) {
            Layouting::Layout box(new QHBoxLayout);
            KitAspect::addToInnerLayout(box);
            QSizePolicy p = comboBoxes().first()->sizePolicy();
            p.setHorizontalStretch(2);
            comboBoxes().first()->setSizePolicy(p);
            box.addItem(createSubWidget<QLabel>(Tr::tr("Mkspec:")));
            embedded.first()->addToInnerLayout(box);
            layout.addItem(box);
        } else {
            KitAspect::addToInnerLayout(layout);
        }
    }
};
} // namespace Internal

class QtKitAspectFactory : public KitAspectFactory
{
public:
    QtKitAspectFactory();

private:
    void setup(Kit *k) override;

    Tasks validate(const Kit *k) const override;
    void fix(Kit *) override;

    KitAspect *createKitAspect(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    void addToBuildEnvironment(const Kit *k, Environment &env) const override;
    QList<OutputLineParser *> createOutputParsers(const Kit *k) const override;
    void addToMacroExpander(Kit *kit, MacroExpander *expander) const override;

    QSet<Id> supportedPlatforms(const Kit *k) const override;
    QSet<Id> availableFeatures(const Kit *k) const override;

    int weight(const Kit *k) const override;

    void qtVersionsChanged(const QList<int> &addedIds,
                           const QList<int> &removedIds,
                           const QList<int> &changedIds);
    void onKitsLoaded() override;
};

const QtKitAspectFactory theQtKitAspectFactory;

QtKitAspectFactory::QtKitAspectFactory()
{
    setId(QtKitAspect::id());
    setDisplayName(Tr::tr("Qt version"));
    setDescription(Tr::tr("The Qt library to use for all projects using this kit.<br>"
                          "A Qt version is required for qmake-based projects "
                          "and optional when using other build systems."));
    setPriority(26000);
    setEmbeddableAspects({QmakeProjectManager::Constants::KIT_INFORMATION_ID});
}

void QtKitAspectFactory::setup(Kit *k)
{
    if (!k || k->hasValue(id()))
        return;
    const Abi tcAbi = ToolchainKitAspect::targetAbi(k);
    const Id deviceType = RunDeviceTypeKitAspect::deviceTypeId(k);

    const QtVersions matches
            = QtVersionManager::versions([&tcAbi, &deviceType](const QtVersion *qt) {
        return qt->targetDeviceTypes().contains(deviceType)
                && Utils::contains(qt->qtAbis(), [&tcAbi](const Abi &qtAbi) {
            return qtAbi.isCompatibleWith(tcAbi); });
    });
    if (matches.empty())
        return;

    // An MSVC 2015 toolchain is compatible with an MSVC 2017 Qt, but we prefer an
    // MSVC 2015 Qt if we find one.
    const QtVersions exactMatches = Utils::filtered(matches, [&tcAbi](const QtVersion *qt) {
        return qt->qtAbis().contains(tcAbi);
    });
    const QtVersions &candidates = !exactMatches.empty() ? exactMatches : matches;

    // Prefer higher versions to lower ones.
    const QVersionNumber maxVersion
        = Utils::maxElementOrDefault(candidates, [](const QtVersion *v1, const QtVersion *v2) {
              return v1->qtVersion() < v2->qtVersion();
          })->qtVersion();
    const auto [highestVersions, lowerVersions]
        = Utils::partition(candidates, [&maxVersion](const QtVersion *v) {
              return v->qtVersion() == maxVersion;
          });

    QtVersion * const qtFromPath = QtVersionManager::version(
                equal(&QtVersion::detectionSource, QString("PATH")));
    if (qtFromPath && highestVersions.contains(qtFromPath))
        k->setValue(id(), qtFromPath->uniqueId());
    else
        k->setValue(id(), highestVersions.first()->uniqueId());
}

Tasks QtKitAspectFactory::validate(const Kit *k) const
{
    QTC_ASSERT(QtVersionManager::isLoaded(), return {});
    QtVersion *version = QtKitAspect::qtVersion(k);
    if (!version)
        return {};

    return version->validateKit(k);
}

void QtKitAspectFactory::fix(Kit *k)
{
    QTC_ASSERT(QtVersionManager::isLoaded(), return);
    QtVersion *version = QtKitAspect::qtVersion(k);
    if (!version) {
        if (QtKitAspect::qtVersionId(k) >= 0) {
            qWarning("Qt version is no longer known, removing from kit \"%s\".",
                     qPrintable(k->displayName()));
            QtKitAspect::setQtVersionId(k, -1);
        }
        return;
    }

    // Set a matching toolchain if we don't have one.
    if (ToolchainKitAspect::cxxToolchain(k))
        return;

    QList<ToolchainBundle> bundles = ToolchainBundle::collectBundles(
        ToolchainBundle::HandleMissing::CreateAndRegister);
    using ProjectExplorer::Constants::CXX_LANGUAGE_ID;
    bundles = Utils::filtered(bundles, [version](const ToolchainBundle &b) {
        if (!b.isCompletelyValid() || !b.factory()->languageCategory().contains(CXX_LANGUAGE_ID))
            return false;
        return Utils::anyOf(version->qtAbis(), [&b](const Abi &qtAbi) {
            return b.supportedAbis().contains(qtAbi)
                   && b.targetAbi().wordWidth() == qtAbi.wordWidth()
                   && b.targetAbi().architecture() == qtAbi.architecture();

        });
    });

    if (bundles.isEmpty())
        return;

    // Prefer exact matches.
    sort(bundles, [version](const ToolchainBundle &b1, const ToolchainBundle &b2) {
        const QVector<Abi> &qtAbis = version->qtAbis();
        const bool tc1ExactMatch = qtAbis.contains(b1.targetAbi());
        const bool tc2ExactMatch = qtAbis.contains(b2.targetAbi());
        if (tc1ExactMatch && !tc2ExactMatch)
            return true;
        if (!tc1ExactMatch && tc2ExactMatch)
            return false;

        // For a multi-arch Qt that support the host ABI, prefer toolchains that match
        // the host ABI.
        if (qtAbis.size() > 1 && qtAbis.contains(Abi::hostAbi())) {
            const bool tc1HasHostAbi = b1.targetAbi() == Abi::hostAbi();
            const bool tc2HasHostAbi = b2.targetAbi() == Abi::hostAbi();
            if (tc1HasHostAbi && !tc2HasHostAbi)
                return true;
            if (!tc1HasHostAbi && tc2HasHostAbi)
                return false;
        }

        return ToolchainManager::isBetterToolchain(b1, b2);
    });

    // TODO: Why is this not done during sorting?
    const QString spec = version->mkspec();
    const QList<ToolchainBundle> goodBundles
        = Utils::filtered(bundles, [&spec](const ToolchainBundle &b) {
              return b.get(&Toolchain::suggestedMkspecList).contains(spec);
          });

    const ToolchainBundle &bestBundle = goodBundles.isEmpty() ? bundles.first()
                                                              : goodBundles.first();
    ToolchainKitAspect::setBundle(k, bestBundle);
}

KitAspect *QtKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::QtKitAspectImpl(k, this);
}

QString QtKitAspectFactory::displayNamePostfix(const Kit *k) const
{
    QtVersion *version = QtKitAspect::qtVersion(k);
    return version ? version->displayName() : QString();
}

KitAspectFactory::ItemList QtKitAspectFactory::toUserOutput(const Kit *k) const
{
    QtVersion *version = QtKitAspect::qtVersion(k);
    return {
        {Tr::tr("Qt version"), version ? version->displayName() : Tr::tr("None", "No Qt version")}};
}

void QtKitAspectFactory::addToBuildEnvironment(const Kit *k, Environment &env) const
{
    QtVersion *version = QtKitAspect::qtVersion(k);
    if (version)
        version->addToBuildEnvironment(k, env);
}

QList<OutputLineParser *> QtKitAspectFactory::createOutputParsers(const Kit *k) const
{
    if (QtKitAspect::qtVersion(k))
        return {new Internal::QtTestParser, new QtParser};
    return {};
}

class QtMacroSubProvider
{
public:
    QtMacroSubProvider(Kit *kit)
        : expander(QtVersion::createMacroExpander(
              [kit] { return QtKitAspect::qtVersion(kit); }))
    {}

    MacroExpander *operator()() const
    {
        return expander.get();
    }

    std::shared_ptr<MacroExpander> expander;
};

void QtKitAspectFactory::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    QTC_ASSERT(kit, return);
    expander->registerSubProvider(QtMacroSubProvider(kit));

    expander->registerVariable("Qt:Name", Tr::tr("Name of Qt Version"),
                [kit]() -> QString {
                   QtVersion *version = QtKitAspect::qtVersion(kit);
                   return version ? version->displayName() : Tr::tr("unknown");
                });
    expander->registerVariable("Qt:qmakeExecutable", Tr::tr("Path to the qmake executable"),
                [kit]() -> QString {
                    QtVersion *version = QtKitAspect::qtVersion(kit);
                    return version ? version->qmakeFilePath().path() : QString();
                });
}

Id QtKitAspect::id()
{
    return "QtSupport.QtInformation";
}

int QtKitAspect::qtVersionId(const Kit *k)
{
    if (!k)
        return -1;

    int id = -1;
    QVariant data = k->value(QtKitAspect::id(), -1);
    if (data.typeId() == QMetaType::Int) {
        bool ok;
        id = data.toInt(&ok);
        if (!ok)
            id = -1;
    } else {
        QString source = data.toString();
        QtVersion *v = QtVersionManager::version([source](const QtVersion *v) { return v->detectionSource() == source; });
        if (v)
            id = v->uniqueId();
    }
    return id;
}

void QtKitAspect::setQtVersionId(Kit *k, const int id)
{
    QTC_ASSERT(k, return);
    k->setValue(QtKitAspect::id(), id);
}

QtVersion *QtKitAspect::qtVersion(const Kit *k)
{
    return QtVersionManager::version(qtVersionId(k));
}

void QtKitAspect::setQtVersion(Kit *k, const QtVersion *v)
{
    if (!v)
        setQtVersionId(k, -1);
    else
        setQtVersionId(k, v->uniqueId());
}

/*!
 * Helper function that prepends the directory containing the C++ toolchain and Qt
 * binaries to PATH. This is used to in build configurations targeting broken build
 * systems to provide hints about which binaries to use.
 */

void QtKitAspect::addHostBinariesToPath(const Kit *k, Environment &env)
{
    if (const Toolchain *tc = ToolchainKitAspect::cxxToolchain(k))
        env.prependOrSetPath(tc->compilerCommand().parentDir());

    if (const QtVersion *qt = qtVersion(k))
        env.prependOrSetPath(qt->hostBinPath());
}

void QtKitAspectFactory::qtVersionsChanged(const QList<int> &addedIds,
                                           const QList<int> &removedIds,
                                           const QList<int> &changedIds)
{
    Q_UNUSED(addedIds)
    Q_UNUSED(removedIds)
    for (Kit *k : KitManager::kits()) {
        if (changedIds.contains(QtKitAspect::qtVersionId(k))) {
            k->validate(); // Qt version may have become (in)valid
            notifyAboutUpdate(k);
        }
    }
}

void QtKitAspectFactory::onKitsLoaded()
{
    for (Kit *k : KitManager::kits())
        fix(k);

    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &QtKitAspectFactory::qtVersionsChanged);
}

Kit::Predicate QtKitAspect::platformPredicate(Id platform)
{
    return [platform](const Kit *kit) -> bool {
        QtVersion *version = QtKitAspect::qtVersion(kit);
        return version && version->targetDeviceTypes().contains(platform);
    };
}

Kit::Predicate QtKitAspect::qtVersionPredicate(const QSet<Id> &required,
                                               const QVersionNumber &min,
                                               const QVersionNumber &max)
{
    return [required, min, max](const Kit *kit) -> bool {
        QtVersion *version = QtKitAspect::qtVersion(kit);
        if (!version)
            return false;
        const QVersionNumber current = version->qtVersion();
        if (min.majorVersion() > -1 && current < min)
            return false;
        if (max.majorVersion() > -1 && current > max)
            return false;
        return version->features().contains(required);
    };
}

QSet<Id> QtKitAspectFactory::supportedPlatforms(const Kit *k) const
{
    QtVersion *version = QtKitAspect::qtVersion(k);
    return version ? version->targetDeviceTypes() : QSet<Id>();
}

QSet<Id> QtKitAspectFactory::availableFeatures(const Kit *k) const
{
    QtVersion *version = QtKitAspect::qtVersion(k);
    return version ? version->features() : QSet<Id>();
}

int QtKitAspectFactory::weight(const Kit *k) const
{
    const QtVersion * const qt = QtKitAspect::qtVersion(k);
    if (!qt)
        return 0;
    if (!qt->targetDeviceTypes().contains(RunDeviceTypeKitAspect::deviceTypeId(k)))
        return 0;
    const Abi tcAbi = ToolchainKitAspect::targetAbi(k);
    if (qt->qtAbis().contains(tcAbi))
        return 2;
    return Utils::contains(qt->qtAbis(), [&tcAbi](const Abi &qtAbi) {
        return qtAbi.isCompatibleWith(tcAbi); }) ? 1 : 0;
}

} // namespace QtSupport
