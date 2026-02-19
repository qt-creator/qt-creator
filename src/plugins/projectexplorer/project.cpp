// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "project.h"

#include "buildconfiguration.h"
#include "buildinfo.h"
#include "buildmanager.h"
#include "buildsystem.h"
#include "deployconfiguration.h"
#include "devicesupport/devicekitaspects.h"
#include "devicesupport/idevice.h"
#include "editorconfiguration.h"
#include "environmentaspect.h"
#include "kit.h"
#include "kitmanager.h"
#include "msvctoolchain.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projectnodes.h"
#include "projecttree.h"
#include "runconfiguration.h"
#include "runconfigurationaspects.h"
#include "target.h"
#include "taskhub.h"
#include "toolchainkitaspect.h"
#include "toolchainmanager.h"
#include "userfileaccessor.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <texteditor/codestyleeditor.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QFileDialog>
#include <QHash>

#include <queue>

#ifdef WITH_TESTS
#include <coreplugin/editormanager/editormanager.h>
#include <utils/temporarydirectory.h>

#include <QEventLoop>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#endif

using namespace Utils;
using namespace Core;

namespace PE = ProjectExplorer;

namespace ProjectExplorer {

/*!
    \class ProjectExplorer::Project

    \brief The Project class implements a project node in the project explorer.
*/

/*!
   \fn void ProjectExplorer::Project::environmentChanged()

   A convenience signal emitted if activeBuildConfiguration emits
   environmentChanged or if the active build configuration changes
   (including due to the active target changing).
*/

/*!
   \fn void ProjectExplorer::Project::buildConfigurationEnabledChanged()

   A convenience signal emitted if activeBuildConfiguration emits
   isEnabledChanged() or if the active build configuration changes
   (including due to the active target changing).
*/

const char ACTIVE_TARGET_KEY[] = "ProjectExplorer.Project.ActiveTarget";
const char TARGET_KEY_PREFIX[] = "ProjectExplorer.Project.Target.";
const char TARGET_COUNT_KEY[] = "ProjectExplorer.Project.TargetCount";
const char EDITOR_SETTINGS_KEY[] = "ProjectExplorer.Project.EditorSettings";
const char PLUGIN_SETTINGS_KEY[] = "ProjectExplorer.Project.PluginSettings";
const char PROJECT_ENV_KEY[] = "ProjectExplorer.Project.Environment";

static bool isListedFileNode(const Node *node)
{
    return node->asContainerNode() || node->listInProject();
}

static bool nodeLessThan(const Node *n1, const Node *n2)
{
    return n1->filePath() < n2->filePath();
}

const Project::NodeMatcher Project::AllFiles = [](const Node *node) {
    return isListedFileNode(node);
};

const Project::NodeMatcher Project::SourceFiles = [](const Node *node) {
    return isListedFileNode(node) && !node->isGenerated();
};

const Project::NodeMatcher Project::GeneratedFiles = [](const Node *node) {
    return isListedFileNode(node) && node->isGenerated();
};

const Project::NodeMatcher Project::HiddenRccFolders = [](const Node *node) {
    return node->isFolderNodeType() && node->filePath().fileName() == ".rcc";
};

// --------------------------------------------------------------------
// ProjectDocument:
// --------------------------------------------------------------------

class ProjectDocument : public IDocument
{
public:
    ProjectDocument(const QString &mimeType, const FilePath &fileName, Project *project)
        : m_project(project)
    {
        QTC_CHECK(project);

        setFilePath(fileName);
        setMimeType(mimeType);
    }

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const final
    {
        Q_UNUSED(state)
        Q_UNUSED(type)
        return BehaviorSilent;
    }

    Result<> reload(ReloadFlag flag, ChangeType type) final
    {
        Q_UNUSED(flag)
        Q_UNUSED(type)

        emit m_project->projectFileIsDirty(filePath());
        return ResultOk;
    }

private:
    Project *m_project;
};

// -------------------------------------------------------------------------
// Project
// -------------------------------------------------------------------------
class ProjectPrivate
{
public:
    ProjectPrivate(Project *q) : q(q) {}
    ~ProjectPrivate();

    Project * const q;
    Id m_id;
    bool m_needsInitialExpansion = false;
    bool m_canBuildProducts = false;
    bool m_hasMakeInstallEquivalent = false;
    bool m_supportsBuilding = true;
    bool m_isEditModePreferred = true;
    bool m_shuttingDown = false;

    std::function<BuildSystem *(BuildConfiguration *)> m_buildSystemCreator;

    std::unique_ptr<IDocument> m_document;
    std::vector<std::unique_ptr<IDocument>> m_extraProjectDocuments;
    std::unique_ptr<ProjectNode> m_rootProjectNode;
    std::unique_ptr<ContainerNode> m_containerNode;
    std::vector<std::unique_ptr<Target>> m_targets;
    Target *m_activeTarget = nullptr;
    EditorConfiguration m_editorConfiguration;
    Context m_projectLanguages;
    Store m_pluginSettings;
    std::unique_ptr<Internal::UserFileAccessor> m_accessor;
    QHash<Id, QPair<QString, std::function<void()>>> m_generators;
    std::function<Tasks(const Kit *)> m_issuesGenerator;
    Tasks m_tasks;
    PerProjectProjectExplorerSettings m_projectExplorerSettings{q};

    QString m_buildSystemName;
    QString m_displayName;

    MacroExpander m_macroExpander;
    FilePath m_rootProjectDirectory;
    mutable QList<const Node *> m_sortedNodeList;

    Store m_extraData;

