/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "project.h"

#include "buildconfiguration.h"
#include "buildinfo.h"
#include "buildsystem.h"
#include "deployconfiguration.h"
#include "editorconfiguration.h"
#include "kit.h"
#include "kitinformation.h"
#include "makestep.h"
#include "projectexplorer.h"
#include "projectnodes.h"
#include "runconfiguration.h"
#include "runcontrol.h"
#include "session.h"
#include "target.h"
#include "taskhub.h"
#include "userfileaccessor.h"

#include <coreplugin/idocument.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/editormanager/documentmodel.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projecttree.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/macroexpander.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QFileDialog>

#include <limits>

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

    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) final
    {
        Q_UNUSED(errorString)
        Q_UNUSED(flag)
        Q_UNUSED(type)

        emit m_project->projectFileIsDirty(filePath());
        return true;
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
    ~ProjectPrivate();

    Id m_id;
    bool m_needsInitialExpansion = false;
    bool m_canBuildProducts = false;
    bool m_hasMakeInstallEquivalent = false;
    bool m_needsBuildConfigurations = true;
    bool m_needsDeployConfigurations = true;
    bool m_shuttingDown = false;

    std::function<BuildSystem *(Target *)> m_buildSystemCreator;

    std::unique_ptr<IDocument> m_document;
    std::vector<std::unique_ptr<IDocument>> m_extraProjectDocuments;
    std::unique_ptr<ProjectNode> m_rootProjectNode;
    std::unique_ptr<ContainerNode> m_containerNode;
    std::vector<std::unique_ptr<Target>> m_targets;
    Target *m_activeTarget = nullptr;
    EditorConfiguration m_editorConfiguration;
    Context m_projectLanguages;
    QVariantMap m_pluginSettings;
    std::unique_ptr<Internal::UserFileAccessor> m_accessor;

    QString m_displayName;

    MacroExpander m_macroExpander;
    FilePath m_rootProjectDirectory;
    mutable QVector<const Node *> m_sortedNodeList;

    QVariantMap m_extraData;
};

ProjectPrivate::~ProjectPrivate()
{
    // Make sure our root node is null when deleting the actual node
    std::unique_ptr<ProjectNode> oldNode = std::move(m_rootProjectNode);
}

Project::Project(const QString &mimeType, const FilePath &fileName)
    : d(new ProjectPrivate)
{
    d->m_document = std::make_unique<ProjectDocument>(mimeType, fileName, this);
    DocumentManager::addDocument(d->m_document.get());

    d->m_macroExpander.setDisplayName(tr("Project"));
    d->m_macroExpander.registerVariable("Project:Name", tr("Project Name"),
            [this] { return displayName(); });

    // Only set up containernode after d is set so that it will find the project directory!
    d->m_containerNode = std::make_unique<ContainerNode>(this);
}

Project::~Project()
{
    delete d;
}

QString Project::displayName() const
{
    return d->m_displayName;
}

Id Project::id() const
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

BuildSystem *Project::createBuildSystem(Target *target) const
{
    return d->m_buildSystemCreator ? d->m_buildSystemCreator(target) : nullptr;
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
        SessionManager::setActiveTarget(this, pointer, SetActive::Cascade);
}

Target *Project::addTargetForDefaultKit()
{
    return addTargetForKit(KitManager::defaultKit());
}

Target *Project::addTargetForKit(Kit *kit)
{
    if (!kit || target(kit))
        return nullptr;

    auto t = std::make_unique<Target>(this, kit, Target::_constructor_tag{});
    Target *pointer = t.get();

    if (!setupTarget(pointer))
        return {};

    addTarget(std::move(t));

    return pointer;
}

bool Project::removeTarget(Target *target)
{
    QTC_ASSERT(target && contains(d->m_targets, target), return false);

    if (BuildManager::isBuilding(target))
        return false;

    target->markAsShuttingDown();
    emit aboutToRemoveTarget(target);
    auto keep = take(d->m_targets, target);
    if (target == d->m_activeTarget) {
        Target *newActiveTarget = (d->m_targets.size() == 0 ? nullptr : d->m_targets.at(0).get());
        SessionManager::setActiveTarget(this, newActiveTarget, SetActive::Cascade);
    }
    emit removedTarget(target);

    return true;
}

const QList<Target *> Project::targets() const
{
    return toRawPointer<QList>(d->m_targets);
}

