// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectimporter.h"

#include "buildinfo.h"
#include "kit.h"
#include "kitaspects.h"
#include "kitmanager.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "target.h"
#include "toolchain.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

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

    return Utils::Id::fromString(prefix + idStr);
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
    useTemporaryKitAspect(ToolChainKitAspect::id(),
                               [this](Kit *k, const QVariantList &vl) { cleanupTemporaryToolChains(k, vl); },
                               [this](Kit *k, const QVariantList &vl) { persistTemporaryToolChains(k, vl); });
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

    const QLoggingCategory log("qtc.projectexplorer.import", QtWarningMsg);
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
    qCDebug(log) << "Examining directory" << absoluteImportPath.toString();
    QString warningMessage;
    QList<void *> dataList = examineDirectory(absoluteImportPath, &warningMessage);
    if (dataList.isEmpty()) {
        qCDebug(log) << "Nothing to import found in" << absoluteImportPath.toString();
        handleFailure();
        return result;
    }
    if (!warningMessage.isEmpty()) {
        qCDebug(log) << "Warning when examining" << absoluteImportPath.toString();
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
            const QList<BuildInfo> infoList = buildInfoList(data);
            if (infoList.isEmpty()) {
                qCDebug(log) << "No build infos for kit" << k->displayName();
                continue;
            }

            auto factory = BuildConfigurationFactory::find(k, projectFilePath());
            for (BuildInfo i : infoList) {
                i.kitId = k->id();
                i.factory = factory;
                if (!result.contains(i))
                    result += i;
            }
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
        if (DeviceTypeKitAspect::deviceTypeId(t->kit()) == Constants::DESKTOP_DEVICE_TYPE) {
            activeTarget = t;
            pickedFallback = true;
        }
    }
    return activeTarget;
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
    projects.append(m_projectPath.toString()); // note: There can be more than one instance of the project added!
    k->setValueSilently(TEMPORARY_OF_PROJECTS, projects);
}

void ProjectImporter::removeProject(Kit *k) const
{
    QTC_ASSERT(k, return);
    if (!k->hasValue(KIT_IS_TEMPORARY))
        return;

    UpdateGuard guard(*this);
    QStringList projects = k->value(TEMPORARY_OF_PROJECTS, QStringList()).toStringList();
    projects.removeOne(m_projectPath.toString());

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

static ToolChain *toolChainFromVariant(const QVariant &v)
{
    const QByteArray tcId = v.toByteArray();
    return ToolChainManager::findToolChain(tcId);
}

void ProjectImporter::cleanupTemporaryToolChains(Kit *k, const QVariantList &vl)
{
    for (const QVariant &v : vl) {
        ToolChain *tc = toolChainFromVariant(v);
        QTC_ASSERT(tc, continue);
        ToolChainManager::deregisterToolChain(tc);
        ToolChainKitAspect::setToolChain(k, nullptr);
    }
}

void ProjectImporter::persistTemporaryToolChains(Kit *k, const QVariantList &vl)
{
    for (const QVariant &v : vl) {
        ToolChain *tmpTc = toolChainFromVariant(v);
        QTC_ASSERT(tmpTc, continue);
        ToolChain *actualTc = ToolChainKitAspect::toolChain(k, tmpTc->language());
        if (tmpTc && actualTc != tmpTc)
            ToolChainManager::deregisterToolChain(tmpTc);
    }
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

static ProjectImporter::ToolChainData createToolChains(const ToolChainDescription &tcd)
{
    ProjectImporter::ToolChainData data;

    for (ToolChainFactory *factory : ToolChainFactory::allToolChainFactories()) {
        data.tcs = factory->detectForImport(tcd);
        if (data.tcs.isEmpty())
            continue;

        for (ToolChain *tc : std::as_const(data.tcs))
            ToolChainManager::registerToolChain(tc);

        data.areTemporary = true;
        break;
    }

    return data;
}

ProjectImporter::ToolChainData
ProjectImporter::findOrCreateToolChains(const ToolChainDescription &tcd) const
{
    ToolChainData result;
    result.tcs = ToolChainManager::toolchains([&tcd](const ToolChain *tc) {
        return tc->language() == tcd.language && tc->matchesCompilerCommand(tcd.compilerPath);
    });
    for (const ToolChain *tc : std::as_const(result.tcs)) {
        const QByteArray tcId = tc->id();
        result.areTemporary = result.areTemporary ? true : hasKitWithTemporaryData(ToolChainKitAspect::id(), tcId);
    }
    if (!result.tcs.isEmpty())
        return result;

    // Create a new toolchain:
    UpdateGuard guard(*this);
    return createToolChains(tcd);
}

} // namespace ProjectExplorer