    QList<Store> m_vanishedTargets;
};

ProjectPrivate::~ProjectPrivate()
{
    for (const Task &t : std::as_const(m_tasks))
        TaskHub::removeTask(t);

    // Make sure our root node is null when deleting the actual node
    std::unique_ptr<ProjectNode> oldNode = std::move(m_rootProjectNode);
}

Project::Project(const QString &mimeType, const FilePath &fileName)
    : d(new ProjectPrivate(this))
{
    d->m_document = std::make_unique<ProjectDocument>(mimeType, fileName, this);
    DocumentManager::addDocument(d->m_document.get());

    d->m_macroExpander.setDisplayName(::PE::Tr::tr("Project"));
    d->m_macroExpander.registerVariable("Project:Name", ::PE::Tr::tr("Project Name"), [this] {
        return displayName();
    });

    // Only set up containernode after d is set so that it will find the project directory!
    d->m_containerNode = std::make_unique<ContainerNode>(this);

    KitManager *km = KitManager::instance();
    connect(km, &KitManager::kitUpdated, this, &Project::handleKitUpdated);
    connect(km, &KitManager::kitRemoved, this, &Project::handleKitRemoval);
    ProjectExplorerSettings::registerCallback(
        this, &ProjectExplorerSettings::syncRunConfigurations, [this] {
            syncRunConfigurations(false);
        });
}

Project::~Project()
{
    delete d;
}

void Project::handleKitUpdated(Kit *k)
{
    for (const std::unique_ptr<Target> &target : d->m_targets) {
        if (k == target->kit()) {
            target->handleKitUpdated();
            break;
        }
    }
}

void Project::handleKitRemoval(Kit *k)
{
    for (const std::unique_ptr<Target> &target : d->m_targets) {
        if (k == target->kit()) {
            d->m_vanishedTargets.append(target->toMap());
            removeTarget(target.get());
            emit vanishedTargetsChanged();
            break;
        }
    }
}

void Project::syncRunConfigurations(bool force)
{
    const Key key = "RcSync";
    const QVariant projectSyncSetting = namedSettings(key);
    const SyncRunConfigs prevSyncSetting = projectSyncSetting.isValid()
        ? static_cast<SyncRunConfigs>(projectSyncSetting.toInt()) : SyncRunConfigs::Off;
    const SyncRunConfigs curSyncSetting
        = ProjectExplorerSettings::get(this).syncRunConfigurations.value();
    setNamedSettings(key, static_cast<int>(curSyncSetting));

    // Invariant: If the setting has not changed, everything is as it should be.
    if (curSyncSetting == prevSyncSetting && !force)
        return;

    // Nothing to do here: Everything stays as it is for now and will slowly
    // go out of sync over time.
    if (curSyncSetting == SyncRunConfigs::Off)
        return;

    if (curSyncSetting == SyncRunConfigs::SameKit) {

        // Nothing to do here: Everything stays as it is for now and will slowly
        // go out of sync over time.
        if (prevSyncSetting == SyncRunConfigs::All && !force)
            return;
    }

    // Collect all run configurations.
    QList<RunConfiguration *> allRcs;
    for (Target * const t : targets()) {
        for (BuildConfiguration * const bc : t->buildConfigurations())
            allRcs << bc->runConfigurations();
    }

    while (!allRcs.isEmpty()) {
        RunConfiguration * const rc = allRcs.takeFirst();
        const QList<BuildConfiguration *> syncableBcs = rc->syncableBuildConfigurations();

        // At the end of this loop, all build configurations in syncableBcs should
        // have a run configuration with the same uniqueId() as rc, unless the
        // source and target build configurations are incompatible with regards
        // to the run configuration, for instance because the target factory
        // cannot create that type of run configuration.
        for (BuildConfiguration * const bc : std::as_const(syncableBcs)) {

            // First we check whether this build configuration already has a counterpart
            // to our run configuration. In that case, we don't need to make a copy.
            bool needsCloning = true;
            for (RunConfiguration * const otherRc : bc->runConfigurations()) {

                // The run configurations do not refer to the same application.
                if (otherRc->buildKey() != rc->buildKey())
                    continue;

                // These run configs were already linked together in the past, in which
                // case we keep this relationship alive, or they were both auto-created
                // and are still in pristine state, which means they are identical.
                // This also means that the other run configuration does not have to be
                // handled on its own anymore.
                if (otherRc->uniqueId() == rc->uniqueId()
                    && (rc->uniqueId() != rc->buildKey()
                        || (!rc->isCustomized() && !otherRc->isCustomized()))) {
                    const bool removed = allRcs.removeOne(otherRc);
                    QTC_CHECK(removed);
                    if (rc->isCustomized() || otherRc->isCustomized()) {
                        otherRc->cloneFromOther(rc);
                        otherRc->setDisplayName(rc->displayName());
                    }
                    needsCloning = false;
                    break;
                }

                // These run configs are equal, so we assume they should get linked.
                if (rc->equals(otherRc)) {
                    otherRc->setUniqueId(rc->uniqueId());
                    const bool removed = allRcs.removeOne(otherRc);
                    QTC_CHECK(removed);
                    needsCloning = false;
                    break;
                }
            }

            if (needsCloning) {
                if (const auto clone = rc->clone(bc))
                    bc->addRunConfiguration(clone, NameHandling::Keep);
            }
        }
    }
}

PerProjectProjectExplorerSettings &Project::projectExplorerSettings() const
{
    return d->m_projectExplorerSettings;
}

QString Project::buildSystemName() const
{
    return d->m_buildSystemName;
}

QString Project::displayName() const
{
    return d->m_displayName;
}

Id Project::type() const
{
    QTC_CHECK(d->m_id.isValid());
    return d->m_id;
}

void Project::markAsShuttingDown()
{
    d->m_shuttingDown = true;
}

bool Project::isShuttingDown() const
{
    return d->m_shuttingDown;
}

QString Project::mimeType() const
{
    return d->m_document->mimeType();
}

bool Project::canBuildProducts() const
{
    return d->m_canBuildProducts;
}

BuildSystem *Project::createBuildSystem(BuildConfiguration *bc) const
{
    QTC_ASSERT(d->m_buildSystemCreator, return nullptr);
    return d->m_buildSystemCreator(bc);
}

FilePath Project::projectFilePath() const
{
    QTC_ASSERT(d->m_document, return {});
    return d->m_document->filePath();
}

void Project::addTarget(std::unique_ptr<Target> &&t)
{
    auto pointer = t.get();
    QTC_ASSERT(t && !contains(d->m_targets, pointer), return);
    QTC_ASSERT(!target(t->kit()), return);
    Q_ASSERT(t->project() == this);

    // add it
    d->m_targets.emplace_back(std::move(t));
    emit addedTarget(pointer);

    // check activeTarget:
    if (!activeTarget())
        setActiveTarget(pointer, SetActive::Cascade);
}

Target *Project::addTargetForDefaultKit()
{
    return addTargetForKit(KitManager::defaultKit());
}

Target *Project::addTargetForKit(Kit *kit)
{
    if (!kit || target(kit))
        return nullptr;

    auto t = Target::create(this, kit);
    Target *pointer = t.get();

    t->updateDefaultBuildConfigurations();
    QTC_ASSERT(!t->buildConfigurations().isEmpty(), return nullptr);

    addTarget(std::move(t));

    return pointer;
}

void Project::removeTarget(Target *target)
{
    QTC_ASSERT(target, return);
    QTC_ASSERT(contains(d->m_targets, target), return);

    if (BuildManager::isBuilding(target))
        return;

    target->markAsShuttingDown();
    for (BuildConfiguration * const bc : target->buildConfigurations())
        emit ProjectManager::instance()->aboutToRemoveBuildConfiguration(bc);
    emit aboutToRemoveTarget(target);
    auto keep = take(d->m_targets, target);
    if (target == d->m_activeTarget) {
        Target *newActiveTarget = (d->m_targets.size() == 0 ? nullptr : d->m_targets.at(0).get());
        setActiveTarget(newActiveTarget, SetActive::Cascade);
    }
    emit removedTarget(target);
}

const QList<Target *> Project::targets() const
{
    return toRawPointer<QList>(d->m_targets);
}

Target *Project::activeTarget() const
{
    return d->m_activeTarget;
}

void Project::setActiveTargetHelper(Target *target)
{
    if (d->m_activeTarget == target)
        return;

    // Allow to set nullptr just before the last target is removed or when no target exists.
    if ((!target && d->m_targets.size() == 0) ||
        (target && contains(d->m_targets, target))) {
        d->m_activeTarget = target;
        emit activeTargetChanged(d->m_activeTarget);
        emit activeBuildConfigurationChanged(target ? target->activeBuildConfiguration() : nullptr);
        if (this == ProjectManager::startupProject()) {
            emit ProjectManager::instance()->activeBuildConfigurationChanged(
                activeBuildConfiguration());
        }
        if (this == ProjectTree::currentProject()) {
            emit ProjectManager::instance()->currentBuildConfigurationChanged(
                activeBuildConfiguration());
        }
        ProjectExplorerPlugin::updateActions();
    }
}

bool Project::needsInitialExpansion() const
{
    return d->m_needsInitialExpansion;
}

void Project::setNeedsInitialExpansion(bool needsExpansion)
{
    d->m_needsInitialExpansion = needsExpansion;
}

void Project::setExtraProjectFiles(const QSet<FilePath> &projectDocumentPaths,
                                   const DocGenerator &docGenerator,
                                   const DocUpdater &docUpdater)
{
    QSet<FilePath> uniqueNewFiles = projectDocumentPaths;
    uniqueNewFiles.remove(projectFilePath()); // Make sure to never add the main project file!

    QSet<FilePath> existingWatches = transform<QSet>(d->m_extraProjectDocuments,
                                                     &IDocument::filePath);

    const QSet<FilePath> toAdd = uniqueNewFiles - existingWatches;
    const QSet<FilePath> toRemove = existingWatches - uniqueNewFiles;

    Utils::erase(d->m_extraProjectDocuments, [&toRemove](const std::unique_ptr<IDocument> &d) {
        return toRemove.contains(d->filePath());
    });
    if (docUpdater) {
        for (const auto &doc : std::as_const(d->m_extraProjectDocuments))
            docUpdater(doc.get());
    }
    QList<IDocument *> toRegister;
    for (const FilePath &p : toAdd) {
        if (docGenerator) {
            std::unique_ptr<IDocument> doc = docGenerator(p);
            QTC_ASSERT(doc, continue);
            d->m_extraProjectDocuments.push_back(std::move(doc));
        } else {
            auto document = std::make_unique<ProjectDocument>(d->m_document->mimeType(), p, this);
            toRegister.append(document.get());
            d->m_extraProjectDocuments.emplace_back(std::move(document));
        }
    }
    DocumentManager::addDocuments(toRegister);
}

void Project::updateExtraProjectFiles(const QSet<FilePath> &projectDocumentPaths,
                                      const DocUpdater &docUpdater)
{
    for (const FilePath &fp : projectDocumentPaths) {
        for (const auto &doc : d->m_extraProjectDocuments) {
            if (doc->filePath() == fp) {
                docUpdater(doc.get());
                break;
            }
        }
    }
}

void Project::updateExtraProjectFiles(const DocUpdater &docUpdater)
{
    for (const auto &doc : std::as_const(d->m_extraProjectDocuments))
        docUpdater(doc.get());
}

Target *Project::target(Id id) const
{
    return findOrDefault(d->m_targets, equal(&Target::id, id));
}

Target *Project::target(Kit *k) const
{
    return findOrDefault(d->m_targets, equal(&Target::kit, k));
}

void Project::setActiveTarget(Target *target, SetActive cascade)
{
    if (isShuttingDown())
        return;

    setActiveTargetHelper(target);

    if (!target) // never cascade setting no target
        return;

    if (cascade != SetActive::Cascade || !ProjectManager::isProjectConfigurationCascading())
        return;

    Utils::Id kitId = target->kit()->id();
    for (Project *otherProject : ProjectManager::projects()) {
        if (otherProject == this)
            continue;
        if (Target *otherTarget = Utils::findOrDefault(otherProject->targets(),
                                                       [kitId](Target *t) { return t->kit()->id() == kitId; }))
            otherProject->setActiveTargetHelper(otherTarget);
    }
}

void Project::setActiveBuildConfiguration(BuildConfiguration *bc, SetActive cascade)
{
    QTC_ASSERT(bc->project() == this, return);
    if (bc != bc->target()->activeBuildConfiguration())
        bc->target()->setActiveBuildConfiguration(bc, cascade);
    if (bc->target() != activeTarget())
        setActiveTarget(bc->target(), cascade);
}

Kit *Project::activeKit() const
{
    return activeTarget() ? activeTarget()->kit() : nullptr;
}

RunConfiguration *Project::activeRunConfiguration() const
{
    return activeTarget() ? activeTarget()->activeRunConfiguration() : nullptr;
}

BuildConfiguration *Project::activeBuildConfiguration() const
{
    return activeTarget() ? activeTarget()->activeBuildConfiguration() : nullptr;
}

DeployConfiguration *Project::activeDeployConfiguration() const
{
    return activeTarget() ? activeTarget()->activeDeployConfiguration() : nullptr;
}

BuildSystem *Project::activeBuildSystem() const
{
    return activeTarget() ? activeTarget()->buildSystem() : nullptr;
}

bool Project::isParsing() const
{
    if (BuildSystem * const bs = activeBuildSystem())
        return bs->isParsing() || bs->isWaitingForParse();
    return false;
}

QList<BuildConfiguration *> Project::allBuildConfigurations() const
{
    QList<BuildConfiguration *> buildConfigs;
    for (Target * const t : targets())
        buildConfigs << t->buildConfigurations();
    return buildConfigs;
}

void Project::setIssuesGenerator(const std::function<Tasks(const Kit *)> &generator)
{
    d->m_issuesGenerator = generator;
}

QList<Store> Project::vanishedTargets() const
{
    return d->m_vanishedTargets;
}

void Project::removeVanishedTarget(int index)
{
    QTC_ASSERT(index >= 0 && index < d->m_vanishedTargets.size(), return);
    d->m_vanishedTargets.removeAt(index);
    emit vanishedTargetsChanged();
}

void Project::removeVanishedTarget(const Utils::Store &store)
{
    if (d->m_vanishedTargets.removeAll(store))
        emit vanishedTargetsChanged();
}

void Project::removeAllVanishedTargets()
{
    d->m_vanishedTargets.clear();
    emit vanishedTargetsChanged();
}

Target *Project::createKitAndTargetFromStore(const Utils::Store &store)
{
    Id deviceTypeId = Id::fromSetting(store.value(Target::deviceTypeKey()));
    if (!deviceTypeId.isValid())
        deviceTypeId = Constants::DESKTOP_DEVICE_TYPE;
    const QString formerKitName = store.value(Target::displayNameKey()).toString();
    Kit *k = KitManager::registerKit(
        [deviceTypeId, &formerKitName](Kit *kit) {
            const QString kitName = makeUniquelyNumbered(formerKitName,
                                                         transform(KitManager::kits(),
                                                                   &Kit::unexpandedDisplayName));
            kit->setUnexpandedDisplayName(kitName);
            RunDeviceTypeKitAspect::setDeviceTypeId(kit, deviceTypeId);
            kit->setup();
        });
    QTC_ASSERT(k, return nullptr);
    auto t = Target::create(this, k);
    if (!t->fromMap(store))
        return nullptr;

    if (t->buildConfigurations().isEmpty())
        return nullptr;

    auto pointer = t.get();
    addTarget(std::move(t));
    return pointer;
}

Tasks Project::projectIssues(const Kit *k) const
{
    if (!k->isValid())
        return {createTask(Task::TaskType::Error, ::PE::Tr::tr("Kit is not valid."))};
    if (const Task t = checkBuildDevice(k, projectFilePath()); !t.isNull())
        return {t};
    if (d->m_issuesGenerator)
        return d->m_issuesGenerator(k);
    return {};
}

Task Project::checkBuildDevice(const Kit *k, const Utils::FilePath &projectFile)
{
    IDeviceConstPtr buildDevice = BuildDeviceKitAspect::device(k);
    if (!buildDevice)
        return createTask(Task::TaskType::Error, ::PE::Tr::tr("Kit has no build device."));
    const Utils::Result<> supportsBuilding = buildDevice->supportsBuildingProject(
        projectFile.parentDir());
    if (!supportsBuilding) {
        return createTask(
            Task::TaskType::Error,
            ::PE::Tr::tr("Build device \"%2\" cannot handle project file \"%1\".")
                    .arg(projectFile.toUserOutput())
                    .arg(buildDevice->displayName())
                + QString(" ") + supportsBuilding.error());
    }
    return {};
}

void Project::addTask(const Task &task)
{
    d->m_tasks << task;
    TaskHub::addTask(task);
}

bool Project::copySteps(Target *sourceTarget, Kit *targetKit)
{
    QTC_ASSERT(targetKit, return false);

    bool fatalError = false;
    QStringList buildconfigurationError;
    QStringList deployconfigurationError;
    QStringList runconfigurationError;

    std::unique_ptr<Target> tempTarget;
    Target *newTarget = target(targetKit->id());
    if (!newTarget) {
        tempTarget = Target::create(this, targetKit);
        newTarget = tempTarget.get();
    }

    for (BuildConfiguration *sourceBc : sourceTarget->buildConfigurations()) {
        BuildConfiguration *newBc = sourceBc->clone(newTarget);
        if (!newBc) {
            buildconfigurationError << sourceBc->displayName();
            continue;
        }
        newBc->setDisplayName(sourceBc->displayName());
        newBc->setBuildDirectory(
            BuildConfiguration::rawBuildDirectoryFromTemplate(targetKit, projectFilePath()));
        newTarget->addBuildConfiguration(newBc);
        if (sourceTarget->activeBuildConfiguration() == sourceBc)
            newTarget->setActiveBuildConfiguration(newBc, SetActive::NoCascade);
    }

    if (!newTarget->activeBuildConfiguration()) {
        QList<BuildConfiguration *> bcs = newTarget->buildConfigurations();
        if (!bcs.isEmpty())
            newTarget->setActiveBuildConfiguration(bcs.first(), SetActive::NoCascade);
    }

    if (buildconfigurationError.count() == sourceTarget->buildConfigurations().count())
        fatalError = true;

    if (fatalError) {
        // That could be a more granular error message
        QMessageBox::critical(ICore::dialogParent(),
                              ::PE::Tr::tr("Incompatible Kit"),
                              ::PE::Tr::tr("Kit %1 is incompatible with kit %2.")
                                  .arg(sourceTarget->kit()->displayName())
                                  .arg(targetKit->displayName()));
    } else if (!buildconfigurationError.isEmpty()
               || !deployconfigurationError.isEmpty()
               || ! runconfigurationError.isEmpty()) {

        QString error;
        if (!buildconfigurationError.isEmpty())
            error += ::PE::Tr::tr("Build configurations:") + QLatin1Char('\n')
                     + buildconfigurationError.join(QLatin1Char('\n'));

        if (!deployconfigurationError.isEmpty()) {
            if (!error.isEmpty())
                error.append(QLatin1Char('\n'));
            error += ::PE::Tr::tr("Deploy configurations:") + QLatin1Char('\n')
                     + deployconfigurationError.join(QLatin1Char('\n'));
        }

        if (!runconfigurationError.isEmpty()) {
            if (!error.isEmpty())
                error.append(QLatin1Char('\n'));
            error += ::PE::Tr::tr("Run configurations:") + QLatin1Char('\n')
                     + runconfigurationError.join(QLatin1Char('\n'));
        }

        QMessageBox msgBox(ICore::dialogParent());
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle(::PE::Tr::tr("Partially Incompatible Kit"));
        msgBox.setText(::PE::Tr::tr("Some configurations could not be copied."));
        msgBox.setDetailedText(error);
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        fatalError = msgBox.exec() != QDialog::Accepted;
    }

    if (fatalError)
        return false;

    addTarget(std::move(tempTarget));
    return true;
}

bool Project::copySteps(const Utils::Store &store, Kit *targetKit)
{
    Target *t = target(targetKit->id());
    if (!t) {
        auto t = Target::create(this, targetKit);
        if (!t->fromMap(store))
            return false;

        if (t->buildConfigurations().isEmpty())
            return false;

        addTarget(std::move(t));
        return true;
    }
    return t->addConfigurationsFromMap(store, /*setActiveConfigurations=*/false);
}

void Project::setDisplayName(const QString &name)
{
    if (name == d->m_displayName)
        return;
    d->m_displayName = name;
    emit displayNameChanged();
}

void Project::setType(Id id)
{
    QTC_ASSERT(!d->m_id.isValid(), return); // Id may not change ever!
    d->m_id = id;
}

void Project::setRootProjectNode(std::unique_ptr<ProjectNode> &&root)
{
    QTC_ASSERT(d->m_rootProjectNode.get() != root.get() || !root, return);

    if (root && root->isEmpty()) {
        // Something went wrong with parsing: At least the project file needs to be
        // shown so that the user can fix the breakage.
        // Do not leak root and use default project tree in this case.
        root.reset();
    }

    if (root) {
        ProjectTree::applyTreeManager(root.get(), ProjectTree::AsyncPhase);
        ProjectTree::applyTreeManager(root.get(), ProjectTree::FinalPhase);
        root->setParentFolderNode(d->m_containerNode.get());
    }

    std::unique_ptr<ProjectNode> oldNode = std::move(d->m_rootProjectNode);

    d->m_rootProjectNode = std::move(root);
    if (oldNode || d->m_rootProjectNode)
        handleSubTreeChanged(d->m_containerNode.get());
}

void Project::handleSubTreeChanged(FolderNode *node)
{
    QList<const Node *> nodeList;
    if (d->m_rootProjectNode) {
        d->m_rootProjectNode->forEachGenericNode([&nodeList](const Node *n) {
            nodeList.append(n);
        });
        sort(nodeList, &nodeLessThan);
    }
    d->m_sortedNodeList = nodeList;

    ProjectTree::emitSubtreeChanged(node);
    emit fileListChanged();
}

void Project::saveSettings()
{
    emit aboutToSaveSettings();
    if (!d->m_accessor)
        d->m_accessor = std::make_unique<Internal::UserFileAccessor>(this);
    if (!targets().isEmpty()) {
        Store map;
        toMap(map);
        d->m_accessor->saveSettings(map);
    }
}

Project::RestoreResult Project::restoreSettings(QString *errorMessage)
{
    if (!KitManager::waitForLoaded()) {
        if (errorMessage)
            *errorMessage = Tr::tr("Could not load kits in a reasonable amount of time.");
        return RestoreResult::Error;
    }

    if (!d->m_accessor)
        d->m_accessor = std::make_unique<Internal::UserFileAccessor>(this);
    Store map = d->m_accessor->restoreSettings();
    RestoreResult result = fromMap(map, errorMessage);
    if (result == RestoreResult::Ok)
        emit settingsLoaded();

    syncRunConfigurations(false);

    return result;
}

/*!
 * Returns a sorted list of all files matching the predicate \a filter.
 */
FilePaths Project::files(const NodeMatcher &filter) const
{
    QTC_ASSERT(filter, return {});

    FilePaths result;
    if (d->m_sortedNodeList.empty() && filter(containerNode()))
        result.append(projectFilePath());

    FilePath lastAdded;
    for (const Node *n : std::as_const(d->m_sortedNodeList)) {
        if (!filter(n))
            continue;

        // Remove duplicates:
        const FilePath path = n->filePath();
        if (path == lastAdded)
            continue; // skip duplicates
        lastAdded = path;

        result.append(path);
    }
    return result;
}

/*!
    Serializes all data into a Store.

    This map is then saved in the .user file of the project.
    Just put all your data into the map.

    \note Do not forget to call your base class' toMap function.
    \note Do not forget to call setActiveBuildConfiguration when
    creating new build configurations.
*/

void Project::toMap(Store &map) const
{
    const QList<Target *> ts = targets();
    const QList<Store> vts = vanishedTargets();

    map.insert(ACTIVE_TARGET_KEY, ts.indexOf(d->m_activeTarget));
    map.insert(TARGET_COUNT_KEY, ts.size() + vts.size());
    int index = 0;
    for (Target *t : ts) {
        map.insert(numberedKey(TARGET_KEY_PREFIX, index), variantFromStore(t->toMap()));
        ++index;
    }
    for (const Store &store : vts) {
        map.insert(numberedKey(TARGET_KEY_PREFIX, index), variantFromStore(store));
        ++index;
    }

    map.insert(EDITOR_SETTINGS_KEY, variantFromStore(d->m_editorConfiguration.toMap()));
    if (!d->m_pluginSettings.isEmpty())
        map.insert(PLUGIN_SETTINGS_KEY, variantFromStore(d->m_pluginSettings));
}

/*!
    Returns the directory that contains the project.

    This includes the absolute path.
*/

FilePath Project::projectDirectory() const
{
    return projectFilePath().absolutePath();
}

void Project::changeRootProjectDirectory()
{
    FilePath rootPath = FileUtils::getExistingDirectory(::PE::Tr::tr("Select the Root Directory"),
                                                        rootProjectDirectory(),
                                                        QFileDialog::ShowDirsOnly
                                                            | QFileDialog::DontResolveSymlinks);
    if (rootPath != d->m_rootProjectDirectory) {
        d->m_rootProjectDirectory = rootPath;
        setNamedSettings(Constants::PROJECT_ROOT_PATH_KEY, d->m_rootProjectDirectory.toUrlishString());
        emit rootProjectDirectoryChanged();
    }
}

/*!
    Returns the common root directory that contains all files which belong to a project.
*/
FilePath Project::rootProjectDirectory() const
{
    if (!d->m_rootProjectDirectory.isEmpty())
        return d->m_rootProjectDirectory;

    return projectDirectory();
}

ProjectNode *Project::rootProjectNode() const
{
    return d->m_rootProjectNode.get();
}

ContainerNode *Project::containerNode() const
{
    return d->m_containerNode.get();
}

Project::RestoreResult Project::fromMap(const Store &map, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    if (map.contains(EDITOR_SETTINGS_KEY)) {
        Store values = storeFromVariant(map.value(EDITOR_SETTINGS_KEY));
        d->m_editorConfiguration.fromMap(values);
    }

    if (map.contains(PLUGIN_SETTINGS_KEY))
        d->m_pluginSettings = storeFromVariant(map.value(PLUGIN_SETTINGS_KEY));

    bool ok;
    int maxI(map.value(TARGET_COUNT_KEY, 0).toInt(&ok));
    if (!ok || maxI < 0)
        maxI = 0;
    int active = map.value(ACTIVE_TARGET_KEY, 0).toInt(&ok);
    if (!ok || active < 0 || active >= maxI)
        active = 0;

    if (active >= 0 && active < maxI)
        createTargetFromMap(map, active); // sets activeTarget since it is the first target created!

    for (int i = 0; i < maxI; ++i) {
        if (i == active) // already covered!
            continue;

        createTargetFromMap(map, i);
    }

    d->m_rootProjectDirectory = FilePath::fromString(
        namedSettings(Constants::PROJECT_ROOT_PATH_KEY).toString());

    d->m_projectExplorerSettings.restore();

    return RestoreResult::Ok;
}

void Project::createTargetFromMap(const Store &map, int index)
{
    const Key key = numberedKey(TARGET_KEY_PREFIX, index);
    if (!map.contains(key))
        return;

    const Store targetMap = storeFromVariant(map.value(key));

    Id id = idFromMap(targetMap);
    if (target(id)) {
        qWarning("Warning: Duplicated target id found, not restoring second target with id '%s'. Continuing.",
                 qPrintable(id.toString()));
        return;
    }

    Kit *k = KitManager::kit(id);
    if (!k) {
        d->m_vanishedTargets.append(targetMap);
        const QString formerKitName = targetMap.value(Target::displayNameKey()).toString();
        TaskHub::addTask<BuildSystemTask>(
            Task::Warning,
            ::PE::Tr::tr(
                "Project \"%1\" was configured for "
                "kit \"%2\" with id %3, which does not exist anymore. You can create a new kit "
                "or copy the steps of the vanished kit to another kit in %4 mode.")
                .arg(displayName(), formerKitName, id.toString(), Tr::tr("Projects")));
        return;
    }

    auto t = Target::create(this, k);
    if (!t->fromMap(targetMap))
        return;

    if (t->buildConfigurations().isEmpty())
        return;

    addTarget(std::move(t));
}

EditorConfiguration *Project::editorConfiguration() const
{
    return &d->m_editorConfiguration;
}

bool Project::isKnownFile(const FilePath &filename) const
{
    if (d->m_sortedNodeList.empty())
        return filename == projectFilePath();
    const FileNode element(filename, FileType::Unknown);
    return std::binary_search(std::begin(d->m_sortedNodeList), std::end(d->m_sortedNodeList),
                              &element, nodeLessThan);
}

const Node *Project::nodeForFilePath(const FilePath &filePath,
                                     const NodeMatcher &extraMatcher) const
{
    const QList<const Node *> nodes = nodesForFilePath(filePath, extraMatcher);
    return nodes.isEmpty() ? nullptr : nodes.first();
}

QList<const Node *> Project::nodesForFilePath(const Utils::FilePath &filePath,
                                              const NodeMatcher &extraMatcher) const
{
    const FileNode dummy(filePath, FileType::Unknown);
    const auto range = std::equal_range(d->m_sortedNodeList.cbegin(), d->m_sortedNodeList.cend(),
                                        &dummy, &nodeLessThan);
    QList<const Node *> nodes;
    for (auto it = range.first; it != range.second; ++it) {
        if ((*it)->filePath() == filePath && (!extraMatcher || extraMatcher(*it)))
            nodes << *it;
    }
    return nodes;
}

ProjectNode *Project::productNodeForFilePath(
    const Utils::FilePath &filePath, const NodeMatcher &extraMatcher) const
{
    const QList<const Node *> fileNodes = nodesForFilePath(filePath, extraMatcher);
    QList<ProjectNode *> candidates;
    for (const Node * const fileNode : fileNodes) {
        for (ProjectNode *projectNode = fileNode->parentProjectNode(); projectNode;
             projectNode = projectNode->parentProjectNode()) {
            if (projectNode->isProduct()) {

                // If there are several candidates, we prefer real products to pseudo-products.
                // See QTCREATORBUG-33224. For now, we assume this is what all callers want.
                // If the need arises, we can make this configurable.
                if (projectNode->productType() == ProductType::App
                    || projectNode->productType() == ProductType::Lib) {
                    return projectNode;
                }

                candidates << projectNode;
            }
        }
    }
    return candidates.isEmpty() ? nullptr : candidates.first();
}

FilePaths Project::binariesForSourceFile(const FilePath &sourceFile) const
{
    if (!rootProjectNode())
        return {};
    const QList<Node *> fileNodes = rootProjectNode()->findNodes([&sourceFile](Node *n) {
        return n->filePath() == sourceFile;
    });
    FilePaths binaries;
    for (const Node * const fileNode : fileNodes) {
        for (ProjectNode *projectNode = fileNode->parentProjectNode(); projectNode;
             projectNode = projectNode->parentProjectNode()) {
            if (!projectNode->isProduct())
                continue;
            if (projectNode->productType() == ProductType::App
                || projectNode->productType() == ProductType::Lib) {
                const QList<Node *> binaryNodes = projectNode->findNodes([](Node *n) {
                    return n->asFileNode() && (n->asFileNode()->fileType() == FileType::App
                               || n->asFileNode()->fileType() == FileType::Lib);

                });
                binaries << Utils::transform(binaryNodes, &Node::filePath);
            }
            break;
        }
    }
    return binaries;
}

void Project::setProjectLanguages(Context language)
{
    if (d->m_projectLanguages == language)
        return;
    d->m_projectLanguages = language;
    emit projectLanguagesUpdated();
}

void Project::addProjectLanguage(Id id)
{
    Context lang = projectLanguages();
    int pos = lang.indexOf(id);
    if (pos < 0)
        lang.add(id);
    setProjectLanguages(lang);
}

void Project::removeProjectLanguage(Id id)
{
    Context lang = projectLanguages();
    int pos = lang.indexOf(id);
    if (pos >= 0)
        lang.removeAt(pos);
    setProjectLanguages(lang);
}

void Project::setProjectLanguage(Id id, bool enabled)
{
    if (enabled)
        addProjectLanguage(id);
    else
        removeProjectLanguage(id);
}

void Project::setHasMakeInstallEquivalent(bool enabled)
{
    d->m_hasMakeInstallEquivalent = enabled;
}

void Project::setSupportsBuilding(bool value)
{
    d->m_supportsBuilding = value;
}

void Project::setBuildSystemName(const QString &name)
{
    d->m_buildSystemName = name;
}

Task Project::createTask(Task::TaskType type, const QString &description)
{
    return Task(type, description, FilePath(), -1, Id());
}

void Project::setBuildSystemCreator(const std::function<BuildSystem *(BuildConfiguration *)> &creator)
{
    d->m_buildSystemCreator = creator;
}

Context Project::projectContext() const
{
    return Context(d->m_id);
}

Context Project::projectLanguages() const
{
    return d->m_projectLanguages;
}

QVariant Project::namedSettings(const Key &name) const
{
    return d->m_pluginSettings.value(name);
}

void Project::setNamedSettings(const Key &name, const QVariant &value)
{
    if (value.isNull())
        d->m_pluginSettings.remove(name);
    else
        d->m_pluginSettings.insert(name, value);
}

void Project::setAdditionalEnvironment(const EnvironmentItems &envItems)
{
    setNamedSettings(PROJECT_ENV_KEY, EnvironmentItem::toStringList(envItems));
    emit environmentChanged();
}

EnvironmentItems Project::additionalEnvironment() const
{
    return EnvironmentItem::fromStringList(namedSettings(PROJECT_ENV_KEY).toStringList());
}

bool Project::needsConfiguration() const
{
    return d->m_targets.size() == 0;
}

bool Project::supportsBuilding() const
{
    return d->m_supportsBuilding;
}

void Project::configureAsExampleProject(Kit * /*kit*/)
{
}

DeploymentKnowledge Project::deploymentKnowledge() const
{
    return DeploymentKnowledge::Bad;
}

bool Project::hasMakeInstallEquivalent() const
{
    return d->m_hasMakeInstallEquivalent;
}

void Project::setup(const QList<BuildInfo> &infoList)
{
    for (const BuildInfo &info : infoList)
        setup(info);
}

BuildConfiguration *Project::setup(const BuildInfo &info)
{
    Kit *k = KitManager::kit(info.kitId);
    if (!k)
        return nullptr;
    Target *t = target(k);
    std::unique_ptr<Target> newTarget;
    if (!t) {
        newTarget = Target::create(this, k);
        t = newTarget.get();
    }

    QTC_ASSERT(t, return nullptr);

    BuildConfiguration *bc = nullptr;
    if (info.factory) {
        bc = info.factory->create(t, info);
        if (bc)
            t->addBuildConfiguration(bc);
    }
    if (newTarget)
        addTarget(std::move(newTarget));
    return bc;
}

MacroExpander *Project::macroExpander() const
{
    return &d->m_macroExpander;
}

ProjectNode *Project::findNodeForBuildKey(const QString &buildKey) const
{
    if (!d->m_rootProjectNode)
        return nullptr;

    return d->m_rootProjectNode->findProjectNode([buildKey](const ProjectNode *node) {
        return node->buildKey() == buildKey;
    });
}

ProjectImporter *Project::projectImporter() const
{
    return nullptr;
}

void Project::setCanBuildProducts()
{
    d->m_canBuildProducts = true;
}

void Project::setIsEditModePreferred(bool preferEditMode)
{
    d->m_isEditModePreferred = preferEditMode;
}

void Project::setExtraData(const Key &key, const QVariant &data)
{
    d->m_extraData.insert(key, data);
}

QVariant Project::extraData(const Key &key) const
{
    return d->m_extraData.value(key);
}

QStringList Project::availableQmlPreviewTranslations(QString *errorMessage)
{
    const auto projectDirectory = rootProjectDirectory().toFileInfo().absoluteFilePath();
    const QDir languageDirectory(projectDirectory + "/i18n");
    const auto qmFiles = languageDirectory.entryList({"qml_*.qm"});
    if (qmFiles.isEmpty() && errorMessage)
        errorMessage->append(::PE::Tr::tr("Could not find any qml_*.qm file at \"%1\"")
                                 .arg(languageDirectory.absolutePath()));
    return transform(qmFiles, [](const QString &qmFile) {
        const int localeStartPosition = qmFile.lastIndexOf("_") + 1;
        const int localeEndPosition = qmFile.size() - QString(".qm").size();
        const QString locale = qmFile.left(localeEndPosition).mid(localeStartPosition);
        return locale;
    });
}

QList<IDocument *> Project::modifiedDocuments() const
{
    QList<IDocument *> modifiedProjectDocuments;

    for (IDocument *doc : DocumentModel::openedDocuments()) {
        if (doc->isModified() && isKnownFile(doc->filePath()))
            modifiedProjectDocuments.append(doc);
    }

    return modifiedProjectDocuments;
}

bool Project::isModified() const
{
    return !modifiedDocuments().isEmpty();
}

bool Project::isEditModePreferred() const
{
    return d->m_isEditModePreferred;
}

void Project::registerGenerator(Utils::Id id, const QString &displayName,
                                const std::function<void ()> &runner)
{
    d->m_generators.insert(id, qMakePair(displayName, runner));
}

const QList<QPair<Id, QString>> Project::allGenerators() const
{
    QList<QPair<Id, QString>> generators;
    for (auto it = d->m_generators.cbegin(); it != d->m_generators.cend(); ++it)
        generators << qMakePair(it.key(), it.value().first);
    if (const BuildSystem * const bs = activeBuildSystem())
        generators += bs->generators();
    return generators;
}

void Project::runGenerator(Utils::Id id)
{
    const auto it = d->m_generators.constFind(id);
    if (it != d->m_generators.constEnd()) {
        it.value().second();
        return;
    }
    if (BuildSystem * const bs = activeBuildSystem())
        bs->runGenerator(id);
}

void Project::addVariablesToMacroExpander(const QByteArray &prefix,
                                          const QString &descriptor,
                                          MacroExpander *expander,
                                          const std::function<Project *()> &projectGetter)
{
    const auto kitGetter = [projectGetter] { return ProjectExplorer::activeKit(projectGetter()); };
    const auto bcGetter = [projectGetter] { return activeBuildConfig(projectGetter()); };
    const auto rcGetter = [projectGetter] { return activeRunConfig(projectGetter()); };
    const QByteArray fullPrefix = (prefix.endsWith(':') ? prefix : prefix + ':');
    const QByteArray prefixWithoutColon = fullPrefix.chopped(1);
    expander->registerVariable(fullPrefix + "Name",
                               //: %1 is something like "Active project"
                               ::PE::Tr::tr("%1: Name.").arg(descriptor),
                               [projectGetter]() -> QString {
                                   if (const Project *const project = projectGetter())
                                       return project->displayName();
                                   return {};
                               });
    expander->registerFileVariables(prefixWithoutColon,
                                    //: %1 is something like "Active project"
                                    ::PE::Tr::tr("%1: Full path to main file.").arg(descriptor),
                                    [projectGetter]() -> FilePath {
                                        if (const Project *const project = projectGetter())
                                            return project->projectFilePath();
                                        return {};
                                    });
    expander->registerVariable(fullPrefix + "ProjectDirectory",
                               //: %1 is something like "Active project"
                               ::PE::Tr::tr("%1: Full path to Project Directory.").arg(descriptor),
                               [projectGetter]() -> QString {
                                   if (const Project *const project = projectGetter())
                                       return project->projectDirectory().toUserOutput();
                                   return {};
                               });
    expander->registerVariable(fullPrefix + "Kit:Name",
                               //: %1 is something like "Active project"
                               ::PE::Tr::tr("%1: The name of the active kit.").arg(descriptor),
                               [kitGetter]() -> QString {
                                   if (const Kit *const kit = kitGetter())
                                       return kit->displayName();
                                   return {};
                               });
    expander->registerVariable(fullPrefix + "BuildConfig:Name",
                               //: %1 is something like "Active project"
                               ::PE::Tr::tr("%1: Name of the active build configuration.")
                                   .arg(descriptor),
                               [bcGetter]() -> QString {
                                   if (const BuildConfiguration *const bc = bcGetter())
                                       return bc->displayName();
                                   return {};
                               });
    expander->registerVariable(fullPrefix + "BuildConfig:Type",
                               //: %1 is something like "Active project"
                               ::PE::Tr::tr("%1: Type of the active build configuration.")
                                   .arg(descriptor),
                               [bcGetter]() -> QString {
                                   const BuildConfiguration *const bc = bcGetter();
                                   const BuildConfiguration::BuildType type
                                       = bc ? bc->buildType() : BuildConfiguration::Unknown;
                                   return BuildConfiguration::buildTypeName(type);
                               });
    expander->registerVariable(fullPrefix + "BuildConfig:Path",
                               //: %1 is something like "Active project"
                               ::PE::Tr::tr("%1: Full build path of active build configuration.")
                                   .arg(descriptor),
                               [bcGetter]() -> QString {
                                   if (const BuildConfiguration *const bc = bcGetter())
                                       return bc->buildDirectory().toUserOutput();
                                   return {};
                               });
    expander->registerPrefix(
        fullPrefix + "BuildConfig:Env",
        "USER",
        //: %1 is something like "Active project"
        ::PE::Tr::tr("%1: Variables in the active build environment.").arg(descriptor),
        [bcGetter](const QString &var) -> QString {
            if (BuildConfiguration *const bc = bcGetter())
                return bc->environment().expandedValueForKey(var);
            return {};
        });

    expander->registerVariable(fullPrefix + "RunConfig:Name",
                               //: %1 is something like "Active project"
                               ::PE::Tr::tr("%1: Name of the active run configuration.")
                                   .arg(descriptor),
                               [rcGetter]() -> QString {
                                   if (const RunConfiguration *const rc = rcGetter())
                                       return rc->displayName();
                                   return {};
                               });
    expander->registerFileVariables(fullPrefix + "RunConfig:Executable",
                                    //: %1 is something like "Active project"
                                    ::PE::Tr::tr("%1: Executable of the active run configuration.")
                                        .arg(descriptor),
                                    [rcGetter]() -> FilePath {
                                        if (const RunConfiguration *const rc = rcGetter())
                                            return rc->commandLine().executable();
                                        return {};
                                    });
    expander->registerPrefix(
        fullPrefix + "RunConfig:Env",
        "USER",
        //: %1 is something like "Active project"
        ::PE::Tr::tr("%1: Variables in the environment of the active run configuration.")
            .arg(descriptor),
        [rcGetter](const QString &var) -> QString {
            if (const RunConfiguration *const rc = rcGetter()) {
                if (const auto envAspect = rc->aspect<EnvironmentAspect>())
                    return envAspect->environment().expandedValueForKey(var);
            }
            return {};
        });
    expander->registerVariable(fullPrefix + "RunConfig:WorkingDir",
                               //: %1 is something like "Active project"
                               ::PE::Tr::tr(
                                   "%1: Working directory of the active run configuration.")
                                   .arg(descriptor),
                               [rcGetter]() -> QString {
                                   if (const RunConfiguration *const rc = rcGetter()) {
                                       if (const auto wdAspect
                                           = rc->aspect<WorkingDirectoryAspect>())
                                           return wdAspect->workingDirectory().toUrlishString();
                                   }
                                   return {};
                               });
}

TextEditor::ProjectWrapper wrapProject(Project *p)
{
    return TextEditor::ProjectWrapper(p, [](const void *p) {
        return reinterpret_cast<const Project *>(p)->projectFilePath();
    });
}

Project *unwrapProject(const TextEditor::ProjectWrapper &w)
{
    return reinterpret_cast<Project *>(w.project());
}

static Project::QmlCodeModelInfoFromQtVersionHook s_qtversionExtraProjectInfoHook;

void Project::setQmlCodeModelInfoFromQtVersionHook(QmlCodeModelInfoFromQtVersionHook hook)
{
   s_qtversionExtraProjectInfoHook = hook;
}

static void findAllQrcFiles(const FilePath &filePath, FilePaths &out)
{
    filePath.iterateDirectory(
        [&out](const FilePath &path) {
            out.append(path.canonicalPath());
            return IterationPolicy::Continue;
        },
        {{"*.qrc"}, QDir::Files});
}

static bool s_qmlCodeModelIsUsed = false;

void Project::setQmlCodeModelIsUsed()
{
    s_qmlCodeModelIsUsed = true;
}

void Project::updateQmlCodeModel(Kit *kit, BuildConfiguration *bc)
{
    if (!s_qmlCodeModelIsUsed)
        return;

    QmlCodeModelInfo projectInfo = gatherQmlCodeModelInfo(kit, bc);

    emit ProjectManager::instance()->extraProjectInfoChanged(bc, projectInfo);
}

void Project::resetQmlCodeModel()
{
    if (!s_qmlCodeModelIsUsed)
        return;

    emit ProjectManager::instance()->requestCodeModelReset();
}

static QStringList getQmlExtensions()
{
    using namespace Utils::Constants;
    static const QSet<QString> qmlTypeNames
        = {QML_MIMETYPE, QBS_MIMETYPE, QMLPROJECT_MIMETYPE, QMLTYPES_MIMETYPE, QMLUI_MIMETYPE};
    QStringList extensions;
    for (const QString &mtn : qmlTypeNames) {
        const MimeType mt = mimeTypeForName(mtn);
        for (const QString &pattern : mt.globPatterns()) {
            if (pattern.size() > 2 && pattern.at(0) == '*' && pattern.at(1) == '.')
                extensions.append(pattern.mid(1));
        }
    }
    return extensions;
}

QmlCodeModelInfo Project::gatherQmlCodeModelInfo(Kit *kit, BuildConfiguration *bc)
{
    QmlCodeModelInfo projectInfo;

    projectInfo.sourceFiles = files([extensions = getQmlExtensions()](const Node *n) {
        if (!Project::SourceFiles(n))
            return false;
        const FileNode *fn = n->asFileNode();
        return fn && fn->fileType() == FileType::QML
               && Utils::anyOf(extensions, [fp = fn->filePath()](const QString &ext) {
                      return fp.endsWith(ext);
                  });
    });

    if (projectInfo.sourceFiles.isEmpty())
        return projectInfo;

    projectInfo.qmlDumpEnvironment = Environment::systemEnvironment();

    projectInfo.tryQmlDump = false;

    FilePath baseDir;
    auto addAppDir = [&baseDir, &projectInfo](const FilePath &mdir) {
        const FilePath dir = mdir.cleanPath();
        if (!baseDir.path().isEmpty()) {
            const QString rDir = dir.relativePathFromDir(baseDir);
            // do not add directories outside the build directory
            // this might happen for example when we think an executable path belongs to
            // a bundle, and we need to remove extra directories, but that was not the case
            if (rDir.split(u'/').contains(QStringLiteral(u"..")))
                return;
        }
        if (!projectInfo.applicationDirectories.contains(dir))
            projectInfo.applicationDirectories.append(dir);
    };

    if (bc && bc->buildSystem()) {
        // Append QML2_IMPORT_PATH if it is defined in build configuration.
        // It enables qmlplugindump to correctly dump custom plugins or other dependent
        // plugins that are not installed in default Qt qml installation directory.
        projectInfo.qmlDumpEnvironment.appendOrSet("QML2_IMPORT_PATH",
                                                   bc->environment().expandedValueForKey(
                                                       "QML2_IMPORT_PATH"));
        // Treat every target (library or application) in the build directory

        FilePath dir = bc->buildDirectory();
        baseDir = dir.absoluteFilePath();
        addAppDir(dir);

        // Qml loads modules from the following sources
        // 1. The build directory of the executable
        // 2. Any QML_IMPORT_PATH (environment variable) or IMPORT_PATH (parameter to qt_add_qml_module)
        // 3. The Qt import path
        // For an IDE things are a bit more complicated because source files might be edited,
        // and the directory of the executable might be outdated.
        // Here we try to get the directory of the executable, adding all targets
        for (const BuildTargetInfo &target : bc->buildSystem()->applicationTargets()) {
            if (target.targetFilePath.isEmpty())
                continue;
            FilePath dir = target.targetFilePath.parentDir();
            projectInfo.applicationDirectories.append(dir);
            // unfortunately the build directory of the executable where cmake puts the qml
            // might be different than the directory of the executable:
            if (HostOsInfo::isWindowsHost()) {
                // On Windows systems QML type information is located one directory higher as we build
                // in dedicated "debug" and "release" directories
                addAppDir(dir.parentDir());
            } else if (HostOsInfo::isMacHost()) {
                // On macOS and iOS when building a bundle this is not the case and
                // we have to go up up to three additional directories
                // (BundleName.app/Contents/MacOS or BundleName.app/Contents for iOS)
                if (dir.fileName() == u"MacOS")
                    dir = dir.parentDir();
                if (dir.fileName() == u"Contents")
                    dir = dir.parentDir().parentDir();
                addAppDir(dir);
            }
        }

        // Let leaves do corrections.
        bc->buildSystem()->updateQmlCodeModelInfo(projectInfo);
    }

    QSet<FilePath> rccDirs;
    if (rootProjectNode()) {
        rootProjectNode()->forEachNode(
            {},
            [&rccDirs](FolderNode *n) {
                static const QString rcc = "rcc";
                static const QString dotRcc = ".rcc";
                static const QString dotQt = ".qt";
                const FilePath &filePath = n->filePath();
                const QStringView fileName = filePath.fileNameView();
                if (fileName == dotRcc
                    || (fileName == rcc && filePath.parentDir().fileNameView() == dotQt)) {
                    rccDirs.insert(n->filePath());
                }
            },
            {});
    }
    for (const FilePath &fp : std::as_const(rccDirs))
        findAllQrcFiles(fp, projectInfo.generatedQrcFiles);

    if (s_qtversionExtraProjectInfoHook && kit)
        s_qtversionExtraProjectInfoHook(kit, projectInfo);

    return projectInfo;
}

#ifdef WITH_TESTS

namespace Internal {

static FilePath constructTestPath(const QString &basePath)
{
    FilePath drive;
    if (HostOsInfo::isWindowsHost())
        drive = "C:";
    return drive.stringAppended(basePath);
}

const FilePath TEST_PROJECT_PATH = constructTestPath("/tmp/foobar/baz.project");
const FilePath TEST_PROJECT_NONEXISTING_FILE = constructTestPath("/tmp/foobar/nothing.cpp");
const FilePath TEST_PROJECT_CPP_FILE = constructTestPath("/tmp/foobar/main.cpp");
const FilePath TEST_PROJECT_GENERATED_FILE = constructTestPath("/tmp/foobar/generated.foo");
const QString TEST_PROJECT_MIMETYPE = "application/vnd.test.qmakeprofile";
const QString TEST_PROJECT_DISPLAYNAME = "testProjectFoo";
const char TEST_PROJECT_ID[] = "Test.Project.Id";

class TestBuildSystem : public BuildSystem
{
public:
    using BuildSystem::BuildSystem;

