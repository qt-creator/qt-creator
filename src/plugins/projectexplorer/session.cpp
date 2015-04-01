/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "session.h"

#include "project.h"
#include "projectexplorer.h"
#include "nodesvisitor.h"
#include "editorconfiguration.h"
#include "projectnodes.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/imode.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/modemanager.h>

#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/stylehelper.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <QMessageBox>
#include <QPushButton>

namespace { bool debug = false; }

using namespace Core;
using namespace Utils;
using namespace ProjectExplorer::Internal;

namespace ProjectExplorer {

/*!
     \class ProjectExplorer::SessionManager

     \brief The SessionManager class manages sessions.

     TODO the interface of this class is not really great.
     The implementation suffers from that all the functions from the
     public interface just wrap around functions which do the actual work.
     This could be improved.
*/

class SessionManagerPrivate
{
public:
    SessionManagerPrivate() :
        m_sessionName(QLatin1String("default")),
        m_virginSession(true),
        m_loadingSession(false),
        m_startupProject(0),
        m_writer(0)
    {}

    bool projectContainsFile(Project *p, const FileName &fileName) const;
    void restoreValues(const PersistentSettingsReader &reader);
    void restoreDependencies(const PersistentSettingsReader &reader);
    void restoreStartupProject(const PersistentSettingsReader &reader);
    void restoreEditors(const PersistentSettingsReader &reader);
    void restoreProjects(const QStringList &fileList);
    void askUserAboutFailedProjects();
    void sessionLoadingProgress();

    bool recursiveDependencyCheck(const QString &newDep, const QString &checkDep) const;
    QStringList dependencies(const QString &proName) const;
    QStringList dependenciesOrder() const;
    void dependencies(const QString &proName, QStringList &result) const;

public:
    static QString windowTitleAddition(const QString &filePath);

    SessionNode *m_sessionNode;
    QString m_sessionName;
    bool m_virginSession;

    mutable QStringList m_sessions;

    mutable QHash<Project *, QStringList> m_projectFileCache;
    bool m_loadingSession;

    Project *m_startupProject;
    QList<Project *> m_projects;
    QStringList m_failedProjects;
    QMap<QString, QStringList> m_depMap;
    QMap<QString, QVariant> m_values;
    QFutureInterface<void> m_future;
    PersistentSettingsWriter *m_writer;
};

static SessionManager *m_instance = 0;
static SessionManagerPrivate *d = 0;

SessionManager::SessionManager(QObject *parent)
  : QObject(parent)
{
    m_instance = this;
    d = new SessionManagerPrivate;

    d->m_sessionNode = new SessionNode;

    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            this, &SessionManager::saveActiveMode);

    connect(EditorManager::instance(), &EditorManager::editorCreated,
            this, &SessionManager::configureEditor);
    connect(this, &SessionManager::projectAdded,
            EditorManager::instance(), &EditorManager::updateWindowTitles);
    connect(this, &SessionManager::projectRemoved,
            EditorManager::instance(), &EditorManager::updateWindowTitles);
    connect(this, &SessionManager::projectDisplayNameChanged,
            EditorManager::instance(), &EditorManager::updateWindowTitles);
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            [this] { markSessionFileDirty(); });
    connect(EditorManager::instance(), &EditorManager::editorsClosed,
            [this] { markSessionFileDirty(); });

    EditorManager::setWindowTitleAdditionHandler(&SessionManagerPrivate::windowTitleAddition);
}

SessionManager::~SessionManager()
{
    emit m_instance->aboutToUnloadSession(d->m_sessionName);
    delete d->m_writer;
    delete d->m_sessionNode;
    delete d;
}

SessionManager *SessionManager::instance()
{
   return m_instance;
}

bool SessionManager::isDefaultVirgin()
{
    return isDefaultSession(d->m_sessionName) && d->m_virginSession;
}

bool SessionManager::isDefaultSession(const QString &session)
{
    return session == QLatin1String("default");
}


void SessionManager::saveActiveMode(IMode *mode)
{
    if (mode->id() != Id(Core::Constants::MODE_WELCOME))
        setValue(QLatin1String("ActiveMode"), mode->id().toString());
}

void SessionManager::clearProjectFileCache()
{
    // If triggered by the fileListChanged signal of one project
    // only invalidate cache for this project
    Project *pro = qobject_cast<Project*>(m_instance->sender());
    if (pro)
        d->m_projectFileCache.remove(pro);
    else
        d->m_projectFileCache.clear();
}