Target *Project::activeTarget() const
{
    return d->m_activeTarget;
}

void Project::setActiveTarget(Target *target)
{
    if (d->m_activeTarget == target)
        return;

    // Allow to set nullptr just before the last target is removed or when no target exists.
    if ((!target && d->m_targets.size() == 0) ||
        (target && contains(d->m_targets, target))) {
        d->m_activeTarget = target;
        emit activeTargetChanged(d->m_activeTarget);
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
        for (const auto &doc : qAsConst(d->m_extraProjectDocuments))
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
    for (const auto &doc : qAsConst(d->m_extraProjectDocuments))
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

Tasks Project::projectIssues(const Kit *k) const
{
    Tasks result;
    if (!k->isValid())
        result.append(createProjectTask(Task::TaskType::Error, tr("Kit is not valid.")));
    return {};
}

bool Project::copySteps(Target *sourceTarget, Target *newTarget)
{
    QTC_ASSERT(newTarget, return false);
    bool fatalError = false;
    QStringList buildconfigurationError;
    QStringList deployconfigurationError;
    QStringList runconfigurationError;

    const Project * const project = newTarget->project();
    for (BuildConfiguration *sourceBc : sourceTarget->buildConfigurations()) {
        BuildConfiguration *newBc = BuildConfigurationFactory::clone(newTarget, sourceBc);
        if (!newBc) {
            buildconfigurationError << sourceBc->displayName();
            continue;
        }
        newBc->setDisplayName(sourceBc->displayName());
        newBc->setBuildDirectory(BuildConfiguration::buildDirectoryFromTemplate(
                    project->projectDirectory(), project->projectFilePath(),
                    project->displayName(), newTarget->kit(),
                    sourceBc->displayName(), sourceBc->buildType()));
        newTarget->addBuildConfiguration(newBc);
        if (sourceTarget->activeBuildConfiguration() == sourceBc)
            SessionManager::setActiveBuildConfiguration(newTarget, newBc, SetActive::NoCascade);
    }
    if (!newTarget->activeBuildConfiguration()) {
        QList<BuildConfiguration *> bcs = newTarget->buildConfigurations();
        if (!bcs.isEmpty())
            SessionManager::setActiveBuildConfiguration(newTarget, bcs.first(), SetActive::NoCascade);
    }

    for (DeployConfiguration *sourceDc : sourceTarget->deployConfigurations()) {
        DeployConfiguration *newDc = DeployConfigurationFactory::clone(newTarget, sourceDc);
        if (!newDc) {
            deployconfigurationError << sourceDc->displayName();
            continue;
        }
        newDc->setDisplayName(sourceDc->displayName());
        newTarget->addDeployConfiguration(newDc);
        if (sourceTarget->activeDeployConfiguration() == sourceDc)
            SessionManager::setActiveDeployConfiguration(newTarget, newDc, SetActive::NoCascade);
    }
    if (!newTarget->activeBuildConfiguration()) {
        QList<DeployConfiguration *> dcs = newTarget->deployConfigurations();
        if (!dcs.isEmpty())
            SessionManager::setActiveDeployConfiguration(newTarget, dcs.first(), SetActive::NoCascade);
    }

    for (RunConfiguration *sourceRc : sourceTarget->runConfigurations()) {
        RunConfiguration *newRc = RunConfigurationFactory::clone(newTarget, sourceRc);
        if (!newRc) {
            runconfigurationError << sourceRc->displayName();
            continue;
        }
        newRc->setDisplayName(sourceRc->displayName());
        newTarget->addRunConfiguration(newRc);
        if (sourceTarget->activeRunConfiguration() == sourceRc)
            newTarget->setActiveRunConfiguration(newRc);
    }
    if (!newTarget->activeRunConfiguration()) {
        QList<RunConfiguration *> rcs = newTarget->runConfigurations();
        if (!rcs.isEmpty())
            newTarget->setActiveRunConfiguration(rcs.first());
    }

    if (buildconfigurationError.count() == sourceTarget->buildConfigurations().count())
        fatalError = true;

    if (deployconfigurationError.count() == sourceTarget->deployConfigurations().count())
        fatalError = true;

    if (runconfigurationError.count() == sourceTarget->runConfigurations().count())
        fatalError = true;

    if (fatalError) {
        // That could be a more granular error message
        QMessageBox::critical(ICore::dialogParent(),
                              tr("Incompatible Kit"),
                              tr("Kit %1 is incompatible with kit %2.")
                                  .arg(sourceTarget->kit()->displayName())
                                  .arg(newTarget->kit()->displayName()));
    } else if (!buildconfigurationError.isEmpty()
               || !deployconfigurationError.isEmpty()
               || ! runconfigurationError.isEmpty()) {

        QString error;
        if (!buildconfigurationError.isEmpty())
            error += tr("Build configurations:") + QLatin1Char('\n')
                    + buildconfigurationError.join(QLatin1Char('\n'));

        if (!deployconfigurationError.isEmpty()) {
            if (!error.isEmpty())
                error.append(QLatin1Char('\n'));
            error += tr("Deploy configurations:") + QLatin1Char('\n')
                    + deployconfigurationError.join(QLatin1Char('\n'));
        }

        if (!runconfigurationError.isEmpty()) {
            if (!error.isEmpty())
                error.append(QLatin1Char('\n'));
            error += tr("Run configurations:") + QLatin1Char('\n')
                    + runconfigurationError.join(QLatin1Char('\n'));
        }

        QMessageBox msgBox(ICore::dialogParent());
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle(tr("Partially Incompatible Kit"));
        msgBox.setText(tr("Some configurations could not be copied."));
        msgBox.setDetailedText(error);
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        fatalError = msgBox.exec() != QDialog::Accepted;
    }

    return !fatalError;
}

bool Project::setupTarget(Target *t)
{
    if (d->m_needsBuildConfigurations)
        t->updateDefaultBuildConfigurations();
    if (d->m_needsDeployConfigurations)
        t->updateDefaultDeployConfigurations();
    t->updateDefaultRunConfigurations();
    return true;
}

void Project::setDisplayName(const QString &name)
{
    if (name == d->m_displayName)
        return;
    d->m_displayName = name;
    emit displayNameChanged();
}

void Project::setId(Id id)
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
    QVector<const Node *> nodeList;
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
    if (!targets().isEmpty())
        d->m_accessor->saveSettings(toMap(), ICore::dialogParent());
}

Project::RestoreResult Project::restoreSettings(QString *errorMessage)
{
    if (!d->m_accessor)
        d->m_accessor = std::make_unique<Internal::UserFileAccessor>(this);
    QVariantMap map(d->m_accessor->restoreSettings(ICore::dialogParent()));
    RestoreResult result = fromMap(map, errorMessage);
    if (result == RestoreResult::Ok)
        emit settingsLoaded();

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
    for (const Node *n : qAsConst(d->m_sortedNodeList)) {
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
    Serializes all data into a QVariantMap.

    This map is then saved in the .user file of the project.
    Just put all your data into the map.

    \note Do not forget to call your base class' toMap function.
    \note Do not forget to call setActiveBuildConfiguration when
    creating new build configurations.
*/

QVariantMap Project::toMap() const
{
    const QList<Target *> ts = targets();

    QVariantMap map;
    map.insert(QLatin1String(ACTIVE_TARGET_KEY), ts.indexOf(d->m_activeTarget));
    map.insert(QLatin1String(TARGET_COUNT_KEY), ts.size());
    for (int i = 0; i < ts.size(); ++i)
        map.insert(QString::fromLatin1(TARGET_KEY_PREFIX) + QString::number(i), ts.at(i)->toMap());

    map.insert(QLatin1String(EDITOR_SETTINGS_KEY), d->m_editorConfiguration.toMap());
    if (!d->m_pluginSettings.isEmpty())
        map.insert(QLatin1String(PLUGIN_SETTINGS_KEY), d->m_pluginSettings);

    return map;
}

/*!
    Returns the directory that contains the project.

    This includes the absolute path.
*/

FilePath Project::projectDirectory() const
{
    return projectDirectory(projectFilePath());
}

/*!
    Returns the directory that contains the file \a top.

    This includes the absolute path.
*/

FilePath Project::projectDirectory(const FilePath &top)
{
    if (top.isEmpty())
        return FilePath();
    return top.absolutePath();
}

void Project::changeRootProjectDirectory()
{
    FilePath rootPath = FileUtils::getExistingDirectory(
          nullptr,
          tr("Select the Root Directory"),
          rootProjectDirectory(),
          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (rootPath != d->m_rootProjectDirectory) {
        d->m_rootProjectDirectory = rootPath;
        setNamedSettings(Constants::PROJECT_ROOT_PATH_KEY, d->m_rootProjectDirectory.toString());
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

Project::RestoreResult Project::fromMap(const QVariantMap &map, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    if (map.contains(QLatin1String(EDITOR_SETTINGS_KEY))) {
        QVariantMap values(map.value(QLatin1String(EDITOR_SETTINGS_KEY)).toMap());
        d->m_editorConfiguration.fromMap(values);
    }

    if (map.contains(QLatin1String(PLUGIN_SETTINGS_KEY)))
        d->m_pluginSettings = map.value(QLatin1String(PLUGIN_SETTINGS_KEY)).toMap();

    bool ok;
    int maxI(map.value(QLatin1String(TARGET_COUNT_KEY), 0).toInt(&ok));
    if (!ok || maxI < 0)
        maxI = 0;
    int active(map.value(QLatin1String(ACTIVE_TARGET_KEY), 0).toInt(&ok));
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

    return RestoreResult::Ok;
}

void Project::createTargetFromMap(const QVariantMap &map, int index)
{
    const QString key = QString::fromLatin1(TARGET_KEY_PREFIX) + QString::number(index);
    if (!map.contains(key))
        return;

    const QVariantMap targetMap = map.value(key).toMap();

    Id id = idFromMap(targetMap);
    if (target(id)) {
        qWarning("Warning: Duplicated target id found, not restoring second target with id '%s'. Continuing.",
                 qPrintable(id.toString()));
        return;
    }

    Kit *k = KitManager::kit(id);
    if (!k) {
        Id deviceTypeId = Id::fromSetting(targetMap.value(Target::deviceTypeKey()));
        if (!deviceTypeId.isValid())
            deviceTypeId = Constants::DESKTOP_DEVICE_TYPE;
        const QString formerKitName = targetMap.value(Target::displayNameKey()).toString();
        k = KitManager::registerKit([deviceTypeId, &formerKitName](Kit *kit) {
                const QString kitNameSuggestion = formerKitName.contains(tr("Replacement for"))
                        ? formerKitName : tr("Replacement for \"%1\"").arg(formerKitName);
                const QString tempKitName = makeUniquelyNumbered(kitNameSuggestion,
                        transform(KitManager::kits(), &Kit::unexpandedDisplayName));
                kit->setUnexpandedDisplayName(tempKitName);
                DeviceTypeKitAspect::setDeviceTypeId(kit, deviceTypeId);
                kit->makeReplacementKit();
                kit->setup();
        }, id);
        QTC_ASSERT(k, return);
        TaskHub::addTask(BuildSystemTask(Task::Warning, tr("Project \"%1\" was configured for "
            "kit \"%2\" with id %3, which does not exist anymore. The new kit \"%4\" was "
            "created in its place, in an attempt not to lose custom project settings.")
                .arg(displayName(), formerKitName, id.toString(), k->displayName())));
    }

    auto t = std::make_unique<Target>(this, k, Target::_constructor_tag{});
    if (!t->fromMap(targetMap))
        return;

    if (t->runConfigurations().isEmpty() && t->buildConfigurations().isEmpty())
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
    const FileNode dummy(filePath, FileType::Unknown);
    const auto range = std::equal_range(d->m_sortedNodeList.cbegin(), d->m_sortedNodeList.cend(),
            &dummy, &nodeLessThan);
    for (auto it = range.first; it != range.second; ++it) {
        if ((*it)->filePath() == filePath && (!extraMatcher || extraMatcher(*it)))
            return *it;
    }
    return nullptr;
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

void Project::setNeedsBuildConfigurations(bool value)
{
    d->m_needsBuildConfigurations = value;
}

void Project::setNeedsDeployConfigurations(bool value)
{
    d->m_needsDeployConfigurations = value;
}

Task Project::createProjectTask(Task::TaskType type, const QString &description)
{
    return Task(type, description, FilePath(), -1, Id());
}

void Project::setBuildSystemCreator(const std::function<BuildSystem *(Target *)> &creator)
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

QVariant Project::namedSettings(const QString &name) const
{
    return d->m_pluginSettings.value(name);
}

void Project::setNamedSettings(const QString &name, const QVariant &value)
{
    if (value.isNull())
        d->m_pluginSettings.remove(name);
    else
        d->m_pluginSettings.insert(name, value);
}

void Project::setAdditionalEnvironment(const EnvironmentItems &envItems)
{
    setNamedSettings(PROJECT_ENV_KEY, NameValueItem::toStringList(envItems));
    emit environmentChanged();
}

EnvironmentItems Project::additionalEnvironment() const
{
    return NameValueItem::fromStringList(namedSettings(PROJECT_ENV_KEY).toStringList());
}

bool Project::needsConfiguration() const
{
    return d->m_targets.size() == 0;
}

bool Project::needsBuildConfigurations() const
{
    return d->m_needsBuildConfigurations;
}

void Project::configureAsExampleProject(Kit * /*kit*/)
{
}

bool Project::hasMakeInstallEquivalent() const
{
    return d->m_hasMakeInstallEquivalent;
}

MakeInstallCommand Project::makeInstallCommand(const Target *target, const QString &installRoot)
{
    QTC_ASSERT(hasMakeInstallEquivalent(), return MakeInstallCommand());
    MakeInstallCommand cmd;
    if (const BuildConfiguration * const bc = target->activeBuildConfiguration()) {
        if (const auto makeStep = bc->buildSteps()->firstOfType<MakeStep>())
            cmd.command = makeStep->makeExecutable();
    }
    cmd.arguments << "install" << ("INSTALL_ROOT=" + QDir::toNativeSeparators(installRoot));
    return cmd;
}

void Project::setup(const QList<BuildInfo> &infoList)
{
    std::vector<std::unique_ptr<Target>> toRegister;
    for (const BuildInfo &info : infoList) {
        Kit *k = KitManager::kit(info.kitId);
        if (!k)
            continue;
        Target *t = target(k);
        if (!t)
            t = findOrDefault(toRegister, equal(&Target::kit, k));
        if (!t) {
            auto newTarget = std::make_unique<Target>(this, k, Target::_constructor_tag{});
            t = newTarget.get();
            toRegister.emplace_back(std::move(newTarget));
        }

        if (!info.factory)
            continue;

        if (BuildConfiguration *bc = info.factory->create(t, info))
            t->addBuildConfiguration(bc);
    }
    for (std::unique_ptr<Target> &t : toRegister) {
        t->updateDefaultDeployConfigurations();
        t->updateDefaultRunConfigurations();
        addTarget(std::move(t));
    }
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

void Project::setExtraData(const QString &key, const QVariant &data)
{
    d->m_extraData.insert(key, data);
}

QVariant Project::extraData(const QString &key) const
{
    return d->m_extraData.value(key);
}

QStringList Project::availableQmlPreviewTranslations(QString *errorMessage)
{
    const auto projectDirectory = rootProjectDirectory().toFileInfo().absoluteFilePath();
    const QDir languageDirectory(projectDirectory + "/i18n");
    const auto qmFiles = languageDirectory.entryList({"qml_*.qm"});
    if (qmFiles.isEmpty() && errorMessage)
        errorMessage->append(tr("Could not find any qml_*.qm file at \"%1\"").arg(languageDirectory.absolutePath()));
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
    return true;
}

#if defined(WITH_TESTS)

static FilePath constructTestPath(const char *basePath)
{
    FilePath drive;
    if (HostOsInfo::isWindowsHost())
        drive = "C:";
    return drive + QLatin1String(basePath);
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

    void triggerParsing() final {}
    QString name() const final { return QLatin1String("test"); }
};

class TestProject : public Project
{
public:
    TestProject() : Project(TEST_PROJECT_MIMETYPE, TEST_PROJECT_PATH)
    {
        setId(TEST_PROJECT_ID);
        setDisplayName(TEST_PROJECT_DISPLAYNAME);
        setBuildSystemCreator([](Target *t) { return new TestBuildSystem(t); });
        setNeedsBuildConfigurations(false);
        setNeedsDeployConfigurations(false);

        target = addTargetForKit(&testKit);
    }

    bool needsConfiguration() const final { return false; }

    Kit testKit;
    Target *target = nullptr;
};

void ProjectExplorerPlugin::testProject_setup()
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

    QCOMPARE(project.files(Project::AllFiles), {TEST_PROJECT_PATH});
    QCOMPARE(project.files(Project::GeneratedFiles), {});

    QCOMPARE(project.id(), Id(TEST_PROJECT_ID));

    QVERIFY(!project.target->buildSystem()->isParsing());
    QVERIFY(!project.target->buildSystem()->hasParsingData());
}

void ProjectExplorerPlugin::testProject_changeDisplayName()
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

void ProjectExplorerPlugin::testProject_parsingSuccess()
{
    TestProject project;

    QSignalSpy startSpy(project.target->buildSystem(), &BuildSystem::parsingStarted);
    QSignalSpy stopSpy(project.target->buildSystem(), &BuildSystem::parsingFinished);

    {
        BuildSystem::ParseGuard guard = project.target->buildSystem()->guardParsingRun();
        QCOMPARE(startSpy.count(), 1);
        QCOMPARE(stopSpy.count(), 0);

        QVERIFY(project.target->buildSystem()->isParsing());
        QVERIFY(!project.target->buildSystem()->hasParsingData());

        guard.markAsSuccess();
    }

    QCOMPARE(startSpy.count(), 1);
    QCOMPARE(stopSpy.count(), 1);
    QCOMPARE(stopSpy.at(0), {QVariant(true)});

    QVERIFY(!project.target->buildSystem()->isParsing());
    QVERIFY(project.target->buildSystem()->hasParsingData());
}

void ProjectExplorerPlugin::testProject_parsingFail()
{
    TestProject project;

    QSignalSpy startSpy(project.target->buildSystem(), &BuildSystem::parsingStarted);
    QSignalSpy stopSpy(project.target->buildSystem(), &BuildSystem::parsingFinished);

    {
        BuildSystem::ParseGuard guard = project.target->buildSystem()->guardParsingRun();
        QCOMPARE(startSpy.count(), 1);
        QCOMPARE(stopSpy.count(), 0);

        QVERIFY(project.target->buildSystem()->isParsing());
        QVERIFY(!project.target->buildSystem()->hasParsingData());
    }

    QCOMPARE(startSpy.count(), 1);
    QCOMPARE(stopSpy.count(), 1);
    QCOMPARE(stopSpy.at(0), {QVariant(false)});

    QVERIFY(!project.target->buildSystem()->isParsing());
    QVERIFY(!project.target->buildSystem()->hasParsingData());
}

std::unique_ptr<ProjectNode> createFileTree(Project *project)
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

void ProjectExplorerPlugin::testProject_projectTree()
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

    QCOMPARE(project.files(Project::GeneratedFiles), {TEST_PROJECT_GENERATED_FILE});
    FilePaths sourceFiles = project.files(Project::SourceFiles);
    QCOMPARE(sourceFiles.count(), 2);
    QVERIFY(sourceFiles.contains(TEST_PROJECT_PATH));
    QVERIFY(sourceFiles.contains(TEST_PROJECT_CPP_FILE));

    project.setRootProjectNode(nullptr);
    QCOMPARE(fileSpy.count(), 2);
    QVERIFY(!project.rootProjectNode());
}

void ProjectExplorerPlugin::testProject_multipleBuildConfigs()
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
    QString error;
    const FilePath projectDir = FilePath::fromString(tempDir->path() + "/generic-project");
    FileUtils::copyRecursively(":/projectexplorer/testdata/generic-project",
                               projectDir, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    const QFileInfoList files = QDir(projectDir.toString()).entryInfoList(QDir::Files | QDir::Dirs);
    for (const QFileInfo &f : files)
        QFile(f.absoluteFilePath()).setPermissions(f.permissions() | QFile::WriteUser);
    const auto theProject = openProject(projectDir.pathAppended("generic-project.creator"));
    QVERIFY2(theProject, qPrintable(theProject.errorMessage()));
    theProject.project()->configureAsExampleProject(kit);
    QCOMPARE(theProject.project()->targets().size(), 1);
    Target * const target = theProject.project()->activeTarget();
    QVERIFY(target);
    QCOMPARE(target->buildConfigurations().size(), 6);
    SessionManager::setActiveBuildConfiguration(target, target->buildConfigurations().at(1),
                                                SetActive::Cascade);
    BuildSystem * const bs = theProject.project()->activeTarget()->buildSystem();
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

    QCOMPARE(SessionManager::startupProject(), theProject.project());
    QCOMPARE(ProjectTree::currentProject(), theProject.project());
    QVERIFY(EditorManager::openEditor(projectDir.pathAppended("main.cpp")));
    QVERIFY(ProjectTree::currentNode());
    ProjectTree::instance()->expandAll();
    SessionManager::closeAllProjects(); // QTCREATORBUG-25655
}

#endif // WITH_TESTS

} // namespace ProjectExplorer
