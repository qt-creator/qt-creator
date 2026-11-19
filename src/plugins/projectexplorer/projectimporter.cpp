// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectimporter.h"

#include "buildinfo.h"
#include "devicesupport/devicekitaspects.h"
#include "kit.h"
#include "kitmanager.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "target.h"
#include "toolchain.h"
#include "toolchainkitaspect.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPushButton>
#include <QString>

namespace ProjectExplorer {

static const Utils::Id KIT_IS_TEMPORARY("PE.tmp.isTemporary");
static const Utils::Id KIT_TEMPORARY_NAME("PE.tmp.Name");
static const Utils::Id KIT_FINAL_NAME("PE.tmp.FinalName");
static const Utils::Id TEMPORARY_OF_PROJECTS("PE.tmp.ForProjects");

static Utils::Id fullId(Utils::Id id)
{
    const QString prefix = "PE.tmp.";

    const QString idStr = id.toString();
    QTC_ASSERT(!idStr.startsWith(prefix), return Utils::Id::fromString(idStr));

    return Utils::Id::fromString(prefix).withSuffix(idStr);
}

static bool hasOtherUsers(Utils::Id id, const QVariant &v, Kit *k)
{
    return Utils::contains(KitManager::kits(), [id, v, k](Kit *in) -> bool {
        if (in == k)
            return false;
        const QVariantList tmp = in->value(id).toList();
        return tmp.contains(v);
    });
}

ProjectImporter::ProjectImporter(const Utils::FilePath &path) : m_projectPath(path)
{
    useTemporaryKitAspect(ToolchainKitAspect::id(),
                               [this](Kit *k, const QVariantList &vl) { cleanupTemporaryToolchains(k, vl); },
                               [this](Kit *k, const QVariantList &vl) { persistTemporaryToolchains(k, vl); });
}

ProjectImporter::~ProjectImporter()
{
    const QList<Kit *> kits = KitManager::kits();
    for (Kit *k : kits)
        removeProject(k);
}

const QList<BuildInfo> ProjectImporter::import(const Utils::FilePath &importPath, bool silent)
{
    QList<BuildInfo> result;

    static const QLoggingCategory log("qtc.projectexplorer.import", QtWarningMsg);
    qCDebug(log) << "ProjectImporter::import" << importPath << silent;

    QFileInfo fi = importPath.toFileInfo();
    if (!fi.exists() && !fi.isDir()) {
        qCDebug(log) << "**doesn't exist";
        return result;
    }

    const Utils::FilePath absoluteImportPath = Utils::FilePath::fromString(fi.absoluteFilePath());

    const auto handleFailure = [this, importPath, silent] {
        if (silent)
            return;
        QMessageBox::critical(Core::ICore::dialogParent(),
                              Tr::tr("No Build Found"),
                              Tr::tr("No build found in %1 matching project %2.")
                                  .arg(importPath.toUserOutput(), projectFilePath().toUserOutput()));
    };
    qCDebug(log) << "Examining directory" << absoluteImportPath.toUserOutput();
    QString warningMessage;
    QList<void *> dataList = examineDirectory(absoluteImportPath, &warningMessage);
    if (dataList.isEmpty()) {
        qCDebug(log) << "Nothing to import found in" << absoluteImportPath.toUserOutput();
        handleFailure();
        return result;
    }
    if (!warningMessage.isEmpty()) {
        qCDebug(log) << "Warning when examining" << absoluteImportPath.toUserOutput();
        // we should ask user before importing
        if (silent)
            return result;
        QMessageBox dialog(Core::ICore::dialogParent());
        dialog.setWindowTitle(Tr::tr("Import Warning"));
        dialog.setText(warningMessage);
        dialog.setIcon(QMessageBox::Warning);
        QPushButton *acceptButton = dialog.addButton(Tr::tr("Import Build"), QMessageBox::AcceptRole);
        dialog.addButton(QMessageBox::Cancel);
        dialog.exec();
        if (dialog.clickedButton() != acceptButton)
            return result;
    }

    qCDebug(log) << "Looking for kits";
    for (void *data : std::as_const(dataList)) {
        QTC_ASSERT(data, continue);
        QList<Kit *> kitList;
        const QList<Kit *> tmp
                = Utils::filtered(KitManager::kits(), [this, data](Kit *k) { return matchKit(data, k); });
        if (tmp.isEmpty()) {
            Kit *k = createKit(data);
            if (k)
                kitList.append(k);
            qCDebug(log) << "  no matching kit found, temporary kit created.";
        } else {
            kitList += tmp;
            qCDebug(log) << "  " << tmp.count() << "matching kits found.";
        }

        for (Kit *k : std::as_const(kitList)) {
            qCDebug(log) << "Creating buildinfos for kit" << k->displayName();
            BuildInfo info = buildInfo(data);

            auto factory = BuildConfigurationFactory::find(k, projectFilePath());
            if (!factory) {
                qCDebug(log) << "No factory for kit" << k->displayName();
                continue;
            }
            const QVariantMap extraInfo = info.extraInfo.toMap();
            if (!extraInfo["hideImportedSuffix"].toBool())
                info.displayName = Tr::tr("%1 (imported)").arg(info.displayName);
            info.kitId = k->id();
            info.factory = factory;
            if (!result.contains(info))
                result += info;
        }
    }

    for (void *dd : std::as_const(dataList))
        deleteDirectoryData(dd);
    dataList.clear();

    if (result.isEmpty())
        handleFailure();

    return result;
}

Target *ProjectImporter::preferredTarget(const QList<Target *> &possibleTargets)
{
    // Select active target
    // a) The default target
    // c) Desktop target
    // d) the first target
    Target *activeTarget = nullptr;
    if (possibleTargets.isEmpty())
        return activeTarget;

    activeTarget = possibleTargets.at(0);
    bool pickedFallback = false;
    for (Target *t : possibleTargets) {
        if (t->kit() == KitManager::defaultKit())
            return t;
        if (pickedFallback)
            continue;
        if (RunDeviceTypeKitAspect::deviceTypeId(t->kit()) == Constants::DESKTOP_DEVICE_TYPE) {
            activeTarget = t;
            pickedFallback = true;
        }
    }
    return activeTarget;
}

// The kits the project brings, e.g. those of its CMake presets, come first
QList<Kit *> ProjectImporter::sortedKits() const
{
    QList<Kit *> kits = KitManager::sortedKits();
    std::stable_partition(kits.begin(), kits.end(), [this](const Kit *k) {
        return isPreferredKit(k);
    });
    return kits;
}

void ProjectImporter::markKitAsTemporary(Kit *k) const
{
    QTC_ASSERT(!k->hasValue(KIT_IS_TEMPORARY), return);

    UpdateGuard guard(*this);

    const QString name = k->displayName();
    k->setUnexpandedDisplayName(Tr::tr("%1 - temporary").arg(name));

    k->setValue(KIT_TEMPORARY_NAME, k->displayName());
    k->setValue(KIT_FINAL_NAME, name);
    k->setValue(KIT_IS_TEMPORARY, true);
}

void ProjectImporter::makePersistent(Kit *k) const
{
    QTC_ASSERT(k, return);
    if (!k->hasValue(KIT_IS_TEMPORARY))
        return;

    UpdateGuard guard(*this);

    KitGuard kitGuard(k);
    k->removeKey(KIT_IS_TEMPORARY);
    k->removeKey(TEMPORARY_OF_PROJECTS);
    const QString tempName = k->value(KIT_TEMPORARY_NAME).toString();
    if (!tempName.isNull() && k->displayName() == tempName)
        k->setUnexpandedDisplayName(k->value(KIT_FINAL_NAME).toString());
    k->removeKey(KIT_TEMPORARY_NAME);
    k->removeKey(KIT_FINAL_NAME);

    for (const TemporaryInformationHandler &tih : std::as_const(m_temporaryHandlers)) {
        const Utils::Id fid = fullId(tih.id);
        const QVariantList temporaryValues = k->value(fid).toList();

        // Mark permanent in all other kits:
        const QList<Kit *> kits = KitManager::kits();
        for (Kit *ok : kits) {
            if (ok == k || !ok->hasValue(fid))
                continue;
            const QVariantList otherTemporaryValues
                    = Utils::filtered(ok->value(fid).toList(), [&temporaryValues](const QVariant &v) {
                return !temporaryValues.contains(v);
            });
            ok->setValueSilently(fid, otherTemporaryValues);
        }

        // persist:
        tih.persist(k, temporaryValues);
        k->removeKeySilently(fid);
    }
}

void ProjectImporter::cleanupKit(Kit *k) const
{
    QTC_ASSERT(k, return);
    for (const TemporaryInformationHandler &tih : std::as_const(m_temporaryHandlers)) {
        const Utils::Id fid = fullId(tih.id);
        const QVariantList temporaryValues
                = Utils::filtered(k->value(fid).toList(), [fid, k](const QVariant &v) {
           return !hasOtherUsers(fid, v, k);
        });
        tih.cleanup(k, temporaryValues);
        k->removeKeySilently(fid);
    }

    // remove keys to manage temporary state of kit:
    k->removeKeySilently(KIT_IS_TEMPORARY);
    k->removeKeySilently(TEMPORARY_OF_PROJECTS);
    k->removeKeySilently(KIT_FINAL_NAME);
    k->removeKeySilently(KIT_TEMPORARY_NAME);
}

void ProjectImporter::addProject(Kit *k) const
{
    QTC_ASSERT(k, return);
    if (!k->hasValue(KIT_IS_TEMPORARY))
        return;

    UpdateGuard guard(*this);
    QStringList projects = k->value(TEMPORARY_OF_PROJECTS, QStringList()).toStringList();
    projects.append(m_projectPath.toUrlishString()); // note: There can be more than one instance of the project added!
    k->setValueSilently(TEMPORARY_OF_PROJECTS, projects);
}

void ProjectImporter::removeProject(Kit *k) const
{
    QTC_ASSERT(k, return);
    if (!k->hasValue(KIT_IS_TEMPORARY))
        return;

    UpdateGuard guard(*this);
    QStringList projects = k->value(TEMPORARY_OF_PROJECTS, QStringList()).toStringList();
    projects.removeOne(m_projectPath.toUrlishString());

    if (projects.isEmpty()) {
        cleanupKit(k);
        KitManager::deregisterKit(k);
    } else {
        k->setValueSilently(TEMPORARY_OF_PROJECTS, projects);
    }
}

bool ProjectImporter::isTemporaryKit(Kit *k) const
{
    QTC_ASSERT(k, return false);
    return k->hasValue(KIT_IS_TEMPORARY);
}

Kit *ProjectImporter::createTemporaryKit(const KitSetupFunction &setup) const
{
    UpdateGuard guard(*this);
    const auto init = [&](Kit *k) {
        KitGuard kitGuard(k);
        k->setUnexpandedDisplayName(Tr::tr("Imported Kit"));
        k->setup();
        setup(k);
        k->fix();
        markKitAsTemporary(k);
        addProject(k);
    }; // ~KitGuard, sending kitUpdated
    return KitManager::registerKit(init); // potentially adds kits to other targetsetuppages
}

bool ProjectImporter::findTemporaryHandler(Utils::Id id) const
{
    return Utils::contains(m_temporaryHandlers, [id](const TemporaryInformationHandler &ch) { return ch.id == id; });
}

static Toolchain *toolChainFromVariant(const QVariant &v)
{
    return ToolchainManager::findToolchain(Utils::Id::fromSetting(v));
}

void ProjectImporter::cleanupTemporaryToolchains(Kit *k, const QVariantList &vl)
{
    // Kits that are removed together can share a temporary toolchain: none of them sees the
    // others as its users, so the first one deregisters it.
    const Toolchains toolchains = Utils::filtered(Utils::transform(vl, toolChainFromVariant),
                                                  [](const Toolchain *tc) { return tc != nullptr; });
    for (Toolchain * const tc : toolchains)
        ToolchainKitAspect::clearToolchain(k, tc->language());
    ToolchainManager::deregisterToolchains(toolchains);
}

void ProjectImporter::persistTemporaryToolchains(Kit *k, const QVariantList &vl)
{
    Toolchains toDeregister;
    for (const QVariant &v : vl) {
        Toolchain *tmpTc = toolChainFromVariant(v);
        QTC_ASSERT(tmpTc, continue);
        Toolchain *actualTc = ToolchainKitAspect::toolchain(k, tmpTc->language());
        if (tmpTc && actualTc != tmpTc)
            toDeregister << tmpTc;
    }
    ToolchainManager::deregisterToolchains(toDeregister);
}

void ProjectImporter::useTemporaryKitAspect(Utils::Id id,
                                                 ProjectImporter::CleanupFunction cleanup,
                                                 ProjectImporter::PersistFunction persist)
{
    QTC_ASSERT(!findTemporaryHandler(id), return);
    m_temporaryHandlers.append({id, cleanup, persist});
}

void ProjectImporter::addTemporaryData(Utils::Id id, const QVariant &cleanupData, Kit *k) const
{
    QTC_ASSERT(k, return);
    QTC_ASSERT(findTemporaryHandler(id), return);
    const Utils::Id fid = fullId(id);

    KitGuard guard(k);
    QVariantList tmp = k->value(fid).toList();
    QTC_ASSERT(!tmp.contains(cleanupData), return);
    tmp.append(cleanupData);
    k->setValue(fid, tmp);
}

bool ProjectImporter::hasKitWithTemporaryData(Utils::Id id, const QVariant &data) const
{
    Utils::Id fid = fullId(id);
    return Utils::contains(KitManager::kits(), [data, fid](Kit *k) {
        return k->value(fid).toList().contains(data);
    });
}

static ProjectImporter::ToolchainData createToolChains(const ToolchainDescription &tcd)
{
    ProjectImporter::ToolchainData data;

    for (ToolchainFactory *factory : ToolchainFactory::allToolchainFactories()) {
        data.tcs = factory->detectForImport(tcd);
        if (data.tcs.isEmpty())
            continue;
        ToolchainManager::registerToolchains(data.tcs);
        data.areTemporary = true;
        break;
    }

    return data;
}

ProjectImporter::ToolchainData
ProjectImporter::findOrCreateToolchains(const ToolchainDescription &tcd) const
{
    ToolchainData result;
    result.tcs = ToolchainManager::toolchains([&tcd](const Toolchain *tc) {
        return tc->language() == tcd.language && tc->matchesCompilerCommand(tcd.compilerPath);
    });
    for (const Toolchain *tc : std::as_const(result.tcs)) {
        result.areTemporary = result.areTemporary
            ? true : hasKitWithTemporaryData(ToolchainKitAspect::id(), tc->id().toSetting());
    }
    if (!result.tcs.isEmpty())
        return result;

    // Create a new toolchain:
    UpdateGuard guard(*this);
    return createToolChains(tcd);
}

} // namespace ProjectExplorer