    static QString name() { return "test"; }
    void triggerParsing() final {}

    bool canRenameFile(Node *, const FilePath &, const FilePath &) override
    {
        ++canRenameFileCount;
        return true;
    }
    bool renameFiles(Node *, const FilePairs &, FilePaths *) override
    {
        ++renameFilesCount;
        return true;
    }

    int canRenameFileCount = 0;
    int renameFilesCount = 0;
};

class RejectingAllRenameBuildSystem : public TestBuildSystem
{
public:
    using TestBuildSystem::TestBuildSystem;
    static QString name() { return "RejectingAll"; }
    bool renameFiles(Node *, const FilePairs &, FilePaths *) override
    {
        ++renameFilesCount;
        return false;
    }
};

class PartiallyRejectingRenameBuildSystem : public TestBuildSystem
{
public:
    using TestBuildSystem::TestBuildSystem;
    static QString name() { return "RejectingSome"; }
    static inline FilePaths pathsToReject;
    bool renameFiles(Node *, const FilePairs &filePairs, FilePaths *notRenamed) override
    {
        ++renameFilesCount;
        for (auto &[o, _] : filePairs) {
            if (pathsToReject.contains(o) && notRenamed)
                notRenamed->append(o);
        }
        return true;
    }
};

class ReparsingBuildSystem : public TestBuildSystem
{
public:
    using TestBuildSystem::TestBuildSystem;
    static QString name() { return "Reparsing"; }
    static inline QPointer<Project> projectToReparse;
    bool renameFiles(Node *originNode, const FilePairs &filePairs, FilePaths *notRenamed) override
    {
        const bool ok = TestBuildSystem::renameFiles(originNode, filePairs, notRenamed);
        if (notRenamed) {
            for (const auto &pair : filePairs)
                notRenamed->append(pair.first);
        }
        if (projectToReparse) {
            auto newRoot = std::make_unique<ProjectNode>(projectToReparse->projectFilePath());
            projectToReparse->setRootProjectNode(std::move(newRoot));
            if (ProjectTree::instance())
                emit ProjectTree::instance()->subtreeChanged(nullptr);
        }
        return ok;
    }
};

class TestBuildConfigurationFactory : public BuildConfigurationFactory
{
public:
    TestBuildConfigurationFactory()
    {
        setSupportedProjectType(TEST_PROJECT_ID);
        setBuildGenerator([](const Kit *, const FilePath &projectFilePath, bool) {
            BuildInfo bi;
            bi.buildSystemName = TestBuildSystem::name();
            bi.buildDirectory = projectFilePath.parentDir();
            bi.displayName = "default";
            return QList<BuildInfo>{bi};
        });
        registerBuildConfiguration<BuildConfiguration>("TestProject.BuildConfiguration");
    }
};

class TestProject : public Project
{
public:
    TestProject() : Project(TEST_PROJECT_MIMETYPE, TEST_PROJECT_PATH)
    {
        setType(TEST_PROJECT_ID);
        setDisplayName(TEST_PROJECT_DISPLAYNAME);
        setBuildSystemCreator<TestBuildSystem>();
        target = addTargetForKit(&testKit);
    }