bool SessionManagerPrivate::recursiveDependencyCheck(const QString &newDep, const QString &checkDep) const
{
    if (newDep == checkDep)
        return false;

    foreach (const QString &dependency, m_depMap.value(checkDep))
        if (!recursiveDependencyCheck(newDep, dependency))
            return false;

    return true;
}

/*
 * The dependency management exposes an interface based on projects, but
 * is internally purely string based. This is suboptimal. Probably it would be
 * nicer to map the filenames to projects on load and only map it back to
 * filenames when saving.
 */

QList<Project *> SessionManager::dependencies(const Project *project)
{
    const QString proName = project->projectFilePath().toString();
    const QStringList proDeps = d->m_depMap.value(proName);

    QList<Project *> projects;
    foreach (const QString &dep, proDeps) {
        if (Project *pro = projectForFile(Utils::FileName::fromString(dep)))
            projects += pro;
    }

    return projects;
}

bool SessionManager::hasDependency(const Project *project, const Project *depProject)
{
    const QString proName = project->projectFilePath().toString();
    const QString depName = depProject->projectFilePath().toString();

    const QStringList proDeps = d->m_depMap.value(proName);
    return proDeps.contains(depName);
}

bool SessionManager::canAddDependency(const Project *project, const Project *depProject)
{
    const QString newDep = project->projectFilePath().toString();
    const QString checkDep = depProject->projectFilePath().toString();

    return d->recursiveDependencyCheck(newDep, checkDep);
}

bool SessionManager::addDependency(Project *project, Project *depProject)
{
    const QString proName = project->projectFilePath().toString();
    const QString depName = depProject->projectFilePath().toString();

    // check if this dependency is valid
    if (!d->recursiveDependencyCheck(proName, depName))
        return false;

    QStringList proDeps = d->m_depMap.value(proName);
    if (!proDeps.contains(depName)) {
        proDeps.append(depName);
        d->m_depMap[proName] = proDeps;
    }
    emit m_instance->dependencyChanged(project, depProject);

    return true;
}

void SessionManager::removeDependency(Project *project, Project *depProject)
{
    const QString proName = project->projectFilePath().toString();
    const QString depName = depProject->projectFilePath().toString();

    QStringList proDeps = d->m_depMap.value(proName);
    proDeps.removeAll(depName);
    if (proDeps.isEmpty())
        d->m_depMap.remove(proName);
    else
        d->m_depMap[proName] = proDeps;
    emit m_instance->dependencyChanged(project, depProject);
}

void SessionManager::setStartupProject(Project *startupProject)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << (startupProject ? startupProject->displayName() : QLatin1String("0"));

    if (startupProject) {
        Q_ASSERT(d->m_projects.contains(startupProject));
    }

    if (d->m_startupProject == startupProject)
        return;

    d->m_startupProject = startupProject;
    emit m_instance->startupProjectChanged(startupProject);
}

Project *SessionManager::startupProject()
{
    return d->m_startupProject;
}

void SessionManager::addProject(Project *project)
{
    addProjects(QList<Project*>() << project);
}

void SessionManager::addProjects(const QList<Project*> &projects)
{
    d->m_virginSession = false;
    QList<Project*> clearedList;
    foreach (Project *pro, projects) {
        if (!d->m_projects.contains(pro)) {
            clearedList.append(pro);
            d->m_projects.append(pro);
            d->m_sessionNode->addProjectNodes(QList<ProjectNode *>() << pro->rootProjectNode());

            connect(pro, &Project::fileListChanged,
                    m_instance, &SessionManager::clearProjectFileCache);

            connect(pro, &Project::displayNameChanged,
                    m_instance, &SessionManager::handleProjectDisplayNameChanged);

            if (debug)
                qDebug() << "SessionManager - adding project " << pro->displayName();
        }
    }

    foreach (Project *pro, clearedList) {
        emit m_instance->projectAdded(pro);
        configureEditors(pro);
        connect(pro, &Project::fileListChanged,
                [pro](){
                    configureEditors(pro);
                });
    }

    if (clearedList.count() == 1)
        emit m_instance->singleProjectAdded(clearedList.first());
}

