// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectmanager.h"

#include "buildconfiguration.h"
#include "editorconfiguration.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projectnodes.h"
#include "target.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/foldernavigationwidget.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/session.h>

#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <QDebug>
#include <QMessageBox>
#include <QPushButton>

#ifdef WITH_TESTS
#include <QTemporaryFile>
#include <QTest>
#include <vector>
#endif

using namespace Core;
using namespace Utils;
using namespace ProjectExplorer::Internal;

namespace ProjectExplorer {

class ProjectManagerPrivate
{
public:
    void loadSession();
    void saveSession();
    void restoreDependencies();
    void restoreStartupProject();
    void restoreProjects(const FilePaths &fileList);
    void askUserAboutFailedProjects();

    bool recursiveDependencyCheck(const FilePath &newDep, const FilePath &checkDep) const;
    FilePaths dependencies(const FilePath &proName) const;
    FilePaths dependenciesOrder() const;
    void dependencies(const FilePath &proName, FilePaths &result) const;

    static QString windowTitleAddition(const FilePath &filePath);
    static QString sessionTitle(const FilePath &filePath);

    bool hasProjects() const { return !m_projects.isEmpty(); }

    bool m_casadeSetActive = false;

    Project *m_startupProject = nullptr;
    QList<Project *> m_projects;
    FilePaths m_failedProjects;
    QMap<FilePath, FilePaths> m_depMap;

private:
    static QString locationInProject(const FilePath &filePath);
};

static ProjectManager *m_instance = nullptr;
static ProjectManagerPrivate *d = nullptr;

static QString projectFolderId(Project *pro)
{
    return pro->projectFilePath().toString();
}

const int PROJECT_SORT_VALUE = 100;

ProjectManager::ProjectManager()
{
    m_instance = this;
    d = new ProjectManagerPrivate;

    connect(EditorManager::instance(), &EditorManager::editorCreated,
            this, &ProjectManager::configureEditor);
    connect(this, &ProjectManager::projectAdded,
            EditorManager::instance(), &EditorManager::updateWindowTitles);
    connect(this, &ProjectManager::projectRemoved,
            EditorManager::instance(), &EditorManager::updateWindowTitles);
    connect(this, &ProjectManager::projectDisplayNameChanged,
            EditorManager::instance(), &EditorManager::updateWindowTitles);

    EditorManager::setWindowTitleAdditionHandler(&ProjectManagerPrivate::windowTitleAddition);
    EditorManager::setSessionTitleHandler(&ProjectManagerPrivate::sessionTitle);

    connect(SessionManager::instance(), &SessionManager::aboutToLoadSession, this, [] {
        d->loadSession();
    });
    connect(SessionManager::instance(), &SessionManager::aboutToSaveSession, this, [] {
        d->saveSession();
    });
}

ProjectManager::~ProjectManager()
{
    EditorManager::setWindowTitleAdditionHandler({});
    EditorManager::setSessionTitleHandler({});
    delete d;
    d = nullptr;
}

ProjectManager *ProjectManager::instance()
{
   return m_instance;
}

bool ProjectManagerPrivate::recursiveDependencyCheck(const FilePath &newDep,
                                                     const FilePath &checkDep) const
{
    if (newDep == checkDep)
        return false;

    const FilePaths depList = m_depMap.value(checkDep);
    for (const FilePath &dependency : depList) {
        if (!recursiveDependencyCheck(newDep, dependency))
            return false;
    }

    return true;
}

/*
 * The dependency management exposes an interface based on projects, but
 * is internally purely string based. This is suboptimal. Probably it would be
 * nicer to map the filenames to projects on load and only map it back to
 * filenames when saving.
 */

QList<Project *> ProjectManager::dependencies(const Project *project)
{
    const FilePath proName = project->projectFilePath();
    const FilePaths proDeps = d->m_depMap.value(proName);

    QList<Project *> projects;
    for (const FilePath &dep : proDeps) {
        Project *pro = Utils::findOrDefault(d->m_projects, [&dep](Project *p) {
            return p->projectFilePath() == dep;
        });
        if (pro)
            projects += pro;
    }

    return projects;
}

bool ProjectManager::hasDependency(const Project *project, const Project *depProject)
{
    const FilePath proName = project->projectFilePath();
    const FilePath depName = depProject->projectFilePath();

    const FilePaths proDeps = d->m_depMap.value(proName);
    return proDeps.contains(depName);
}

bool ProjectManager::canAddDependency(const Project *project, const Project *depProject)
{
    const FilePath newDep = project->projectFilePath();
    const FilePath checkDep = depProject->projectFilePath();

    return d->recursiveDependencyCheck(newDep, checkDep);
}

bool ProjectManager::addDependency(Project *project, Project *depProject)
{
    const FilePath proName = project->projectFilePath();
    const FilePath depName = depProject->projectFilePath();

    // check if this dependency is valid
    if (!d->recursiveDependencyCheck(proName, depName))
        return false;

    FilePaths proDeps = d->m_depMap.value(proName);
    if (!proDeps.contains(depName)) {
        proDeps.append(depName);
        d->m_depMap[proName] = proDeps;
    }
    emit m_instance->dependencyChanged(project, depProject);

    return true;
}

void ProjectManager::removeDependency(Project *project, Project *depProject)
{
    const FilePath proName = project->projectFilePath();
    const FilePath depName = depProject->projectFilePath();

    FilePaths proDeps = d->m_depMap.value(proName);
    proDeps.removeAll(depName);
    if (proDeps.isEmpty())
        d->m_depMap.remove(proName);
    else
        d->m_depMap[proName] = proDeps;
    emit m_instance->dependencyChanged(project, depProject);
}

bool ProjectManager::isProjectConfigurationCascading()
{
    return d->m_casadeSetActive;
}

void ProjectManager::setProjectConfigurationCascading(bool b)
{
    d->m_casadeSetActive = b;
    SessionManager::markSessionFileDirty();
}

void ProjectManager::setStartupProject(Project *startupProject)
{
    QTC_ASSERT((!startupProject && d->m_projects.isEmpty())
               || (startupProject && d->m_projects.contains(startupProject)), return);

    if (d->m_startupProject == startupProject)
        return;

    d->m_startupProject = startupProject;
    if (d->m_startupProject && d->m_startupProject->needsConfiguration()) {
        ModeManager::activateMode(Constants::MODE_SESSION);
        ModeManager::setFocusToCurrentMode();
    }
    FolderNavigationWidgetFactory::setFallbackSyncFilePath(
        startupProject ? startupProject->projectFilePath().parentDir() : FilePath());
    emit m_instance->startupProjectChanged(startupProject);
}

Project *ProjectManager::startupProject()
{
    return d->m_startupProject;
}

Target *ProjectManager::startupTarget()
{
    return d->m_startupProject ? d->m_startupProject->activeTarget() : nullptr;
}

BuildSystem *ProjectManager::startupBuildSystem()
{
    Target *t = startupTarget();
    return t ? t->buildSystem() : nullptr;
}

/*!
 * Returns the RunConfiguration of the currently active target
 * of the startup project, if such exists, or \c nullptr otherwise.
 */


RunConfiguration *ProjectManager::startupRunConfiguration()
{
    Target *t = startupTarget();
    return t ? t->activeRunConfiguration() : nullptr;
}

void ProjectManager::addProject(Project *pro)
{
    QTC_ASSERT(pro, return);
    QTC_CHECK(!pro->displayName().isEmpty());
    QTC_CHECK(pro->id().isValid());

    SessionManager::markSessionFileDirty();
    QTC_ASSERT(!d->m_projects.contains(pro), return);

    d->m_projects.append(pro);

    connect(pro, &Project::displayNameChanged,
            m_instance, [pro]() { emit m_instance->projectDisplayNameChanged(pro); });

    emit m_instance->projectAdded(pro);
    const auto updateFolderNavigation = [pro] {
        // destructing projects might trigger changes, so check if the project is actually there
        if (QTC_GUARD(d->m_projects.contains(pro))) {
            const QIcon icon = pro->rootProjectNode() ? pro->rootProjectNode()->icon() : QIcon();
            FolderNavigationWidgetFactory::insertRootDirectory({projectFolderId(pro),
                                                                PROJECT_SORT_VALUE,
                                                                pro->displayName(),
                                                                pro->projectFilePath().parentDir(),
                                                                icon});
        }
    };
    updateFolderNavigation();
    configureEditors(pro);
    connect(pro, &Project::fileListChanged, m_instance, [pro, updateFolderNavigation]() {
        configureEditors(pro);
        updateFolderNavigation(); // update icon
    });
    connect(pro, &Project::displayNameChanged, m_instance, updateFolderNavigation);

    if (!startupProject())
        setStartupProject(pro);
}

void ProjectManager::removeProject(Project *project)
{
    SessionManager::markSessionFileDirty();
    QTC_ASSERT(project, return);
    removeProjects({project});
}

void ProjectManagerPrivate::saveSession()
{
    // save the startup project
    if (d->m_startupProject)
        SessionManager::setSessionValue("StartupProject",
                                        m_startupProject->projectFilePath().toSettings());

    FilePaths projectFiles = Utils::transform(m_projects, &Project::projectFilePath);
    // Restore information on projects that failed to load:
    // don't read projects to the list, which the user loaded
    for (const FilePath &failed : std::as_const(m_failedProjects)) {
        if (!projectFiles.contains(failed))
            projectFiles << failed;
    }

    SessionManager::setSessionValue("ProjectList",
                                    Utils::transform<QStringList>(projectFiles,
                                                                  &FilePath::toString));
    SessionManager::setSessionValue("CascadeSetActive", m_casadeSetActive);

    QVariantMap depMap;
    auto i = m_depMap.constBegin();
    while (i != m_depMap.constEnd()) {
        QString key = i.key().toString();
        QStringList values;
        const FilePaths valueList = i.value();
        for (const FilePath &value : valueList)
            values << value.toString();
        depMap.insert(key, values);
        ++i;
    }
    SessionManager::setSessionValue("ProjectDependencies", QVariant(depMap));
}

/*!
  Closes all projects
  */
void ProjectManager::closeAllProjects()
{
    removeProjects(projects());
}

const QList<Project *> ProjectManager::projects()
{
    return d->m_projects;
}

bool ProjectManager::hasProjects()
{
    return d->hasProjects();
}

bool ProjectManager::hasProject(Project *p)
{
    return d->m_projects.contains(p);
}

FilePaths ProjectManagerPrivate::dependencies(const FilePath &proName) const
{
    FilePaths result;
    dependencies(proName, result);
    return result;
}

void ProjectManagerPrivate::dependencies(const FilePath &proName, FilePaths &result) const
{
    const FilePaths depends = m_depMap.value(proName);

    for (const FilePath &dep : depends)
        dependencies(dep, result);

    if (!result.contains(proName))
        result.append(proName);
}

QString ProjectManagerPrivate::sessionTitle(const FilePath &filePath)
{
    const QString sessionName = SessionManager::activeSession();
    if (SessionManager::isDefaultSession(sessionName)) {
        if (filePath.isEmpty()) {
            // use single project's name if there is only one loaded.
            const QList<Project *> projects = ProjectManager::projects();
            if (projects.size() == 1)
                return projects.first()->displayName();
        }
    } else {
        return sessionName.isEmpty() ? Tr::tr("Untitled") : sessionName;
    }
    return {};
}

QString ProjectManagerPrivate::locationInProject(const FilePath &filePath)
{
    const Project *project = ProjectManager::projectForFile(filePath);
    if (!project)
        return {};

    const FilePath parentDir = filePath.parentDir();
    if (parentDir == project->projectDirectory())
        return "@ " + project->displayName();

    if (filePath.isChildOf(project->projectDirectory())) {
        const FilePath dirInProject = parentDir.relativeChildPath(project->projectDirectory());
        return "(" + dirInProject.toUserOutput() + " @ " + project->displayName() + ")";
    }

    // For a file that is "outside" the project it belongs to, we display its
    // dir's full path because it is easier to read than a series of  "../../.".
    // Example: /home/hugo/GenericProject/App.files lists /home/hugo/lib/Bar.cpp
   return "(" + parentDir.toUserOutput() + " @ " + project->displayName() + ")";
}

QString ProjectManagerPrivate::windowTitleAddition(const FilePath &filePath)
{
    return filePath.isEmpty() ? QString() : locationInProject(filePath);
}

FilePaths ProjectManagerPrivate::dependenciesOrder() const
{
    QList<QPair<FilePath, FilePaths>> unordered;
    FilePaths ordered;

    // copy the map to a temporary list
    for (const Project *pro : m_projects) {
        const FilePath proName = pro->projectFilePath();
        const FilePaths depList = filtered(m_depMap.value(proName),
                                             [this](const FilePath &proPath) {
            return contains(m_projects, [proPath](const Project *p) {
                return p->projectFilePath() == proPath;
            });
        });
        unordered.push_back({proName, depList});
    }

    while (!unordered.isEmpty()) {
        for (int i = (unordered.count() - 1); i >= 0; --i) {
            if (unordered.at(i).second.isEmpty()) {
                ordered << unordered.at(i).first;
                unordered.removeAt(i);
            }
        }

        // remove the handled projects from the dependency lists
        // of the remaining unordered projects
        for (int i = 0; i < unordered.count(); ++i) {
            for (const FilePath &pro : std::as_const(ordered)) {
                FilePaths depList = unordered.at(i).second;
                depList.removeAll(pro);
                unordered[i].second = depList;
            }
        }
    }

    return ordered;
}

QList<Project *> ProjectManager::projectOrder(const Project *project)
{
    QList<Project *> result;

    FilePaths pros;
    if (project)
        pros = d->dependencies(project->projectFilePath());
    else
        pros = d->dependenciesOrder();

    for (const FilePath &proFile : std::as_const(pros)) {
        for (Project *pro : projects()) {
            if (pro->projectFilePath() == proFile) {
                result << pro;
                break;
            }
        }
    }

    return result;
}

Project *ProjectManager::projectForFile(const FilePath &fileName)
{
    if (Project * const project = Utils::findOrDefault(ProjectManager::projects(),
            [&fileName](const Project *p) { return p->isKnownFile(fileName); })) {
        return project;
    }
    return Utils::findOrDefault(ProjectManager::projects(),
                                [&fileName](const Project *p) {
        for (const Target * const target : p->targets()) {
            for (const BuildConfiguration * const bc : target->buildConfigurations()) {
                if (fileName.isChildOf(bc->buildDirectory()))
                    return false;
            }
        }
        return fileName.isChildOf(p->projectDirectory());
    });
}

Project *ProjectManager::projectWithProjectFilePath(const FilePath &filePath)
{
    return Utils::findOrDefault(ProjectManager::projects(),
            [&filePath](const Project *p) { return p->projectFilePath() == filePath; });
}

void ProjectManager::configureEditor(IEditor *editor, const FilePath &filePath)
{
    if (auto textEditor = qobject_cast<TextEditor::BaseTextEditor*>(editor)) {
        // Global settings are the default.
        if (Project *project = projectForFile(filePath))
            project->editorConfiguration()->configureEditor(textEditor);
    }
}

void ProjectManager::configureEditors(Project *project)
{
    const QList<IDocument *> documents = DocumentModel::openedDocuments();
    for (IDocument *document : documents) {
        if (project->isKnownFile(document->filePath())) {
            const QList<IEditor *> editors = DocumentModel::editorsForDocument(document);
            for (IEditor *editor : editors) {
                if (auto textEditor = qobject_cast<TextEditor::BaseTextEditor*>(editor)) {
                        project->editorConfiguration()->configureEditor(textEditor);
                }
            }
        }
    }
}

void ProjectManager::removeProjects(const QList<Project *> &remove)
{
    for (Project *pro : remove)
        emit m_instance->aboutToRemoveProject(pro);

    bool changeStartupProject = false;

    // Delete projects
    for (Project *pro : remove) {
        pro->saveSettings();
        pro->markAsShuttingDown();

        // Remove the project node:
        d->m_projects.removeOne(pro);

        if (pro == d->m_startupProject)
            changeStartupProject = true;

        FolderNavigationWidgetFactory::removeRootDirectory(projectFolderId(pro));
        disconnect(pro, nullptr, m_instance, nullptr);
        emit m_instance->projectRemoved(pro);
    }

    if (changeStartupProject)
        setStartupProject(hasProjects() ? projects().first() : nullptr);

     qDeleteAll(remove);
}

void ProjectManagerPrivate::restoreDependencies()
{
     QMap<QString, QVariant> depMap = mapEntryFromStoreEntry(SessionManager::sessionValue(
                                                                 "ProjectDependencies")).toMap();
     auto i = depMap.constBegin();
     while (i != depMap.constEnd()) {
        const QString &key = i.key();
        FilePaths values;
        const QStringList valueList = i.value().toStringList();
        for (const QString &value : valueList)
            values << FilePath::fromString(value);
        m_depMap.insert(FilePath::fromString(key), values);
        ++i;
     }
}

void ProjectManagerPrivate::askUserAboutFailedProjects()
{
    FilePaths failedProjects = m_failedProjects;
    if (!failedProjects.isEmpty()) {
        QString fileList = FilePath::formatFilePaths(failedProjects, "<br>");
        QMessageBox box(QMessageBox::Warning,
                                   Tr::tr("Failed to restore project files"),
                                   Tr::tr("Could not restore the following project files:<br><b>%1</b>").
                                   arg(fileList));
        auto keepButton = new QPushButton(Tr::tr("Keep projects in Session"), &box);
        auto removeButton = new QPushButton(Tr::tr("Remove projects from Session"), &box);
        box.addButton(keepButton, QMessageBox::AcceptRole);
        box.addButton(removeButton, QMessageBox::DestructiveRole);

        box.exec();

        if (box.clickedButton() == removeButton)
            m_failedProjects.clear();
    }
}

void ProjectManagerPrivate::restoreStartupProject()
{
    const FilePath startupProject = FilePath::fromSettings(
        SessionManager::sessionValue("StartupProject"));
    if (!startupProject.isEmpty()) {
        for (Project *pro : std::as_const(m_projects)) {
            if (pro->projectFilePath() == startupProject) {
                m_instance->setStartupProject(pro);
                break;
            }
        }
    }
    if (!m_startupProject) {
        if (!startupProject.isEmpty())
            qWarning() << "Could not find startup project" << startupProject;
        if (hasProjects())
            m_instance->setStartupProject(m_projects.first());
    }
}

/*!
     Loads a session, takes a session name (not filename).
*/
void ProjectManagerPrivate::restoreProjects(const FilePaths &fileList)
{
    // indirectly adds projects to session
    // Keep projects that failed to load in the session!
    m_failedProjects = fileList;
    if (!fileList.isEmpty()) {
        ProjectExplorerPlugin::OpenProjectResult result = ProjectExplorerPlugin::openProjects(fileList);
        if (!result)
            ProjectExplorerPlugin::showOpenProjectError(result);
        const QList<Project *> projects = result.projects();
        for (const Project *p : projects)
            m_failedProjects.removeAll(p->projectFilePath());
    }
}

void ProjectManagerPrivate::loadSession()
{
    d->m_failedProjects.clear();
    d->m_depMap.clear();
    d->m_casadeSetActive = false;

    // not ideal that this is in ProjectManager
    Id modeId = Id::fromSetting(SessionManager::value("ActiveMode"));
    if (!modeId.isValid())
        modeId = Id(Core::Constants::MODE_EDIT);

    // find a list of projects to close later
    const FilePaths fileList = FileUtils::toFilePathList(
        SessionManager::sessionValue("ProjectList").toStringList());
    const QList<Project *> projectsToRemove
        = Utils::filtered(ProjectManager::projects(), [&fileList](Project *p) {
              return !fileList.contains(p->projectFilePath());
          });
    const QList<Project *> openProjects = ProjectManager::projects();
    const FilePaths projectPathsToLoad
        = Utils::filtered(fileList, [&openProjects](const FilePath &path) {
              return !Utils::contains(openProjects,
                                      [&path](Project *p) { return p->projectFilePath() == path; });
          });

    SessionManager::addSessionLoadingSteps(projectPathsToLoad.count());

    d->restoreProjects(projectPathsToLoad);
    d->restoreDependencies();
    d->restoreStartupProject();

    // only remove old projects now that the startup project is set!
    ProjectManager::removeProjects(projectsToRemove);

    // Fall back to Project mode if the startup project is unconfigured and
    // use the mode saved in the session otherwise
    if (d->m_startupProject && d->m_startupProject->needsConfiguration())
        modeId = Id(Constants::MODE_SESSION);

    ModeManager::activateMode(modeId);
    ModeManager::setFocusToCurrentMode();

    d->m_casadeSetActive = SessionManager::sessionValue("CascadeSetActive", false).toBool();

    // Starts a event loop, better do that at the very end
    QMetaObject::invokeMethod(m_instance, [this] { askUserAboutFailedProjects(); });
}

FilePaths ProjectManager::projectsForSessionName(const QString &session)
{
    const FilePath fileName = SessionManager::sessionNameToFileName(session);
    PersistentSettingsReader reader;
    if (fileName.exists()) {
        if (!reader.load(fileName)) {
            qWarning() << "Could not restore session" << fileName.toUserOutput();
            return {};
        }
    }
    return transform(reader.restoreValue("ProjectList").toStringList(), &FilePath::fromUserInput);
}

#ifdef WITH_TESTS

void ProjectExplorerPlugin::testSessionSwitch()
{
    QVERIFY(SessionManager::createSession("session1"));
    QVERIFY(SessionManager::createSession("session2"));
    QTemporaryFile cppFile("main.cpp");
    QVERIFY(cppFile.open());
    cppFile.close();
    QTemporaryFile projectFile1("XXXXXX.pro");
    QTemporaryFile projectFile2("XXXXXX.pro");
    struct SessionSpec {
        SessionSpec(const QString &n, QTemporaryFile &f) : name(n), projectFile(f) {}
        const QString name;
        QTemporaryFile &projectFile;
    };
    std::vector<SessionSpec> sessionSpecs{SessionSpec("session1", projectFile1),
                SessionSpec("session2", projectFile2)};
    for (const SessionSpec &sessionSpec : sessionSpecs) {
        static const QByteArray proFileContents
                = "TEMPLATE = app\n"
                  "CONFIG -= qt\n"
                  "SOURCES = " + cppFile.fileName().toLocal8Bit();
        QVERIFY(sessionSpec.projectFile.open());
        sessionSpec.projectFile.write(proFileContents);
        sessionSpec.projectFile.close();
        QVERIFY(SessionManager::loadSession(sessionSpec.name));
        const OpenProjectResult openResult
                = ProjectExplorerPlugin::openProject(
                    FilePath::fromString(sessionSpec.projectFile.fileName()));
        if (openResult.errorMessage().contains("text/plain"))
            QSKIP("This test requires the presence of QmakeProjectManager to be fully functional");
        QVERIFY(openResult);
        QCOMPARE(openResult.projects().count(), 1);
        QVERIFY(openResult.project());
        QCOMPARE(ProjectManager::projects().count(), 1);
    }
    for (int i = 0; i < 30; ++i) {
        QVERIFY(SessionManager::loadSession("session1"));
        QCOMPARE(SessionManager::activeSession(), "session1");
        QCOMPARE(ProjectManager::projects().count(), 1);
        QVERIFY(SessionManager::loadSession("session2"));
        QCOMPARE(SessionManager::activeSession(), "session2");
        QCOMPARE(ProjectManager::projects().count(), 1);
    }
    QVERIFY(SessionManager::loadSession("session1"));
    ProjectManager::closeAllProjects();
    QVERIFY(SessionManager::loadSession("session2"));
    ProjectManager::closeAllProjects();
    QVERIFY(SessionManager::deleteSession("session1"));
    QVERIFY(SessionManager::deleteSession("session2"));
}

#endif // WITH_TESTS

} // namespace ProjectExplorer