    bool needsConfiguration() const final { return false; }

    Kit testKit;
    Target *target = nullptr;
};

class RenameTestProject : public Project
{
public:
    explicit RenameTestProject(const TemporaryDirectory &td)
        : Project(TEST_PROJECT_MIMETYPE, td.path())
    {
        setType(TEST_PROJECT_ID);
        setDisplayName(TEST_PROJECT_DISPLAYNAME);
    }

    bool needsConfiguration() const final { return false; }

    template<typename BS>
    void initialize()
    {
        setBuildSystemCreator<BS>();
        target = addTargetForKit(&kit);
        createNodes();
        Q_ASSERT(dynamic_cast<BS *>(target->buildSystem()));
    }

    TestBuildSystem *testBuildSystem()
    {
        return dynamic_cast<TestBuildSystem *>(target->buildSystem());
    }
    Kit kit;
    Target *target = nullptr;
    FilePath sourceFile, secondSourceFile;

private:
    void createNodes()
    {
        sourceFile = projectFilePath().pathAppended("test.cpp");
        secondSourceFile = projectFilePath().pathAppended("test2.cpp");
        QVERIFY(sourceFile.writeFileContents("content1"));
        QVERIFY(secondSourceFile.writeFileContents("content2"));

        auto root = std::make_unique<ProjectNode>(projectFilePath());
        std::vector<std::unique_ptr<FileNode>> vec;
        vec.emplace_back(std::make_unique<FileNode>(projectFilePath(), FileType::Project));
        vec.emplace_back(std::make_unique<FileNode>(sourceFile, FileType::Source));
        vec.emplace_back(std::make_unique<FileNode>(secondSourceFile, FileType::Source));
        root->addNestedNodes(std::move(vec));
        setRootProjectNode(std::move(root));
    }
};

static FilePath makeRenamedFilePath(const FilePath &original)
{
    return original.chopped(4).stringAppended("_renamed.cpp");
}

class ProjectTest : public QObject
{
    Q_OBJECT

private slots:
    void testSetup()
    {
        TestProject project;

        QCOMPARE(project.displayName(), TEST_PROJECT_DISPLAYNAME);

        QVERIFY(!project.rootProjectNode());
        QVERIFY(project.containerNode());

        QVERIFY(project.macroExpander());

        QCOMPARE(project.mimeType(), TEST_PROJECT_MIMETYPE);
        QCOMPARE(project.projectFilePath(), TEST_PROJECT_PATH);
        QCOMPARE(project.projectDirectory(), TEST_PROJECT_PATH.parentDir());

        QCOMPARE(project.isKnownFile(TEST_PROJECT_PATH), true);
        QCOMPARE(project.isKnownFile(TEST_PROJECT_NONEXISTING_FILE), false);
        QCOMPARE(project.isKnownFile(TEST_PROJECT_CPP_FILE), false);

        QCOMPARE(project.files(Project::AllFiles), FilePaths{TEST_PROJECT_PATH});
        QCOMPARE(project.files(Project::GeneratedFiles), {});

        QCOMPARE(project.type(), Id(TEST_PROJECT_ID));

        QVERIFY(!project.activeBuildSystem()->isParsing());
        QVERIFY(!project.activeBuildSystem()->hasParsingData());
    }