#ifdef WITH_TESTS

#include "toolchainconfigwidget.h"

#include <QScopeGuard>
#include <QTest>

namespace ProjectExplorer::Internal {

const char ImporterTestToolchainType[] = "ProjectExplorer.ToolChain.ImporterTest";

class ImporterTestToolchain final : public Toolchain
{
public:
    ImporterTestToolchain()
        : Toolchain(ImporterTestToolchainType)
    {
        setLanguage(Constants::CXX_LANGUAGE_ID);
        setTypeDisplayName("Importer Test Toolchain");
        setTargetAbiNoSignal(Abi::hostAbi());
    }

    MacroInspectionRunner createMacroInspectionRunner() const final { return {}; }
    Utils::LanguageExtensions languageExtensions(const QStringList &) const final
    { return Utils::LanguageExtension::None; }
    Utils::WarningFlags warningFlags(const QStringList &) const final
    { return Utils::WarningFlag::NoWarnings; }
    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(const Utils::Environment &) const final
    { return {}; }
    void addToEnvironment(Utils::Environment &) const final {}
    Utils::FilePath makeCommand(const Utils::Environment &) const final { return "make"; }
    QList<Utils::OutputLineParser *> createOutputParsers() const final { return {}; }
};

class ImporterTestToolchainFactory final : public ToolchainFactory
{
public:
    ImporterTestToolchainFactory()
    {
        setSupportedToolchainType(ImporterTestToolchainType);
        setSupportedLanguages({Constants::CXX_LANGUAGE_ID});
        setToolchainConstructor([] { return new ImporterTestToolchain; });
    }