void SessionManager::removeProject(Project *project)
{
    d->m_virginSession = false;
    if (project == 0) {
        qDebug() << "SessionManager::removeProject(0) ... THIS SHOULD NOT HAPPEN";
        return;
    }
    removeProjects(QList<Project*>() << project);
}

bool SessionManager::loadingSession()
{
    return d->m_loadingSession;
}

bool SessionManager::save()
{
    if (debug)
        qDebug() << "SessionManager - saving session" << d->m_sessionName;

    emit m_instance->aboutToSaveSession();

    if (!d->m_writer || d->m_writer->fileName() != sessionNameToFileName(d->m_sessionName)) {
        delete d->m_writer;
        d->m_writer = new PersistentSettingsWriter(sessionNameToFileName(d->m_sessionName),
                                                       QLatin1String("QtCreatorSession"));
    }

    QVariantMap data;
    // save the startup project
    if (d->m_startupProject)
        data.insert(QLatin1String("StartupProject"), d->m_startupProject->projectFilePath().toString());

    QColor c = StyleHelper::requestedBaseColor();
    if (c.isValid()) {
        QString tmp = QString::fromLatin1("#%1%2%3")
                .arg(c.red(), 2, 16, QLatin1Char('0'))
                .arg(c.green(), 2, 16, QLatin1Char('0'))
                .arg(c.blue(), 2, 16, QLatin1Char('0'));
        data.insert(QLatin1String("Color"), tmp);
    }

    QStringList projectFiles;
    foreach (Project *pro, d->m_projects)
        projectFiles << pro->projectFilePath().toString();

    // Restore infromation on projects that failed to load:
    // don't readd projects to the list, which the user loaded
    foreach (const QString &failed, d->m_failedProjects)
        if (!projectFiles.contains(failed))
            projectFiles << failed;

    data.insert(QLatin1String("ProjectList"), projectFiles);

    QMap<QString, QVariant> depMap;
    QMap<QString, QStringList>::const_iterator i = d->m_depMap.constBegin();
    while (i != d->m_depMap.constEnd()) {
        QString key = i.key();
        QStringList values;
        foreach (const QString &value, i.value()) {
            values << value;
        }
        depMap.insert(key, values);
        ++i;
    }
    data.insert(QLatin1String("ProjectDependencies"), QVariant(depMap));
    data.insert(QLatin1String("EditorSettings"), EditorManager::saveState().toBase64());

    QMap<QString, QVariant>::const_iterator it, end = d->m_values.constEnd();
    QStringList keys;
    for (it = d->m_values.constBegin(); it != end; ++it) {
        data.insert(QLatin1String("value-") + it.key(), it.value());
        keys << it.key();
    }

    data.insert(QLatin1String("valueKeys"), keys);

    bool result = d->m_writer->save(data, ICore::mainWindow());
    if (!result) {
        QMessageBox::warning(ICore::dialogParent(), tr("Error while saving session"),
            tr("Could not save session to file %1").arg(d->m_writer->fileName().toUserOutput()));
    }

    if (debug)
        qDebug() << "SessionManager - saving session returned " << result;

    return result;
}

/*!
  Closes all projects
  */
void SessionManager::closeAllProjects()
{
    setStartupProject(0);
    removeProjects(projects());
}

const QList<Project *> &SessionManager::projects()
{
    return d->m_projects;
}

bool SessionManager::hasProjects()
{
    return !d->m_projects.isEmpty();
}

QStringList SessionManagerPrivate::dependencies(const QString &proName) const
{
    QStringList result;
    dependencies(proName, result);
    return result;
}

void SessionManagerPrivate::dependencies(const QString &proName, QStringList &result) const
{
    QStringList depends = m_depMap.value(proName);

    foreach (const QString &dep, depends)
        dependencies(dep, result);

    if (!result.contains(proName))
        result.append(proName);
}

QString SessionManagerPrivate::windowTitleAddition(const QString &filePath)
{
    if (SessionManager::isDefaultSession(d->m_sessionName)) {
        if (filePath.isEmpty()) {
            // use single project's name if there is only one loaded.
            const QList<Project *> projects = SessionManager::projects();
            if (projects.size() == 1)
                return projects.first()->displayName();
            return QString();
        } else if (Project *project = SessionManager::projectForFile(
                       Utils::FileName::fromString(filePath))) {
            return project->displayName();
        } else {
            return QString();
        }
    } else {
        QString sessionName = d->m_sessionName;
        if (sessionName.isEmpty())
            sessionName = SessionManager::tr("Untitled");
        return sessionName;
    }
}