    void testChangeDisplayName()
    {
        TestProject project;

        QSignalSpy spy(&project, &Project::displayNameChanged);

        const QString newName = "other name";
        project.setDisplayName(newName);
        QCOMPARE(spy.count(), 1);
        QVariantList args = spy.takeFirst();
        QCOMPARE(args, {});

        project.setDisplayName(newName);
        QCOMPARE(spy.count(), 0);
    }

    void testParsingSuccess()
    {
        TestProject project;

        QSignalSpy startSpy(project.activeBuildSystem(), &BuildSystem::parsingStarted);
        QSignalSpy stopSpy(project.activeBuildSystem(), &BuildSystem::parsingFinished);

        {
            BuildSystem::ParseGuard guard = project.activeBuildSystem()->guardParsingRun();
            QCOMPARE(startSpy.count(), 1);
            QCOMPARE(stopSpy.count(), 0);

            QVERIFY(project.activeBuildSystem()->isParsing());
            QVERIFY(!project.activeBuildSystem()->hasParsingData());

            guard.markAsSuccess();
        }

        QCOMPARE(startSpy.count(), 1);
        QCOMPARE(stopSpy.count(), 1);
        QCOMPARE(stopSpy.at(0), {QVariant(true)});

        QVERIFY(!project.activeBuildSystem()->isParsing());
        QVERIFY(project.activeBuildSystem()->hasParsingData());
    }