    std::unique_ptr<ToolchainConfigWidget> createConfigurationWidget(
        const ToolchainBundle &) const final
    {
        return nullptr;
    }
};

class TestImporter final : public ProjectImporter
{
public:
    TestImporter()
        : ProjectImporter(Utils::FilePath::fromString("/importer-test/project.txt"))
    {}

    void addTemporaryToolchain(Toolchain *tc, Kit *k) const
    {
        addTemporaryData(ToolchainKitAspect::id(), tc->id().toSetting(), k);
    }

    Utils::FilePaths importCandidates() final { return {}; }

private:
    QList<void *> examineDirectory(const Utils::FilePath &, QString *) const final { return {}; }
    bool matchKit(void *, const Kit *) const final { return false; }
    Kit *createKit(void *) const final { return nullptr; }
    BuildInfo buildInfo(void *) const final { return {}; }
    void deleteDirectoryData(void *) const final {}
};

class ProjectImporterTest final : public QObject
{
    Q_OBJECT

private slots:
    void testCleanupOfKitsRemovedTogether()
    {
        const ImporterTestToolchainFactory factory;
        const TestImporter importer;

        auto tc = new ImporterTestToolchain;
        QVERIFY(ToolchainManager::registerToolchains({tc}).isEmpty());
        const Utils::Id tcId = tc->id();

        // Two kits with the same temporary toolchain, as two CMake presets with the same
        // compiler have
        QList<Kit *> kits;
        for (const QString &name : {QString("first"), QString("second")}) {
            Kit * const k = KitManager::registerKit(
                [name](Kit *k) { k->setUnexpandedDisplayName(name); });
            QVERIFY(k);
            ToolchainKitAspect::setToolchain(k, tc);
            importer.addTemporaryToolchain(tc, k);
            kits << k;
        }
        const auto cleanup = qScopeGuard([&kits, tcId] {
            KitManager::deregisterKits(Utils::filtered(KitManager::kits(), [&kits](Kit *k) {
                return kits.contains(k);
            }));
            if (Toolchain * const tc = ToolchainManager::findToolchain(tcId))
                ToolchainManager::deregisterToolchains({tc});
        });

        // What the Configure Project page does when the CMake presets are reloaded: the kits
        // are removed at once, and each one is cleaned up after
        QObject context;
        connect(KitManager::instance(), &KitManager::kitRemoved, &context, [&importer](Kit *k) {
            importer.cleanupKit(k);
        });
        KitManager::deregisterKits(kits);
        QVERIFY2(!ToolchainManager::findToolchain(tcId),
                 "The toolchain that only the removed kits used is removed with them.");
    }
};

QObject *createProjectImporterTest()
{
    return new ProjectImporterTest;
}

} // namespace ProjectExplorer::Internal

#include "projectimporter.moc"

#endif // WITH_TESTS