QStringList SessionManagerPrivate::dependenciesOrder() const
{
    QList<QPair<QString, QStringList> > unordered;
    QStringList ordered;

    // copy the map to a temporary list
    foreach (Project *pro, m_projects) {
        const QString proName = pro->projectFilePath().toString();
        unordered << QPair<QString, QStringList>(proName, m_depMap.value(proName));
    }

    while (!unordered.isEmpty()) {
        for (int i=(unordered.count()-1); i>=0; --i) {
            if (unordered.at(i).second.isEmpty()) {
                ordered << unordered.at(i).first;
                unordered.removeAt(i);
            }
        }

        // remove the handled projects from the dependency lists
        // of the remaining unordered projects
        for (int i = 0; i < unordered.count(); ++i) {
            foreach (const QString &pro, ordered) {
                QStringList depList = unordered.at(i).second;
                depList.removeAll(pro);
                unordered[i].second = depList;
            }
        }
    }

    return ordered;
}

QList<Project *> SessionManager::projectOrder(Project *project)
{
    QList<Project *> result;

    QStringList pros;
    if (project)
        pros = d->dependencies(project->projectFilePath().toString());
    else
        pros = d->dependenciesOrder();

    foreach (const QString &proFile, pros) {
        foreach (Project *pro, projects()) {
            if (pro->projectFilePath().toString() == proFile) {
                result << pro;
                break;
            }
        }
    }

    return result;
}

QList<Node *> SessionManager::nodesForFile(const Utils::FileName &fileName)
{
    FindNodesForFileVisitor findNodes(fileName);
    sessionNode()->accept(&findNodes);
    return findNodes.nodes();
}

// node for file returns a randomly selected node if there are multiple
// prefer to use nodesForFile and figure out which node you want
Node *SessionManager::nodeForFile(const Utils::FileName &fileName)
{
    Node *node = 0;
    foreach (Node *n, nodesForFile(fileName)) {
        // prefer file nodes
        if (!node || (node->nodeType() != FileNodeType && n->nodeType() == FileNodeType))
            node = n;
    }
    return node;
}

Project *SessionManager::projectForNode(Node *node)
{
    if (!node)
        return 0;

    FolderNode *rootProjectNode = node->asFolderNode();
    if (!rootProjectNode)
        rootProjectNode = node->parentFolderNode();

    while (rootProjectNode && rootProjectNode->parentFolderNode() != d->m_sessionNode)
        rootProjectNode = rootProjectNode->parentFolderNode();

    Q_ASSERT(rootProjectNode);

    return Utils::findOrDefault(d->m_projects, Utils::equal(&Project::rootProjectNode, rootProjectNode));
}

Project *SessionManager::projectForFile(const Utils::FileName &fileName)
{
    if (debug)
        qDebug() << "SessionManager::projectForFile(" << fileName << ")";

    const QList<Project *> &projectList = projects();
    foreach (Project *p, projectList)
        if (d->projectContainsFile(p, fileName))
            return p;

    return 0;
}

bool SessionManagerPrivate::projectContainsFile(Project *p, const Utils::FileName &fileName) const
{
    if (!m_projectFileCache.contains(p))
        m_projectFileCache.insert(p, p->files(Project::AllFiles));

    return m_projectFileCache.value(p).contains(fileName.toString());
}

void SessionManager::configureEditor(IEditor *editor, const QString &fileName)
{
    if (TextEditor::BaseTextEditor *textEditor = qobject_cast<TextEditor::BaseTextEditor*>(editor)) {
        Project *project = projectForFile(Utils::FileName::fromString(fileName));
        // Global settings are the default.
        if (project)
            project->editorConfiguration()->configureEditor(textEditor);
    }
}

void SessionManager::configureEditors(Project *project)
{
    foreach (IDocument *document, DocumentModel::openedDocuments()) {
        if (d->projectContainsFile(project, document->filePath())) {
            foreach (IEditor *editor, DocumentModel::editorsForDocument(document)) {
                if (TextEditor::BaseTextEditor *textEditor = qobject_cast<TextEditor::BaseTextEditor*>(editor)) {
                        project->editorConfiguration()->configureEditor(textEditor);
                }
            }
        }
    }
}

