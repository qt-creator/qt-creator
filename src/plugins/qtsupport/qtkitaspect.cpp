// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtkitaspect.h"

#include "qtoptionspage.h"
#include "qtparser.h"
#include "qtsupportconstants.h"
#include "qtsupporttr.h"
#include "qttestparser.h"
#include "qtversionfactory.h"
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
#include <utils/async.h>
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
    QString moduleForHeader(const Kit *k, const QString &className) const override;

    void qtVersionsChanged(const QList<int> &addedIds,
                           const QList<int> &removedIds,
                           const QList<int> &changedIds);
    void onKitsLoaded() override;

    std::optional<Tasking::ExecutableItem> autoDetect(
        Kit *kit,
        const Utils::FilePaths &searchPaths,
        const QString &detectionSource,
        const LogCallback &logCallback) const override;

    std::optional<Tasking::ExecutableItem> removeAutoDetected(
        const QString &detectionSource, const LogCallback &logCallback) const override;

    void listAutoDetected(
        const QString &detectionSource, const LogCallback &logCallback) const override;

    Utils::Result<Tasking::ExecutableItem> createAspectFromJson(
        const QString &detectionSource,
        const FilePath &rootPath,
        Kit *kit,
        const QJsonValue &json,
        const LogCallback &logCallback) const override;
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

    QtVersion *const qtFromPath = QtVersionManager::version(
        [](const QtVersion *v) { return v->detectionSource().id == "PATH"; });
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
        QtVersion *v = QtVersionManager::version(
            [source](const QtVersion *v) { return v->detectionSource().id == source; });
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

std::optional<Tasking::ExecutableItem> QtKitAspectFactory::autoDetect(
    Kit *kit,
    const FilePaths &searchPaths,
    const QString &detectionSource,
    const LogCallback &logCallback) const
{
    const auto searchQtse = [searchPaths, detectionSource](Async<QtVersion *> &async) {
        async.setConcurrentCallData(
            [detectionSource](QPromise<QtVersion *> &promise, FilePaths searchPaths) {
                QList<QtVersion *> foundQtVersions;
                const auto handleQmake =
                    [&detectionSource, &foundQtVersions, &promise](const FilePath &qmake) {
                        QString error;
                        QtVersion *qtVersion = QtVersionFactory::createQtVersionFromQMakePath(
                            qmake, true, detectionSource, &error);

                        if (qtVersion && qtVersion->isValid()) {
                            // Trigger loading the version data
                            const Utils::FilePath binPath = qtVersion->binPath();

                            const bool alreadyFound
                                = Utils::anyOf(foundQtVersions, [qtVersion](QtVersion *other) {
                                      return qtVersion->mkspecPath() == other->mkspecPath();
                                  });
                            if (!alreadyFound) {
                                foundQtVersions.append(qtVersion);
                                promise.addResult(qtVersion);
                            }
                        }
                        return IterationPolicy::Continue;
                    };

                const QStringList candidates
                    = {"qmake6", "qmake-qt6", "qmake-qt5", "qmake", "qtpaths6", "qtpaths"};
                for (const FilePath &searchPath : searchPaths) {
                    searchPath.iterateDirectory(
                        handleQmake,
                        {candidates, QDir::Files | QDir::Executable, QDirIterator::Subdirectories});
                }
            },
            searchPaths);
    };

    const auto qtDetectionDone = [kit, logCallback](const Async<QtVersion *> &async) {
        const auto versions = async.results();

        for (QtVersion *version : versions) {
            logCallback(Tr::tr("Found Qt version: %1").arg(version->displayName()));
            QtVersionManager::addVersion(version);
            QtKitAspect::setQtVersion(kit, version);
        }
    };

    return AsyncTask<QtVersion *>(searchQtse, qtDetectionDone);
}

std::optional<Tasking::ExecutableItem> QtKitAspectFactory::removeAutoDetected(
    const QString &detectionSource, const LogCallback &logCallback) const
{
    return Tasking::Sync([detectionSource, logCallback]() {
        const auto versions = QtVersionManager::versions([detectionSource](const QtVersion *qt) {
            return qt->detectionSource().id == detectionSource;
        });

        for (QtVersion *version : versions) {
            logCallback(Tr::tr("Removing Qt: %1").arg(version->displayName()));
            QtVersionManager::removeVersion(version);
        }
    });
}

void QtKitAspectFactory::listAutoDetected(
    const QString &detectionSource, const LogCallback &logCallback) const
{
    for (const QtVersion *qt : QtVersionManager::versions()) {
        if (qt->detectionSource().id == detectionSource)
            logCallback(Tr::tr("Qt: %1").arg(qt->displayName()));
    }
}

Utils::Result<Tasking::ExecutableItem> QtKitAspectFactory::createAspectFromJson(
    const QString &detectionSource,
    const FilePath &rootPath,
    Kit *kit,
    const QJsonValue &json,
    const LogCallback &logCallback) const
{
    using ResultType = Result<QtVersion *>;

    if (!json.isString())
        return ResultError(Tr::tr("Expected String, got: %1").arg(json.toString()));

    const QString qmakePath = json.toString();

    if (qmakePath.isEmpty())
        return ResultError(Tr::tr("Expected non-empty qmake path."));

    const auto setup =
        [qmakePath, rootPath, detectionSource, logCallback](Async<ResultType> &async) {
            async.setConcurrentCallData(
                [](QPromise<ResultType> &promise,
                   const QString &qmakePath,
                   const QString &detectionSource,
                   const FilePath &rootPath) {
                    QString error;
                    QtVersion *qtVersion = QtVersionFactory::createQtVersionFromQMakePath(
                        rootPath.withNewPath(qmakePath), true, detectionSource, &error);

                    if (!qtVersion) {
                        promise.addResult(ResultError(
                            Tr::tr("Failed to create Qt version from qmake path '%1': %2")
                                .arg(qmakePath, error)));
                        return;
                    }

                    promise.addResult(qtVersion);
                },
                qmakePath,
                detectionSource,
                rootPath);
        };

    const auto onDone = [logCallback, kit, json](const Async<ResultType> &async) {
        const ResultType result = async.result();
        if (!result) {
            logCallback(result.error());
            return;
        }

        QtVersion *qtVersion = result.value();
        if (!qtVersion->isValid()) {
            logCallback(Tr::tr("Qt version '%1' is not valid.").arg(qtVersion->displayName()));
            return;
        }

        logCallback(Tr::tr("Adding Qt version: %1").arg(qtVersion->displayName()));
        QtVersionManager::addVersion(qtVersion);
        QtKitAspect::setQtVersion(kit, qtVersion);
    };

    return AsyncTask<ResultType>(setup, onDone);
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

QString QtKitAspectFactory::moduleForHeader(const Kit *k, const QString &headerFileName) const
{
    if (const QtVersion * const v = QtKitAspect::qtVersion(k))
        return v->moduleForHeader(headerFileName);
    return {};
}

} // namespace QtSupport
