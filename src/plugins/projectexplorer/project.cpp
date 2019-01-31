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

#include "buildinfo.h"
#include "buildconfiguration.h"
#include "deployconfiguration.h"
#include "editorconfiguration.h"
#include "kit.h"
#include "projectexplorer.h"
#include "projectnodes.h"
#include "target.h"
#include "session.h"
#include "userfileaccessor.h"

#include <coreplugin/idocument.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projecttree.h>

#include <utils/pointeralgorithm.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <limits>
#include <memory>

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

namespace {
const char ACTIVE_TARGET_KEY[] = "ProjectExplorer.Project.ActiveTarget";
const char TARGET_KEY_PREFIX[] = "ProjectExplorer.Project.Target.";
const char TARGET_COUNT_KEY[] = "ProjectExplorer.Project.TargetCount";
const char EDITOR_SETTINGS_KEY[] = "ProjectExplorer.Project.EditorSettings";
const char PLUGIN_SETTINGS_KEY[] = "ProjectExplorer.Project.PluginSettings";
} // namespace

namespace ProjectExplorer {

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

ProjectDocument::ProjectDocument(const QString &mimeType, const Utils::FileName &fileName,
                                 const ProjectDocument::ProjectCallback &callback) :
    m_callback(callback)
{
    setFilePath(fileName);
    setMimeType(mimeType);
    if (m_callback)
        Core::DocumentManager::addDocument(this);
}

Core::IDocument::ReloadBehavior
ProjectDocument::reloadBehavior(Core::IDocument::ChangeTrigger state,
                                Core::IDocument::ChangeType type) const
{
    Q_UNUSED(state);
    Q_UNUSED(type);
    return BehaviorSilent;
}

bool ProjectDocument::reload(QString *errorString, Core::IDocument::ReloadFlag flag,
                             Core::IDocument::ChangeType type)
{
    Q_UNUSED(errorString);
    Q_UNUSED(flag);
    Q_UNUSED(type);

    if (m_callback)
        m_callback();
    return true;
}

// -------------------------------------------------------------------------
// Project
// -------------------------------------------------------------------------
class ProjectPrivate
{
public:
    ProjectPrivate(const QString &mimeType, const Utils::FileName &fileName,
                   const ProjectDocument::ProjectCallback &callback)
    {
        m_document = std::make_unique<ProjectDocument>(mimeType, fileName, callback);
    }

    ~ProjectPrivate();

    Core::Id m_id;
    bool m_isParsing = false;
    bool m_hasParsingData = false;
    bool m_needsInitialExpansion = false;
    std::unique_ptr<Core::IDocument> m_document;
    std::unique_ptr<ProjectNode> m_rootProjectNode;
    std::unique_ptr<ContainerNode> m_containerNode;
    std::vector<std::unique_ptr<Target>> m_targets;
    Target *m_activeTarget = nullptr;
    EditorConfiguration m_editorConfiguration;
    Core::Context m_projectLanguages;
    QVariantMap m_pluginSettings;
    std::unique_ptr<Internal::UserFileAccessor> m_accessor;

    QString m_displayName;

    Kit::Predicate m_requiredKitPredicate;
    Kit::Predicate m_preferredKitPredicate;