void SessionManager::removeProjects(QList<Project *> remove)
{
    QMap<QString, QStringList> resMap;

    foreach (Project *pro, remove) {
        if (debug)
            qDebug() << "SessionManager - emitting aboutToRemoveProject(" << pro->displayName() << ")";
        emit m_instance->aboutToRemoveProject(pro);
    }


    // Refresh dependencies
    QSet<QString> projectFiles;
    foreach (Project *pro, projects()) {
        if (!remove.contains(pro))
            projectFiles.insert(pro->projectFilePath().toString());
    }

    QSet<QString>::const_iterator i = projectFiles.begin();
    while (i != projectFiles.end()) {
        QStringList dependencies;
        foreach (const QString &dependency, d->m_depMap.value(*i)) {
            if (projectFiles.contains(dependency))
                dependencies << dependency;
        }
        if (!dependencies.isEmpty())
            resMap.insert(*i, dependencies);
        ++i;
    }

    d->m_depMap = resMap;

    // TODO: Clear m_modelProjectHash

    // Delete projects
    foreach (Project *pro, remove) {
        pro->saveSettings();
        d->m_projects.removeOne(pro);

        if (pro == d->m_startupProject)
            setStartupProject(0);

        disconnect(pro, &Project::fileListChanged,
                   m_instance, &SessionManager::clearProjectFileCache);
        d->m_projectFileCache.remove(pro);

        if (debug)
            qDebug() << "SessionManager - emitting projectRemoved(" << pro->displayName() << ")";
        d->m_sessionNode->removeProjectNodes(QList<ProjectNode *>() << pro->rootProjectNode());
        emit m_instance->projectRemoved(pro);
        delete pro;
    }

    if (startupProject() == 0)
        if (!d->m_projects.isEmpty())
            setStartupProject(d->m_projects.first());
}

/*!
    Lets other plugins store persistent values within the session file.
*/

void SessionManager::setValue(const QString &name, const QVariant &value)
{
    if (d->m_values.value(name) == value)
        return;
    d->m_values.insert(name, value);
    markSessionFileDirty(false);
}

QVariant SessionManager::value(const QString &name)
{
    QMap<QString, QVariant>::const_iterator it = d->m_values.constFind(name);
    return (it == d->m_values.constEnd()) ? QVariant() : *it;
}

QString SessionManager::activeSession()
{
    return d->m_sessionName;
}

QStringList SessionManager::sessions()
{
    if (d->m_sessions.isEmpty()) {
        // We are not initialized yet, so do that now
        QDir sessionDir(ICore::userResourcePath());
        QList<QFileInfo> sessionFiles = sessionDir.entryInfoList(QStringList() << QLatin1String("*.qws"), QDir::NoFilter, QDir::Time);
        foreach (const QFileInfo &fileInfo, sessionFiles) {
            if (fileInfo.completeBaseName() != QLatin1String("default"))
                d->m_sessions << fileInfo.completeBaseName();
        }
        d->m_sessions.prepend(QLatin1String("default"));
    }
    return d->m_sessions;
}

FileName SessionManager::sessionNameToFileName(const QString &session)
{
    return FileName::fromString(ICore::userResourcePath() + QLatin1Char('/') + session + QLatin1String(".qws"));
}

/*!
    Creates \a session, but does not actually create the file.
*/

bool SessionManager::createSession(const QString &session)
{
    if (sessions().contains(session))
        return false;
    Q_ASSERT(d->m_sessions.size() > 0);
    d->m_sessions.insert(1, session);
    return true;
}

bool SessionManager::renameSession(const QString &original, const QString &newName)
{
    if (!cloneSession(original, newName))
        return false;
    if (original == activeSession())
        loadSession(newName);
    return deleteSession(original);
}