    void testParsingFail()
    {
        TestProject project;

        QSignalSpy startSpy(project.activeBuildSystem(), &BuildSystem::parsingStarted);
        QSignalSpy stopSpy(project.activeBuildSystem(), &BuildSystem::parsingFinished);

        {
            BuildSystem::ParseGuard guard = project.activeBuildSystem()->guardParsingRun();
            QCOMPARE(startSpy.count(), 1);
            QCOMPARE(stopSpy.count(), 0);

            QVERIFY(project.activeBuildSystem()->isParsing());
            QVERIFY(!project.activeBuildSystem()->hasParsingData());
        }

        QCOMPARE(startSpy.count(), 1);
        QCOMPARE(stopSpy.count(), 1);
        QCOMPARE(stopSpy.at(0), {QVariant(false)});

        QVERIFY(!project.activeBuildSystem()->isParsing());
        QVERIFY(!project.activeBuildSystem()->hasParsingData());
    }

    static std::unique_ptr<ProjectNode> createFileTree(Project *project)
    {
        std::unique_ptr<ProjectNode> root = std::make_unique<ProjectNode>(project->projectDirectory());
        std::vector<std::unique_ptr<FileNode>> nodes;
        nodes.emplace_back(std::make_unique<FileNode>(TEST_PROJECT_PATH, FileType::Project));
        nodes.emplace_back(std::make_unique<FileNode>(TEST_PROJECT_CPP_FILE, FileType::Source));
        nodes.emplace_back(std::make_unique<FileNode>(TEST_PROJECT_GENERATED_FILE, FileType::Source));
        nodes.back()->setIsGenerated(true);
        root->addNestedNodes(std::move(nodes));

        return root;
    }