    Utils::MacroExpander m_macroExpander;
    mutable QVector<const Node *> m_sortedNodeList;
};

ProjectPrivate::~ProjectPrivate()
{
    // Make sure our root node is null when deleting the actual node
    std::unique_ptr<ProjectNode> oldNode = std::move(m_rootProjectNode);
}

Project::Project(const QString &mimeType, const Utils::FileName &fileName,
                 const ProjectDocument::ProjectCallback &callback) :
    d(new ProjectPrivate(mimeType, fileName, callback))
{
    d->m_macroExpander.setDisplayName(tr("Project"));
    d->m_macroExpander.registerVariable("Project:Name", tr("Project Name"),
            [this] { return displayName(); });

    // Only set up containernode after d is set so that it will find the project directory!
    d->m_containerNode = std::make_unique<ContainerNode>(this);

    setRequiredKitPredicate([this](const Kit *k) {
        return !containsType(projectIssues(k), Task::TaskType::Error);
    });
}

Project::~Project()
{
    delete d;
}

QString Project::displayName() const
{
    return d->m_displayName;
}

Core::Id Project::id() const
{
    QTC_CHECK(d->m_id.isValid());
    return d->m_id;
}

QString Project::mimeType() const
{
    return document()->mimeType();
}

Core::IDocument *Project::document() const
{
    QTC_CHECK(d->m_document);
    return d->m_document.get();
}

Utils::FileName Project::projectFilePath() const
{
    QTC_ASSERT(document(), return Utils::FileName());
    return document()->filePath();
}

bool Project::hasActiveBuildSettings() const
{
    return activeTarget() && BuildConfigurationFactory::find(activeTarget());
}

void Project::addTarget(std::unique_ptr<Target> &&t)
{
    auto pointer = t.get();
    QTC_ASSERT(t && !Utils::contains(d->m_targets, pointer), return);
    QTC_ASSERT(!target(t->kit()), return);
    Q_ASSERT(t->project() == this);

    t->setDefaultDisplayName(t->displayName());

    // add it
    d->m_targets.emplace_back(std::move(t));
    connect(pointer, &Target::addedProjectConfiguration, this, &Project::addedProjectConfiguration);
    connect(pointer, &Target::aboutToRemoveProjectConfiguration, this, &Project::aboutToRemoveProjectConfiguration);
    connect(pointer, &Target::removedProjectConfiguration, this, &Project::removedProjectConfiguration);
    connect(pointer, &Target::activeProjectConfigurationChanged, this, &Project::activeProjectConfigurationChanged);
    emit addedProjectConfiguration(pointer);
    emit addedTarget(pointer);

    // check activeTarget:
    if (!activeTarget())
        SessionManager::setActiveTarget(this, pointer, SetActive::Cascade);
}

bool Project::removeTarget(Target *target)
{
    QTC_ASSERT(target && Utils::contains(d->m_targets, target), return false);

    if (BuildManager::isBuilding(target))
        return false;

    emit aboutToRemoveProjectConfiguration(target);
    emit aboutToRemoveTarget(target);
    auto keep = Utils::take(d->m_targets, target);
    if (target == d->m_activeTarget) {
        Target *newActiveTarget = (d->m_targets.size() == 0 ? nullptr : d->m_targets.at(0).get());
        SessionManager::setActiveTarget(this, newActiveTarget, SetActive::Cascade);
    }
    emit removedTarget(target);
    emit removedProjectConfiguration(target);

    return true;
}

QList<Target *> Project::targets() const
{
    return Utils::toRawPointer<QList>(d->m_targets);
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
        (target && Utils::contains(d->m_targets, target))) {
        d->m_activeTarget = target;
        emit activeProjectConfigurationChanged(d->m_activeTarget);
        emit activeTargetChanged(d->m_activeTarget);
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

Target *Project::target(Core::Id id) const
{
    return Utils::findOrDefault(d->m_targets, Utils::equal(&Target::id, id));
}

Target *Project::target(Kit *k) const
{
    return Utils::findOrDefault(d->m_targets, Utils::equal(&Target::kit, k));
}

QList<Task> Project::projectIssues(const Kit *k) const
{
    QList<Task> result;
    if (!k->isValid())
        result.append(createProjectTask(Task::TaskType::Error, tr("Kit is not valid.")));
    return {};
}

std::unique_ptr<Target> Project::createTarget(Kit *k)
{
    if (!k || target(k))
        return nullptr;

    auto t = std::make_unique<Target>(this, k, Target::_constructor_tag{});
    if (!setupTarget(t.get()))
        return {};
    return t;
}

bool Project::copySteps(Target *sourceTarget, Target *newTarget)
{
    QTC_ASSERT(newTarget, return false);
    bool fatalError = false;
    QStringList buildconfigurationError;
    QStringList deployconfigurationError;
    QStringList runconfigurationError;

    foreach (BuildConfiguration *sourceBc, sourceTarget->buildConfigurations()) {
        BuildConfiguration *newBc = BuildConfigurationFactory::clone(newTarget, sourceBc);
        if (!newBc) {
            buildconfigurationError << sourceBc->displayName();
            continue;
        }
        newBc->setDisplayName(sourceBc->displayName());
        newTarget->addBuildConfiguration(newBc);
        if (sourceTarget->activeBuildConfiguration() == sourceBc)
            SessionManager::setActiveBuildConfiguration(newTarget, newBc, SetActive::NoCascade);
    }
    if (!newTarget->activeBuildConfiguration()) {
        QList<BuildConfiguration *> bcs = newTarget->buildConfigurations();
        if (!bcs.isEmpty())
            SessionManager::setActiveBuildConfiguration(newTarget, bcs.first(), SetActive::NoCascade);
    }

    foreach (DeployConfiguration *sourceDc, sourceTarget->deployConfigurations()) {
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

    foreach (RunConfiguration *sourceRc, sourceTarget->runConfigurations()) {
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
        QMessageBox::critical(Core::ICore::mainWindow(),
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

        QMessageBox msgBox(Core::ICore::mainWindow());
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
    if (needsBuildConfigurations())
        t->updateDefaultBuildConfigurations();
    t->updateDefaultDeployConfigurations();
    t->updateDefaultRunConfigurations();
    return true;
}

void Project::emitParsingStarted()
{
    QTC_ASSERT(!d->m_isParsing, return);

    d->m_isParsing = true;
    d->m_hasParsingData = false;
    emit parsingStarted();
}

void Project::emitParsingFinished(bool success)
{
    QTC_ASSERT(d->m_isParsing, return);

    d->m_isParsing = false;
    d->m_hasParsingData = success;
    emit parsingFinished(success);
}

void Project::setDisplayName(const QString &name)
{
    if (name == d->m_displayName)
        return;
    d->m_displayName = name;
    emit displayNameChanged();
}

void Project::setId(Core::Id id)
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
        ProjectTree::applyTreeManager(root.get());
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
        Utils::sort(nodeList, &nodeLessThan);
    }
    d->m_sortedNodeList = nodeList;

    ProjectTree::emitSubtreeChanged(node);
    emit fileListChanged();
}

std::unique_ptr<Target> Project::restoreTarget(const QVariantMap &data)
{
    Core::Id id = idFromMap(data);
    if (target(id)) {
        qWarning("Warning: Duplicated target id found, not restoring second target with id '%s'. Continuing.",
                 qPrintable(id.toString()));
        return nullptr;
    }

    Kit *k = KitManager::kit(id);
    if (!k) {
        qWarning("Warning: No kit '%s' found. Continuing.", qPrintable(id.toString()));
        return nullptr;
    }

    auto t = std::make_unique<Target>(this, k, Target::_constructor_tag{});
    if (!t->fromMap(data))
        return {};

    return t;
}

void Project::saveSettings()
{
    emit aboutToSaveSettings();
    if (!d->m_accessor)
        d->m_accessor = std::make_unique<Internal::UserFileAccessor>(this);
    if (!targets().isEmpty())
        d->m_accessor->saveSettings(toMap(), Core::ICore::mainWindow());
}

Project::RestoreResult Project::restoreSettings(QString *errorMessage)
{
    if (!d->m_accessor)
        d->m_accessor = std::make_unique<Internal::UserFileAccessor>(this);
    QVariantMap map(d->m_accessor->restoreSettings(Core::ICore::mainWindow()));
    RestoreResult result = fromMap(map, errorMessage);
    if (result == RestoreResult::Ok)
        emit settingsLoaded();
    return result;
}

/*!
 * Returns a sorted list of all files matching the predicate \a filter.
 */
Utils::FileNameList Project::files(const Project::NodeMatcher &filter) const
{
    Utils::FileNameList result;
    if (d->m_sortedNodeList.empty() && filter(containerNode()))
        result.append(projectFilePath());

    Utils::FileName lastAdded;
    for (const Node *n : qAsConst(d->m_sortedNodeList)) {
        if (filter && !filter(n))
            continue;

        // Remove duplicates:
        const Utils::FileName path = n->filePath();
        if (path == lastAdded)
            continue; // skip duplicates
        lastAdded = path;

        result.append(path);
    };
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
    map.insert(QLatin1String(PLUGIN_SETTINGS_KEY), d->m_pluginSettings);

    return map;
}

/*!
    Returns the directory that contains the project.

    This includes the absolute path.
*/

Utils::FileName Project::projectDirectory() const
{
    return projectDirectory(projectFilePath());
}

/*!
    Returns the directory that contains the file \a top.

    This includes the absolute path.
*/

Utils::FileName Project::projectDirectory(const Utils::FileName &top)
{
    if (top.isEmpty())
        return Utils::FileName();
    return Utils::FileName::fromString(top.toFileInfo().absoluteDir().path());
}

/*!
    Returns the common root directory that contains all files which belongs to a project.
*/

Utils::FileName Project::rootProjectDirectory() const
{
    return projectDirectory(); // TODO parse all files and find the common path
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
    Q_UNUSED(errorMessage);
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

    return RestoreResult::Ok;
}

void Project::createTargetFromMap(const QVariantMap &map, int index)
{
    const QString key = QString::fromLatin1(TARGET_KEY_PREFIX) + QString::number(index);
    if (!map.contains(key))
        return;
    QVariantMap targetMap = map.value(key).toMap();

    std::unique_ptr<Target> t = restoreTarget(targetMap);
    if (!t || (t->runConfigurations().isEmpty() && t->buildConfigurations().isEmpty()))
        return;

    addTarget(std::move(t));
}

EditorConfiguration *Project::editorConfiguration() const
{
    return &d->m_editorConfiguration;
}

QStringList Project::filesGeneratedFrom(const QString &file) const
{
    Q_UNUSED(file);
    return QStringList();
}

bool Project::isKnownFile(const Utils::FileName &filename) const
{
    if (d->m_sortedNodeList.empty())
        return filename == projectFilePath();
    const FileNode element(filename, FileType::Unknown, false);
    return std::binary_search(std::begin(d->m_sortedNodeList), std::end(d->m_sortedNodeList),
                              &element, nodeLessThan);
}

void Project::setProjectLanguages(Core::Context language)
{
    if (d->m_projectLanguages == language)
        return;
    d->m_projectLanguages = language;
    emit projectLanguagesUpdated();
}

void Project::addProjectLanguage(Core::Id id)
{
    Core::Context lang = projectLanguages();
    int pos = lang.indexOf(id);
    if (pos < 0)
        lang.add(id);
    setProjectLanguages(lang);
}

void Project::removeProjectLanguage(Core::Id id)
{
    Core::Context lang = projectLanguages();
    int pos = lang.indexOf(id);
    if (pos >= 0)
        lang.removeAt(pos);
    setProjectLanguages(lang);
}

void Project::setProjectLanguage(Core::Id id, bool enabled)
{
    if (enabled)
        addProjectLanguage(id);
    else
        removeProjectLanguage(id);
}

void Project::projectLoaded()
{
}

Task Project::createProjectTask(Task::TaskType type, const QString &description)
{
    return Task(type, description, Utils::FileName(), -1, Core::Id());
}

Core::Context Project::projectContext() const
{
    return Core::Context(d->m_id);
}

Core::Context Project::projectLanguages() const
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

bool Project::needsConfiguration() const
{
    return d->m_targets.size() == 0;
}

bool Project::needsBuildConfigurations() const
{
    return true;
}

void Project::configureAsExampleProject(const QSet<Core::Id> &platforms)
{
    Q_UNUSED(platforms);
}

bool Project::knowsAllBuildExecutables() const
{
    return true;
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
            t = Utils::findOrDefault(toRegister, Utils::equal(&Target::kit, k));
        if (!t) {
            auto newTarget = std::make_unique<Target>(this, k, Target::_constructor_tag{});
            t = newTarget.get();
            toRegister.emplace_back(std::move(newTarget));
        }

        if (!info.factory())
            continue;

        if (BuildConfiguration *bc = info.factory()->create(t, info))
            t->addBuildConfiguration(bc);
    }
    for (std::unique_ptr<Target> &t : toRegister) {
        t->updateDefaultDeployConfigurations();
        t->updateDefaultRunConfigurations();
        addTarget(std::move(t));
    }
}

Utils::MacroExpander *Project::macroExpander() const
{
    return &d->m_macroExpander;
}

QVariant Project::additionalData(Core::Id id, const Target *target) const
{
    Q_UNUSED(id);
    Q_UNUSED(target);
    return QVariant();
}

bool Project::isParsing() const
{
    return d->m_isParsing;
}

bool Project::hasParsingData() const
{
    return d->m_hasParsingData;
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

Kit::Predicate Project::requiredKitPredicate() const
{
    return d->m_requiredKitPredicate;
}

void Project::setRequiredKitPredicate(const Kit::Predicate &predicate)
{
    d->m_requiredKitPredicate = predicate;
}

Kit::Predicate Project::preferredKitPredicate() const
{
    return d->m_preferredKitPredicate;
}

void Project::setPreferredKitPredicate(const Kit::Predicate &predicate)
{
    d->m_preferredKitPredicate = predicate;
}

#if defined(WITH_TESTS)

} // namespace ProjectExplorer

#include <utils/hostosinfo.h>

#include <QTest>
#include <QSignalSpy>

namespace ProjectExplorer {

static Utils::FileName constructTestPath(const char *basePath)
{
    Utils::FileName drive;
    if (Utils::HostOsInfo::isWindowsHost())
        drive = Utils::FileName::fromString("C:");
    return drive + QLatin1String(basePath);
}

const Utils::FileName TEST_PROJECT_PATH = constructTestPath("/tmp/foobar/baz.project");
const Utils::FileName TEST_PROJECT_NONEXISTING_FILE = constructTestPath("/tmp/foobar/nothing.cpp");
const Utils::FileName TEST_PROJECT_CPP_FILE = constructTestPath("/tmp/foobar/main.cpp");
const Utils::FileName TEST_PROJECT_GENERATED_FILE = constructTestPath("/tmp/foobar/generated.foo");
const QString TEST_PROJECT_MIMETYPE = "application/vnd.test.qmakeprofile";
const QString TEST_PROJECT_DISPLAYNAME = "testProjectFoo";
const char TEST_PROJECT_ID[] = "Test.Project.Id";

class TestProject : public Project
{
public:
    TestProject() : Project(TEST_PROJECT_MIMETYPE, TEST_PROJECT_PATH)
    {
        setId(TEST_PROJECT_ID);
        setDisplayName(TEST_PROJECT_DISPLAYNAME);
    }

    void testStartParsing()
    {
        emitParsingStarted();
    }

    void testParsingFinished(bool success) {
        emitParsingFinished(success);
    }

    bool needsConfiguration() const final { return false; }
};

class TestProjectNode : public ProjectNode
{
public:
    TestProjectNode(const Utils::FileName &dir) : ProjectNode(dir) { }
};

void ProjectExplorerPlugin::testProject_setup()
{
    TestProject project;

    QCOMPARE(project.displayName(), TEST_PROJECT_DISPLAYNAME);

    QVERIFY(!project.rootProjectNode());
    QVERIFY(project.containerNode());

    QVERIFY(project.macroExpander());

    QVERIFY(project.document());
    QCOMPARE(project.document()->filePath(), TEST_PROJECT_PATH);
    QCOMPARE(project.document()->mimeType(), TEST_PROJECT_MIMETYPE);

    QCOMPARE(project.mimeType(), TEST_PROJECT_MIMETYPE);
    QCOMPARE(project.projectFilePath(), TEST_PROJECT_PATH);
    QCOMPARE(project.projectDirectory(), TEST_PROJECT_PATH.parentDir());

    QCOMPARE(project.isKnownFile(TEST_PROJECT_PATH), true);
    QCOMPARE(project.isKnownFile(TEST_PROJECT_NONEXISTING_FILE), false);
    QCOMPARE(project.isKnownFile(TEST_PROJECT_CPP_FILE), false);

    QCOMPARE(project.files(Project::AllFiles), {TEST_PROJECT_PATH});
    QCOMPARE(project.files(Project::GeneratedFiles), {});

    QCOMPARE(project.id(), Core::Id(TEST_PROJECT_ID));

    QVERIFY(!project.isParsing());
    QVERIFY(!project.hasParsingData());
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

    QSignalSpy startSpy(&project, &Project::parsingStarted);
    QSignalSpy stopSpy(&project, &Project::parsingFinished);

    project.testStartParsing();
    QCOMPARE(startSpy.count(), 1);
    QCOMPARE(stopSpy.count(), 0);

    QVERIFY(project.isParsing());
    QVERIFY(!project.hasParsingData());

    project.testParsingFinished(true);
    QCOMPARE(startSpy.count(), 1);
    QCOMPARE(stopSpy.count(), 1);
    QCOMPARE(stopSpy.at(0), {QVariant(true)});

    QVERIFY(!project.isParsing());
    QVERIFY(project.hasParsingData());
}

void ProjectExplorerPlugin::testProject_parsingFail()
{
    TestProject project;

    QSignalSpy startSpy(&project, &Project::parsingStarted);
    QSignalSpy stopSpy(&project, &Project::parsingFinished);

    project.testStartParsing();
    QCOMPARE(startSpy.count(), 1);
    QCOMPARE(stopSpy.count(), 0);

    QVERIFY(project.isParsing());
    QVERIFY(!project.hasParsingData());

    project.testParsingFinished(false);
    QCOMPARE(startSpy.count(), 1);
    QCOMPARE(stopSpy.count(), 1);
    QCOMPARE(stopSpy.at(0), {QVariant(false)});

    QVERIFY(!project.isParsing());
    QVERIFY(!project.hasParsingData());
}

std::unique_ptr<ProjectNode> createFileTree(Project *project)
{
    std::unique_ptr<ProjectNode> root = std::make_unique<TestProjectNode>(project->projectDirectory());
    std::vector<std::unique_ptr<FileNode>> nodes;
    nodes.emplace_back(std::make_unique<FileNode>(TEST_PROJECT_PATH, FileType::Project, false));
    nodes.emplace_back(std::make_unique<FileNode>(TEST_PROJECT_CPP_FILE, FileType::Source, false));
    nodes.emplace_back(std::make_unique<FileNode>(TEST_PROJECT_GENERATED_FILE, FileType::Source, true));
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

    project.setRootProjectNode(std::make_unique<TestProjectNode>(project.projectDirectory()));
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

    Utils::FileNameList allFiles = project.files(Project::AllFiles);
    QCOMPARE(allFiles.count(), 3);
    QVERIFY(allFiles.contains(TEST_PROJECT_PATH));
    QVERIFY(allFiles.contains(TEST_PROJECT_CPP_FILE));
    QVERIFY(allFiles.contains(TEST_PROJECT_GENERATED_FILE));

    QCOMPARE(project.files(Project::GeneratedFiles), {TEST_PROJECT_GENERATED_FILE});
    Utils::FileNameList sourceFiles = project.files(Project::SourceFiles);
    QCOMPARE(sourceFiles.count(), 2);
    QVERIFY(sourceFiles.contains(TEST_PROJECT_PATH));
    QVERIFY(sourceFiles.contains(TEST_PROJECT_CPP_FILE));

    project.setRootProjectNode(nullptr);
    QCOMPARE(fileSpy.count(), 2);
    QVERIFY(!project.rootProjectNode());
}

#endif // WITH_TESTS

} // namespace ProjectExplorer