/*!
    \brief Shows a dialog asking the user to confirm deleting the session \p session
*/
bool SessionManager::confirmSessionDelete(const QString &session)
{
    return QMessageBox::question(ICore::mainWindow(),
                                 tr("Delete Session"),
                                 tr("Delete session %1?").arg(session),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

/*!
     Deletes \a session name from session list and the file from disk.
*/
bool SessionManager::deleteSession(const QString &session)
{
    if (!d->m_sessions.contains(session))
        return false;
    d->m_sessions.removeOne(session);
    QFile fi(sessionNameToFileName(session).toString());
    if (fi.exists())
        return fi.remove();
    return false;
}

bool SessionManager::cloneSession(const QString &original, const QString &clone)
{
    if (!d->m_sessions.contains(original))
        return false;

    QFile fi(sessionNameToFileName(original).toString());
    // If the file does not exist, we can still clone
    if (!fi.exists() || fi.copy(sessionNameToFileName(clone).toString())) {
        Q_ASSERT(d->m_sessions.size() > 0);
        d->m_sessions.insert(1, clone);
        return true;
    }
    return false;
}

void SessionManagerPrivate::restoreValues(const PersistentSettingsReader &reader)
{
    const QStringList keys = reader.restoreValue(QLatin1String("valueKeys")).toStringList();
    foreach (const QString &key, keys) {
        QVariant value = reader.restoreValue(QLatin1String("value-") + key);
        m_values.insert(key, value);
    }
}

void SessionManagerPrivate::restoreDependencies(const PersistentSettingsReader &reader)
{
    QMap<QString, QVariant> depMap = reader.restoreValue(QLatin1String("ProjectDependencies")).toMap();
    QMap<QString, QVariant>::const_iterator i = depMap.constBegin();
    while (i != depMap.constEnd()) {
        const QString &key = i.key();
        if (!m_failedProjects.contains(key)) {
            QStringList values;
            foreach (const QString &value, i.value().toStringList()) {
                if (!m_failedProjects.contains(value))
                    values << value;
            }
            m_depMap.insert(key, values);
        }
        ++i;
    }
}

void SessionManagerPrivate::askUserAboutFailedProjects()
{
    QStringList failedProjects = m_failedProjects;
    if (!failedProjects.isEmpty()) {
        QString fileList =
            QDir::toNativeSeparators(failedProjects.join(QLatin1String("<br>")));
        QMessageBox * box = new QMessageBox(QMessageBox::Warning,
                                            SessionManager::tr("Failed to restore project files"),
                                            SessionManager::tr("Could not restore the following project files:<br><b>%1</b>").
                                            arg(fileList));
        QPushButton * keepButton = new QPushButton(SessionManager::tr("Keep projects in Session"), box);
        QPushButton * removeButton = new QPushButton(SessionManager::tr("Remove projects from Session"), box);
        box->addButton(keepButton, QMessageBox::AcceptRole);
        box->addButton(removeButton, QMessageBox::DestructiveRole);

        box->exec();

        if (box->clickedButton() == removeButton)
            m_failedProjects.clear();
    }
}

void SessionManagerPrivate::restoreStartupProject(const PersistentSettingsReader &reader)
{
    const QString startupProject = reader.restoreValue(QLatin1String("StartupProject")).toString();
    if (!startupProject.isEmpty()) {
        foreach (Project *pro, d->m_projects) {
            if (pro->projectFilePath().toString() == startupProject) {
                m_instance->setStartupProject(pro);
                break;
            }
        }
    }
    if (!m_startupProject) {
        if (!startupProject.isEmpty())
            qWarning() << "Could not find startup project" << startupProject;
        if (!m_projects.isEmpty())
            m_instance->setStartupProject(m_projects.first());
    }
}

void SessionManagerPrivate::restoreEditors(const PersistentSettingsReader &reader)
{
    const QVariant editorsettings = reader.restoreValue(QLatin1String("EditorSettings"));
    if (editorsettings.isValid()) {
        EditorManager::restoreState(QByteArray::fromBase64(editorsettings.toByteArray()));
        sessionLoadingProgress();
    }
}

/*!
     Loads a session, takes a session name (not filename).
*/
void SessionManagerPrivate::restoreProjects(const QStringList &fileList)
{
    // indirectly adds projects to session
    // Keep projects that failed to load in the session!
    m_failedProjects = fileList;
    if (!fileList.isEmpty()) {
        QString errors;
        QList<Project *> projects = ProjectExplorerPlugin::openProjects(fileList, &errors);
        if (!errors.isEmpty())
            QMessageBox::critical(ICore::mainWindow(), SessionManager::tr("Failed to open project"), errors);
        foreach (Project *p, projects)
            m_failedProjects.removeAll(p->projectFilePath().toString());
    }
}

bool SessionManager::loadSession(const QString &session)
{
    // Do nothing if we have that session already loaded,
    // exception if the session is the default virgin session
    // we still want to be able to load the default session
    if (session == d->m_sessionName && !isDefaultVirgin())
        return true;

    if (!sessions().contains(session))
        return false;

    // Try loading the file
    FileName fileName = sessionNameToFileName(session);
    PersistentSettingsReader reader;
    if (fileName.exists()) {
        if (!reader.load(fileName)) {
            QMessageBox::warning(ICore::dialogParent(), tr("Error while restoring session"),
                                 tr("Could not restore session %1").arg(fileName.toUserOutput()));
            return false;
        }
    }

    d->m_loadingSession = true;

    // Allow everyone to set something in the session and before saving
    emit m_instance->aboutToUnloadSession(d->m_sessionName);

    if (!isDefaultVirgin()) {
        if (!save()) {
            d->m_loadingSession = false;
            return false;
        }
    }

    // Clean up
    if (!EditorManager::closeAllEditors()) {
        d->m_loadingSession = false;
        return false;
    }

    setStartupProject(0);
    removeProjects(projects());

    d->m_failedProjects.clear();
    d->m_depMap.clear();
    d->m_values.clear();

    d->m_sessionName = session;
    EditorManager::updateWindowTitles();

    if (fileName.exists()) {
        d->m_virginSession = false;

        ProgressManager::addTask(d->m_future.future(), tr("Loading Session"),
           "ProjectExplorer.SessionFile.Load");

        d->m_future.setProgressRange(0, 1);
        d->m_future.setProgressValue(0);

        d->restoreValues(reader);
        emit m_instance->aboutToLoadSession(session);

        // retrieve all values before the following code could change them again
        Id modeId = Id::fromSetting(value(QLatin1String("ActiveMode")));

        QColor c = QColor(reader.restoreValue(QLatin1String("Color")).toString());
        if (c.isValid())
            StyleHelper::setBaseColor(c);

        QStringList fileList =
            reader.restoreValue(QLatin1String("ProjectList")).toStringList();

        d->m_future.setProgressRange(0, fileList.count() + 1/*initialization above*/ + 1/*editors*/);
        d->m_future.setProgressValue(1);

        // if one processEvents doesn't get the job done
        // just use two!
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        d->restoreProjects(fileList);
        d->sessionLoadingProgress();
        d->restoreDependencies(reader);
        d->restoreStartupProject(reader);
        d->restoreEditors(reader);

        d->m_future.reportFinished();
        d->m_future = QFutureInterface<void>();

        // restore the active mode
        if (!modeId.isValid())
            modeId = Id(Core::Constants::MODE_EDIT);

        ModeManager::activateMode(modeId);
        ModeManager::setFocusToCurrentMode();
    } else {
        ModeManager::activateMode(Id(Core::Constants::MODE_EDIT));
        ModeManager::setFocusToCurrentMode();
    }
    emit m_instance->sessionLoaded(session);

    // Starts a event loop, better do that at the very end
    d->askUserAboutFailedProjects();
    d->m_loadingSession = false;
    return true;
}

QString SessionManager::lastSession()
{
    return ICore::settings()->value(QLatin1String("ProjectExplorer/StartupSession")).toString();
}

SessionNode *SessionManager::sessionNode()
{
    return d->m_sessionNode;
}

void SessionManager::reportProjectLoadingProgress()
{
    d->sessionLoadingProgress();
}

void SessionManager::markSessionFileDirty(bool makeDefaultVirginDirty)
{
    if (makeDefaultVirginDirty)
        d->m_virginSession = false;
}

void SessionManagerPrivate::sessionLoadingProgress()
{
    m_future.setProgressValue(m_future.progressValue() + 1);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void SessionManager::handleProjectDisplayNameChanged()
{
    Project *pro = qobject_cast<Project*>(m_instance->sender());
    if (pro) {
        d->m_sessionNode->projectDisplayNameChanged(pro->rootProjectNode());
        emit m_instance->projectDisplayNameChanged(pro);
    }
}

QStringList SessionManager::projectsForSessionName(const QString &session)
{
    const FileName fileName = sessionNameToFileName(session);
    PersistentSettingsReader reader;
    if (fileName.exists()) {
        if (!reader.load(fileName)) {
            qWarning() << "Could not restore session" << fileName.toUserOutput();
            return QStringList();
        }
    }
    return reader.restoreValue(QLatin1String("ProjectList")).toStringList();
}

} // namespace ProjectExplorer