    void testProjectTree()
    {
        TestProject project;
        QSignalSpy fileSpy(&project, &Project::fileListChanged);

        project.setRootProjectNode(nullptr);
        QCOMPARE(fileSpy.count(), 0);
        QVERIFY(!project.rootProjectNode());

        project.setRootProjectNode(std::make_unique<ProjectNode>(project.projectDirectory()));
        QCOMPARE(fileSpy.count(), 0);
        QVERIFY(!project.rootProjectNode());

        std::unique_ptr<ProjectNode> root = createFileTree(&project);
        ProjectNode *rootNode = root.get();
        project.setRootProjectNode(std::move(root));
        QCOMPARE(fileSpy.count(), 1);
        QCOMPARE(project.rootProjectNode(), rootNode);

        // Test known files:
        QCOMPARE(project.isKnownFile(TEST_PROJECT_PATH), true);
        QCOMPARE(project.isKnownFile(TEST_PROJECT_NONEXISTING_FILE), false);
        QCOMPARE(project.isKnownFile(TEST_PROJECT_CPP_FILE), true);
        QCOMPARE(project.isKnownFile(TEST_PROJECT_GENERATED_FILE), true);

        FilePaths allFiles = project.files(Project::AllFiles);
        QCOMPARE(allFiles.count(), 3);
        QVERIFY(allFiles.contains(TEST_PROJECT_PATH));
        QVERIFY(allFiles.contains(TEST_PROJECT_CPP_FILE));
        QVERIFY(allFiles.contains(TEST_PROJECT_GENERATED_FILE));

        QCOMPARE(project.files(Project::GeneratedFiles), FilePaths{TEST_PROJECT_GENERATED_FILE});
        FilePaths sourceFiles = project.files(Project::SourceFiles);
        QCOMPARE(sourceFiles.count(), 2);
        QVERIFY(sourceFiles.contains(TEST_PROJECT_PATH));
        QVERIFY(sourceFiles.contains(TEST_PROJECT_CPP_FILE));

        project.setRootProjectNode(nullptr);
        QCOMPARE(fileSpy.count(), 2);
        QVERIFY(!project.rootProjectNode());
    }

    void testMultipleBuildConfigs()
    {
        // Find suitable kit.
        Kit * const kit = findOr(KitManager::kits(), nullptr, [](const Kit *k) {
            return k->isValid();
        });
        if (!kit)
            QSKIP("The test requires at least one valid kit.");

        // Copy project from qrc file and set it up.
        QTemporaryDir * const tempDir = TemporaryDirectory::masterTemporaryDirectory();
        QVERIFY(tempDir->isValid());
        const FilePath projectDir = FilePath::fromString(tempDir->path() + "/generic-project");
        const auto copyResult = FilePath(":/projectexplorer/testdata/generic-project").copyRecursively(projectDir);
        if (!copyResult)
            qDebug() << copyResult.error();
        QVERIFY(copyResult);

        const QFileInfoList files = QDir(projectDir.toUrlishString()).entryInfoList(QDir::Files | QDir::Dirs);
        for (const QFileInfo &f : files)
            QFile(f.absoluteFilePath()).setPermissions(f.permissions() | QFile::WriteUser);
        const auto theProject = ProjectExplorerPlugin::openProject(projectDir.pathAppended("generic-project.creator"));
        QVERIFY2(theProject, qPrintable(theProject.errorMessage()));
        theProject.project()->configureAsExampleProject(kit);
        QCOMPARE(theProject.project()->targets().size(), 1);
        Target * const target = theProject.project()->activeTarget();
        QVERIFY(target);
        QCOMPARE(target->buildConfigurations().size(), 6);
        target->setActiveBuildConfiguration(target->buildConfigurations().at(1), SetActive::Cascade);
        BuildSystem * const bs = theProject.project()->activeBuildSystem();
        QVERIFY(bs);
        QCOMPARE(bs, target->activeBuildConfiguration()->buildSystem());
        if (bs->isWaitingForParse() || bs->isParsing()) {
            QEventLoop loop;
            QTimer t;
            t.setSingleShot(true);
            connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
            connect(bs, &BuildSystem::parsingFinished, &loop, &QEventLoop::quit);
            t.start(10000);
            QVERIFY(loop.exec());
            QVERIFY(t.isActive());
        }
        QVERIFY(!bs->isWaitingForParse() && !bs->isParsing());

        QCOMPARE(ProjectManager::startupProject(), theProject.project());
        QCOMPARE(ProjectTree::currentProject(), theProject.project());
        QVERIFY(EditorManager::openEditor(projectDir.pathAppended("main.cpp")));
        QVERIFY(ProjectTree::currentNode());
        ProjectTree::instance()->expandAll();
        ProjectManager::closeAllProjects(); // QTCREATORBUG-25655
    }

    void testSourceToBinaryMapping()
    {
        // Find suitable kit.
        Kit * const kit = findOr(KitManager::kits(), nullptr, [](const Kit *k) {
            return k->isValid() && ToolchainKitAspect::cxxToolchain(k);
        });
        if (!kit)
            QSKIP("The test requires at least one kit with a toolchain.");

        const auto toolchain = ToolchainKitAspect::cxxToolchain(kit);
        QVERIFY(toolchain);
        if (const auto msvcToolchain = dynamic_cast<Internal::MsvcToolchain *>(toolchain)) {
            while (!msvcToolchain->environmentInitialized()) {
                QSignalSpy parsingFinishedSpy(ToolchainManager::instance(),
                                              &ToolchainManager::toolchainUpdated);
                QVERIFY(parsingFinishedSpy.wait(10000));
            }
        }

        // Copy project from qrc.
        QTemporaryDir * const tempDir = TemporaryDirectory::masterTemporaryDirectory();
        QVERIFY(tempDir->isValid());
        const FilePath projectDir = FilePath::fromString(tempDir->path() + "/multi-target-project");
        if (!projectDir.exists()) {
            const auto result = FilePath(":/projectexplorer/testdata/multi-target-project")
            .copyRecursively(projectDir);
            if (!result)
                qDebug() << result.error();
            QVERIFY(result);
            const QFileInfoList files = QDir(projectDir.toUrlishString()).entryInfoList(QDir::Files);
            for (const QFileInfo &f : files)
                QFile(f.absoluteFilePath()).setPermissions(f.permissions() | QFile::WriteUser);
        }

        // Load Project.
        QFETCH(QString, projectFileName);
        const auto theProject = ProjectExplorerPlugin::openProject(projectDir.pathAppended(projectFileName));
        if (!theProject
            && !ProjectManager::canOpenProjectForMimeType(Utils::mimeTypeForFile(projectFileName))) {
            QSKIP("This test requires the presence of the qmake/cmake/qbs project managers "
                  "to be fully functional");
        }

        QVERIFY2(theProject, qPrintable(theProject.errorMessage()));
        theProject.project()->configureAsExampleProject(kit);
        QCOMPARE(theProject.project()->targets().size(), 1);
        BuildSystem * const bs = theProject.project()->activeBuildSystem();
        QVERIFY(bs);
        if (bs->isWaitingForParse() || bs->isParsing()) {
            QSignalSpy parsingFinishedSpy(bs, &BuildSystem::parsingFinished);
            QVERIFY(parsingFinishedSpy.wait(10000));
        }
        QVERIFY(!bs->isWaitingForParse() && !bs->isParsing());

        if (QLatin1String(QTest::currentDataTag()) == QLatin1String("qbs")) {
            BuildManager::buildProjectWithoutDependencies(theProject.project());
            if (BuildManager::isBuilding()) {
                QSignalSpy buildingFinishedSpy(BuildManager::instance(), &BuildManager::buildQueueFinished);
                QVERIFY(buildingFinishedSpy.wait(10000));
            }
            QVERIFY(!BuildManager::isBuilding());
            QSignalSpy projectUpdateSpy(theProject.project(), &Project::fileListChanged);
            QVERIFY(projectUpdateSpy.wait(5000));
        }

        // Check mapping
        const auto binariesForSource = [&](const QString &fileName) {
            return theProject.project()->binariesForSourceFile(projectDir.pathAppended(fileName));
        };
        QCOMPARE(binariesForSource("multi-target-project-main.cpp").size(), 1);
        QCOMPARE(binariesForSource("multi-target-project-lib.cpp").size(), 1);
        QCOMPARE(binariesForSource("multi-target-project-shared.h").size(), 2);
    }

    void testSourceToBinaryMapping_data()
    {
        QTest::addColumn<QString>("projectFileName");
        QTest::addRow("cmake") << "CMakeLists.txt";
        QTest::addRow("qbs") << "multi-target-project.qbs";
        QTest::addRow("qmake") << "multi-target-project.pro";
    }

    void testRenameFile()
    {
        TemporaryDirectory tempDir("testProject_renameFile");
        RenameTestProject testProject(tempDir);
        testProject.initialize<TestBuildSystem>();
        auto sourceFileNode = const_cast<Node *>(testProject.nodeForFilePath(testProject.sourceFile));
        QVERIFY(sourceFileNode);
        const FilePath testRenamed = makeRenamedFilePath(testProject.sourceFile);
        QList<std::pair<Node *, FilePath>> nodesToRename{{sourceFileNode, testRenamed}};

        const FilePairs result = ProjectExplorerPlugin::renameFiles(nodesToRename);

        QCOMPARE(result.size(), 1);
        QCOMPARE(testProject.testBuildSystem()->canRenameFileCount, 1);
        QCOMPARE(testProject.testBuildSystem()->renameFilesCount, 1);
        QCOMPARE(testProject.sourceFile.exists(), false);
        QCOMPARE(testRenamed.exists(), true);
    }

    void testRenameFile_NullNode()
    {
        TemporaryDirectory tempDir("testProject_renameFile_NullNode");
        RenameTestProject testProject(tempDir);
        testProject.initialize<TestBuildSystem>();
        const FilePath testRenamed = makeRenamedFilePath(testProject.sourceFile);
        QList<std::pair<Node *, FilePath>> nodesToRename{{nullptr, testRenamed}};

        const FilePairs result = ProjectExplorerPlugin::renameFiles(nodesToRename);

        QVERIFY(result.isEmpty());
    }

    void testRenameMultipleFiles()
    {
        TemporaryDirectory tempDir("testProject_renameMultipleFiles");
        RenameTestProject testProject(tempDir);
        testProject.initialize<TestBuildSystem>();
        const FilePath renamedFilePath1 = makeRenamedFilePath(testProject.sourceFile);
        const FilePath renamedFilePath2 = makeRenamedFilePath(testProject.secondSourceFile);
        auto testNode1 = const_cast<Node *>(testProject.nodeForFilePath(testProject.sourceFile));
        auto testNode2 = const_cast<Node *>(testProject.nodeForFilePath(testProject.secondSourceFile));
        QList<std::pair<Node *, FilePath>>
            nodesToRename{{testNode1, renamedFilePath1}, {testNode2, renamedFilePath2}};

        const FilePairs result = ProjectExplorerPlugin::renameFiles(nodesToRename);

        QCOMPARE(result.size(), 2);
        QCOMPARE(testProject.testBuildSystem()->canRenameFileCount, 2);
        QCOMPARE(testProject.testBuildSystem()->renameFilesCount, 1);
        QCOMPARE(testProject.sourceFile.exists(), false);
        QCOMPARE(testProject.secondSourceFile.exists(), false);
        QCOMPARE(renamedFilePath1.exists(), true);
        QCOMPARE(renamedFilePath2.exists(), true);
    }

    void testRenameFile_BuildSystemRejectsAll()
    {
        TemporaryDirectory tempDir("testProject_renameFile_BuildSystemRejectsAll");
        RenameTestProject testProject(tempDir);
        testProject.initialize<RejectingAllRenameBuildSystem>();
        Node *sourcNode = const_cast<Node *>(testProject.nodeForFilePath(testProject.sourceFile));
        const FilePath renamedPath = makeRenamedFilePath(testProject.sourceFile);
        QList<std::pair<Node *, FilePath>> toRename{{sourcNode, renamedPath}};

        const FilePairs result = ProjectExplorerPlugin::renameFiles(toRename);

        QVERIFY(result.isEmpty());
        QCOMPARE(testProject.testBuildSystem()->canRenameFileCount, 1);
        QCOMPARE(testProject.testBuildSystem()->renameFilesCount, 1);
        QCOMPARE(testProject.sourceFile.exists(), false);
        QCOMPARE(renamedPath.exists(), true);
    }

    void testRenameFile_BuildSystemRejectsPartial()
    {
        TemporaryDirectory tempDir("testProject_renameFile_BuildSystemRejectsPartial");
        RenameTestProject testProject(tempDir);
        testProject.initialize<PartiallyRejectingRenameBuildSystem>();
        PartiallyRejectingRenameBuildSystem::pathsToReject = {testProject.sourceFile};
        const FilePath renamed1 = makeRenamedFilePath(testProject.sourceFile);
        const FilePath renamed2 = makeRenamedFilePath(testProject.secondSourceFile);
        Node *node1 = const_cast<Node *>(testProject.nodeForFilePath(testProject.sourceFile));
        Node *node2 = const_cast<Node *>(testProject.nodeForFilePath(testProject.secondSourceFile));
        QList<std::pair<Node *, FilePath>> toRename{{node1, renamed1}, {node2, renamed2}};

        const FilePairs result = ProjectExplorerPlugin::renameFiles(toRename);

        QCOMPARE(result.size(), 1);
        QVERIFY(result.contains({testProject.secondSourceFile, renamed2}));
        QCOMPARE(testProject.testBuildSystem()->canRenameFileCount, 2);
        QCOMPARE(testProject.testBuildSystem()->renameFilesCount, 1);
        QCOMPARE(testProject.sourceFile.exists(), false);
        QCOMPARE(testProject.secondSourceFile.exists(), false);
        QCOMPARE(renamed1.exists(), true);
        QCOMPARE(renamed2.exists(), true);
    }

    void testRenameFile_QmlCrashSimulation()
    {
        TemporaryDirectory tempDir("testProject_renameFile_QmlCrashSimulation");
        RenameTestProject testProject(tempDir);
        testProject.initialize<ReparsingBuildSystem>();
        ReparsingBuildSystem::projectToReparse = &testProject;
        Node *sourceNode = const_cast<Node *>(testProject.nodeForFilePath(testProject.sourceFile));
        const FilePath renamedPath = makeRenamedFilePath(testProject.sourceFile);
        QList<std::pair<Node *, FilePath>> toRename{{sourceNode, renamedPath}};

        const FilePairs result = ProjectExplorerPlugin::renameFiles(toRename);

        QVERIFY(renamedPath.exists());
        QCOMPARE(testProject.sourceFile.exists(), false);
        QVERIFY(result.isEmpty());
    }
};

static TestBuildConfigurationFactory testBuildConfigFactory;

QObject *createProjectTest()
{
    return new ProjectTest;
}

} // namespace Internal

#endif // WITH_TESTS

} // namepace ProjectExplorer

#ifdef WITH_TESTS
#include <project.moc>
#endif
