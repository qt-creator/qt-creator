/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "qt4project.h"

#include "qt4projectmanager.h"
#include "profilereader.h"
#include "prowriter.h"
#include "makestep.h"
#include "qmakestep.h"
#include "deployhelper.h"
#include "qt4runconfiguration.h"
#include "qtversionmanager.h"
#include "qt4nodes.h"
#include "qt4buildconfigwidget.h"
#include "qt4buildenvironmentwidget.h"
#include "qt4projectmanagerconstants.h"
#include "projectloadwizard.h"
#include "gdbmacrosbuildstep.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/coreconstants.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/nodesvisitor.h>
#include <projectexplorer/project.h>
#include <projectexplorer/customexecutablerunconfiguration.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtGui/QFileDialog>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace ProjectExplorer;

enum { debug = 0 };

namespace Qt4ProjectManager {
namespace Internal {

// Qt4ProjectFiles: Struct for (Cached) lists of files in a project
struct Qt4ProjectFiles {
    void clear();
    bool equals(const Qt4ProjectFiles &f) const;

    QStringList files[ProjectExplorer::FileTypeSize];
    QStringList generatedFiles[ProjectExplorer::FileTypeSize];
    QStringList proFiles;
};

void Qt4ProjectFiles::clear()
{
    for (int i = 0; i < FileTypeSize; ++i) {
        files[i].clear();
        generatedFiles[i].clear();
    }
    proFiles.clear();
}

bool Qt4ProjectFiles::equals(const Qt4ProjectFiles &f) const
{
    for (int i = 0; i < FileTypeSize; ++i)
        if (files[i] != f.files[i] || generatedFiles[i] != f.generatedFiles[i])
            return false;
    if (proFiles != f.proFiles)
        return false;
    return true;
}

inline bool operator==(const Qt4ProjectFiles &f1, const Qt4ProjectFiles &f2)
{       return f1.equals(f2); }

inline bool operator!=(const Qt4ProjectFiles &f1, const Qt4ProjectFiles &f2)
{       return !f1.equals(f2); }

QDebug operator<<(QDebug d, const  Qt4ProjectFiles &f)
{
    QDebug nsp = d.nospace();
    nsp << "Qt4ProjectFiles: proFiles=" <<  f.proFiles << '\n';
    for (int i = 0; i < FileTypeSize; ++i)
        nsp << "Type " << i << " files=" << f.files[i] <<  " generated=" << f.generatedFiles[i] << '\n';
    return d;
}

// A visitor to collect all files of a project in a Qt4ProjectFiles struct
class ProjectFilesVisitor : public ProjectExplorer::NodesVisitor
{
    Q_DISABLE_COPY(ProjectFilesVisitor)
    ProjectFilesVisitor(Qt4ProjectFiles *files);
public:

    static void findProjectFiles(Qt4ProFileNode *rootNode, Qt4ProjectFiles *files);

    void visitProjectNode(ProjectNode *projectNode);
    void visitFolderNode(FolderNode *folderNode);

private:
    Qt4ProjectFiles *m_files;
};

ProjectFilesVisitor::ProjectFilesVisitor(Qt4ProjectFiles *files) :
    m_files(files)
{
}

void ProjectFilesVisitor::findProjectFiles(Qt4ProFileNode *rootNode, Qt4ProjectFiles *files)
{
    files->clear();
    ProjectFilesVisitor visitor(files);
    rootNode->accept(&visitor);
    for (int i = 0; i < FileTypeSize; ++i) {
        qSort(files->files[i]);
        qSort(files->generatedFiles[i]);
    }
    qSort(files->proFiles);
}

void ProjectFilesVisitor::visitProjectNode(ProjectNode *projectNode)
{
    const QString path = projectNode->path();
    if (!m_files->proFiles.contains(path))
        m_files->proFiles.append(path);
    visitFolderNode(projectNode);
}

void ProjectFilesVisitor::visitFolderNode(FolderNode *folderNode)
{
    foreach (FileNode *fileNode, folderNode->fileNodes()) {
        const QString path = fileNode->path();
        const int type = fileNode->fileType();
        QStringList &targetList = fileNode->isGenerated() ? m_files->generatedFiles[type] : m_files->files[type];
        if (!targetList.contains(path))
            targetList.push_back(path);
    }
}

}
}

// ----------- Qt4ProjectFile
Qt4ProjectFile::Qt4ProjectFile(Qt4Project *project, const QString &filePath, QObject *parent)
    : Core::IFile(parent),
      m_mimeType(QLatin1String(Qt4ProjectManager::Constants::PROFILE_MIMETYPE)),
      m_project(project),
      m_filePath(filePath)
{
}

bool Qt4ProjectFile::save(const QString &)
{
    // This is never used
    return false;
}

QString Qt4ProjectFile::fileName() const
{
    return m_filePath;
}

QString Qt4ProjectFile::defaultPath() const
{
    return QString();
}

QString Qt4ProjectFile::suggestedFileName() const
{
    return QString();
}

QString Qt4ProjectFile::mimeType() const
{
    return m_mimeType;
}

bool Qt4ProjectFile::isModified() const
{
    return false; // we save after changing anyway
}

bool Qt4ProjectFile::isReadOnly() const
{
    QFileInfo fi(m_filePath);
    return !fi.isWritable();
}

bool Qt4ProjectFile::isSaveAsAllowed() const
{
    return false;
}

void Qt4ProjectFile::modified(Core::IFile::ReloadBehavior *)
{
}

/*!
  /class Qt4Project

  Qt4Project manages information about an individual Qt 4 (.pro) project file.
  */

Qt4Project::Qt4Project(Qt4Manager *manager, const QString& fileName) :
    m_manager(manager),
    m_rootProjectNode(new Qt4ProFileNode(this, fileName, this)),
    m_nodesWatcher(new Internal::Qt4NodesWatcher(this)),
    m_fileInfo(new Qt4ProjectFile(this, fileName, this)),
    m_isApplication(true),
    m_projectFiles(new Qt4ProjectFiles)
{
    m_manager->registerProject(this);
    m_rootProjectNode->registerWatcher(m_nodesWatcher);
    connect(m_nodesWatcher, SIGNAL(foldersAdded()), this, SLOT(updateFileList()));
    connect(m_nodesWatcher, SIGNAL(foldersRemoved()), this, SLOT(updateFileList()));
    connect(m_nodesWatcher, SIGNAL(filesAdded()), this, SLOT(updateFileList()));
    connect(m_nodesWatcher, SIGNAL(filesRemoved()), this, SLOT(updateFileList()));
    connect(m_nodesWatcher, SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *)),
            this, SLOT(scheduleUpdateCodeModel()));

    connect(qt4ProjectManager()->versionManager(), SIGNAL(defaultQtVersionChanged()),
            this, SLOT(defaultQtVersionChanged()));
    connect(qt4ProjectManager()->versionManager(), SIGNAL(qtVersionsChanged()),
            this, SLOT(qtVersionsChanged()));

    m_updateCodeModelTimer.setSingleShot(true);
    m_updateCodeModelTimer.setInterval(20);
    connect(&m_updateCodeModelTimer, SIGNAL(timeout()), this, SLOT(updateCodeModel()));

    m_addUiFilesTimer.setSingleShot(true);
    m_addUiFilesTimer.setInterval(20);
    connect(&m_addUiFilesTimer, SIGNAL(timeout()), this, SLOT(addUiFiles()));
}

Qt4Project::~Qt4Project()
{
    m_manager->unregisterProject(this);
    delete m_projectFiles;
}

void Qt4Project::defaultQtVersionChanged()
{
    if (qtVersionId(activeBuildConfiguration()) == 0)
        m_rootProjectNode->update();
}

void Qt4Project::qtVersionsChanged()
{
    foreach (QString bc, buildConfigurations()) {
        if (!qt4ProjectManager()->versionManager()->version(qtVersionId(bc))->isValid()) {
            setQtVersion(bc, 0);
            if (bc == activeBuildConfiguration())
                m_rootProjectNode->update();
        }
    }
}

void Qt4Project::updateFileList()
{
    Qt4ProjectFiles newFiles;
    ProjectFilesVisitor::findProjectFiles(m_rootProjectNode, &newFiles);
    if (newFiles != *m_projectFiles) {
        *m_projectFiles = newFiles;
        emit fileListChanged();
        if (debug)
            qDebug() << Q_FUNC_INFO << *m_projectFiles;
    }
}

void Qt4Project::restoreSettingsImpl(PersistentSettingsReader &settingsReader)
{
    Project::restoreSettingsImpl(settingsReader);

    addDefaultBuild();

    // Ensure that the qt version in each build configuration is valid
    // or if not, is reset to the default
    foreach (const QString &bc, buildConfigurations())
        qtVersionId(bc);

    update();

    // restored old runconfigurations
    if (runConfigurations().isEmpty()) {
        // Oha no runConfigurations, add some
        QList<Qt4ProFileNode *> list;
        collectApplicationProFiles(list, m_rootProjectNode);

        if (!list.isEmpty()) {
            foreach (Qt4ProFileNode *node, list) {
                QSharedPointer<RunConfiguration> rc(new Qt4RunConfiguration(this, node->path()));
                addRunConfiguration(rc);
            }
            setActiveRunConfiguration(runConfigurations().first());
        } else {
            QSharedPointer<RunConfiguration> rc(new ProjectExplorer::CustomExecutableRunConfiguration(this));
            addRunConfiguration(rc);
            setActiveRunConfiguration(rc);
            m_isApplication = false;
        }
    }

    // Now connect
    connect(m_nodesWatcher, SIGNAL(foldersAboutToBeAdded(FolderNode *, const QList<FolderNode*> &)),
            this, SLOT(foldersAboutToBeAdded(FolderNode *, const QList<FolderNode*> &)));
    connect(m_nodesWatcher, SIGNAL(foldersAdded()), this, SLOT(checkForNewApplicationProjects()));

    connect(m_nodesWatcher, SIGNAL(foldersRemoved()), this, SLOT(checkForDeletedApplicationProjects()));

    connect(m_nodesWatcher, SIGNAL(projectTypeChanged(Qt4ProjectManager::Internal::Qt4ProFileNode *,
                                                      const Qt4ProjectManager::Internal::Qt4ProjectType,
                                                      const Qt4ProjectManager::Internal::Qt4ProjectType)),
            this, SLOT(projectTypeChanged(Qt4ProjectManager::Internal::Qt4ProFileNode *,
                                          const Qt4ProjectManager::Internal::Qt4ProjectType,
                                          const Qt4ProjectManager::Internal::Qt4ProjectType)));

    connect(m_nodesWatcher, SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *)),
            this, SLOT(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *)));

}

void Qt4Project::saveSettingsImpl(ProjectExplorer::PersistentSettingsWriter &writer)
{
    Project::saveSettingsImpl(writer);
}

namespace {
    class FindQt4ProFiles: protected ProjectExplorer::NodesVisitor {
        QList<Qt4ProFileNode *> m_proFiles;

    public:
        QList<Qt4ProFileNode *> operator()(ProjectNode *root)
        {
            m_proFiles.clear();
            root->accept(this);
            return m_proFiles;
        }

    protected:
        virtual void visitProjectNode(ProjectNode *projectNode)
        {
            if (Qt4ProFileNode *pro = qobject_cast<Qt4ProFileNode *>(projectNode))
                m_proFiles.append(pro);
        }
    };
}

void Qt4Project::addUiFilesToCodeModel(const QStringList &files)
{
    // if we already have a full updateCodeModel() scheduled
    // then we don't need to this seperately
    // since that one will add also all the ui files
    if (m_updateCodeModelTimer.isActive())
        return;
    m_addUiFilesTimer.start();
    m_uiFilesToAdd << files;
}

void Qt4Project::addUiFiles()
{
    if (m_updateCodeModelTimer.isActive())
        return;
    CppTools::CppModelManagerInterface *modelManager =
        ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();
    modelManager->updateSourceFiles(m_uiFilesToAdd);
    m_uiFilesToAdd.clear();
}

void Qt4Project::scheduleUpdateCodeModel()
{
    m_updateCodeModelTimer.start();
}

void Qt4Project::updateCodeModel()
{
    if (debug)
        qDebug()<<"Qt4Project::updateCodeModel()";

    CppTools::CppModelManagerInterface *modelmanager =
        ExtensionSystem::PluginManager::instance()
            ->getObject<CppTools::CppModelManagerInterface>();

    if (!modelmanager)
        return;

    QStringList allIncludePaths;
    QStringList allFrameworkPaths;

    const QHash<QString, QString> versionInfo = qtVersion(activeBuildConfiguration())->versionInfo();
    const QString newQtIncludePath = versionInfo.value(QLatin1String("QT_INSTALL_HEADERS"));
    const QString newQtLibsPath = versionInfo.value(QLatin1String("QT_INSTALL_LIBS"));

    QByteArray predefinedMacros;
    QtVersion::ToolchainType t = qtVersion(activeBuildConfiguration())->toolchainType();
    if (t == QtVersion::MinGW || t == QtVersion::OTHER) {
        QStringList list = rootProjectNode()->variableValue(Internal::CxxCompilerVar);
        QString qmake_cxx = list.isEmpty() ? QString::null : list.first();

        if (qmake_cxx.isEmpty()) {
            // macx-xcode mkspec resets the value of QMAKE_CXX.
            // Unfortunately, we need a valid QMAKE_CXX to configure the parser.
            qmake_cxx = QLatin1String("cc");
        }

        qmake_cxx = environment(activeBuildConfiguration()).searchInPath(qmake_cxx);
        m_preproc.setGcc(qmake_cxx);
        predefinedMacros = m_preproc.predefinedMacros();
        foreach (HeaderPath headerPath, m_preproc.systemHeaderPaths()) {
            if (headerPath.kind() == HeaderPath::FrameworkHeaderPath)
                allFrameworkPaths.append(headerPath.path());
            else
                allIncludePaths.append(headerPath.path());
        }

    } else if (t == QtVersion::MSVC || t == QtVersion::WINCE) {
#ifdef QTCREATOR_WITH_MSVC_INCLUDES
        Environment env = environment(activeBuildConfiguration());
        allIncludePaths.append(env.value("INCLUDE").split(QLatin1Char(';')));
#endif
        predefinedMacros +=
                "#define __WIN32__\n"
                "#define __WIN32\n"
                "#define _WIN32\n"
                "#define WIN32\n"
                "#define __WINNT__\n"
                "#define __WINNT\n"
                "#define WINNT\n"
                "#define _X86_\n"
                "#define __MSVCRT__\n";
    }

    allIncludePaths.append(newQtIncludePath);

    QDir dir(newQtIncludePath);
    foreach (QFileInfo info, dir.entryInfoList(QDir::Dirs)) {
        if (! info.fileName().startsWith(QLatin1String("Qt")))
            continue;
        allIncludePaths.append(info.absoluteFilePath());
    }

#ifdef Q_OS_MAC
    allFrameworkPaths.append(newQtLibsPath);
    // put QtXXX.framework/Headers directories in include path since that qmake's behavior
    QDir frameworkDir(newQtLibsPath);
    foreach (QFileInfo info, frameworkDir.entryInfoList(QDir::Dirs)) {
        if (! info.fileName().startsWith(QLatin1String("Qt")))
            continue;
        allIncludePaths.append(info.absoluteFilePath()+"/Headers");
    }
#endif

    FindQt4ProFiles findQt4ProFiles;
    QList<Qt4ProFileNode *> proFiles = findQt4ProFiles(rootProjectNode());
    QByteArray definedMacros;

    foreach (Qt4ProFileNode *pro, proFiles) {
        foreach (const QString def, pro->variableValue(DefinesVar)) {
            definedMacros += "#define ";
            const int index = def.indexOf(QLatin1Char('='));
            if (index == -1) {
                definedMacros += def.toLatin1();
                definedMacros += " 1\n";
            } else {
                const QString name = def.left(index);
                const QString value = def.mid(index + 1);
                definedMacros += name.toLatin1();
                definedMacros += ' ';
                definedMacros += value.toLocal8Bit();
                definedMacros += '\n';
            }
        }

        const QStringList proIncludePaths = pro->variableValue(IncludePathVar);
        foreach (QString includePath, proIncludePaths) {
            if (allIncludePaths.contains(includePath))
                continue;

            allIncludePaths.append(includePath);
        }
    }

    // Add mkspec directory
    allIncludePaths.append(qtVersion(activeBuildConfiguration())->mkspecPath());

    QStringList files;
    files += m_projectFiles->files[HeaderType];
    files += m_projectFiles->generatedFiles[HeaderType];
    files += m_projectFiles->files[SourceType];
    files += m_projectFiles->generatedFiles[SourceType];

    CppTools::CppModelManagerInterface::ProjectInfo pinfo = modelmanager->projectInfo(this);

    if (pinfo.defines == predefinedMacros             &&
            pinfo.includePaths == allIncludePaths     &&
            pinfo.frameworkPaths == allFrameworkPaths &&
            pinfo.sourceFiles == files) {
        modelmanager->updateProjectInfo(pinfo);
    } else {
        pinfo.defines = predefinedMacros;
        // pinfo->defines += definedMacros;   // ### FIXME: me
        pinfo.includePaths = allIncludePaths;
        pinfo.frameworkPaths = allFrameworkPaths;
        pinfo.sourceFiles = files;

        modelmanager->updateProjectInfo(pinfo);
        modelmanager->updateSourceFiles(pinfo.sourceFiles);
    }
}


///*!
//  Updates complete project
//  */
void Qt4Project::update()
{
    // TODO Maybe remove this method completely?
    m_rootProjectNode->update();
    //updateCodeModel();
}

ProFileReader *Qt4Project::createProFileReader() const
{
    ProFileReader *reader = new ProFileReader();
    connect(reader, SIGNAL(errorFound(const QString&)),
            this, SLOT(proFileParseError(const QString&)));
    QtVersion *version = qtVersion(activeBuildConfiguration());
    if (version->isValid()) {
        reader->setQtVersion(version);
    }
    return reader;
}

/*!
  Returns whether the project is an application, or has an application as a subproject.
 */
bool Qt4Project::isApplication() const
{
    return m_isApplication;
}

ProjectExplorer::ProjectExplorerPlugin *Qt4Project::projectExplorer() const
{
    return m_manager->projectExplorer();
}

ProjectExplorer::IProjectManager *Qt4Project::projectManager() const
{
    return m_manager;
}

Qt4Manager *Qt4Project::qt4ProjectManager() const
{
    return m_manager;
}

QString Qt4Project::name() const
{
    return QFileInfo(file()->fileName()).completeBaseName();
}

Core::IFile *Qt4Project::file() const
{
    return m_fileInfo;
}

QStringList Qt4Project::files(FilesMode fileMode) const
{
    QStringList files;
    for (int i = 0; i < FileTypeSize; ++i) {
        files += m_projectFiles->files[i];
        if (fileMode == AllFiles)
            files += m_projectFiles->generatedFiles[i];
    }
    return files;
}

QList<ProjectExplorer::Project*> Qt4Project::dependsOn()
{
    // NBS implement dependsOn
    return QList<Project *>();
}

void Qt4Project::addDefaultBuild()
{
    if (buildConfigurations().isEmpty()) {
        // We don't have any buildconfigurations, so this is a new project
        // The Project Load Wizard is a work of art
        // It will ask the user what kind of build setup he want
        // It will add missing Qt Versions
        // And get the project into a buildable state

        //TODO have a better check wheter there is already a configuration?
        QMakeStep *qmakeStep = 0;
        MakeStep *makeStep = 0;
        GdbMacrosBuildStep *gdbmacrostep;

        gdbmacrostep = new GdbMacrosBuildStep(this);
        insertBuildStep(0, gdbmacrostep);

        qmakeStep = new QMakeStep(this);
        qmakeStep->setValue("mkspec", "");
        insertBuildStep(1, qmakeStep);

        makeStep = new MakeStep(this);
        insertBuildStep(2, makeStep);

        GdbMacrosBuildStep *gdbmacrosCleanStep = new GdbMacrosBuildStep(this);
        gdbmacrosCleanStep->setValue("clean", true);
        insertCleanStep(0, gdbmacrosCleanStep);

        MakeStep* cleanStep = new MakeStep(this);
        cleanStep->setValue("clean", true);
        insertCleanStep(1, cleanStep);

        ProjectLoadWizard wizard(this);
        wizard.execDialog();
    } else {
        // Restoring configuration
        // Do we already have a gdbmacrobuildstep?
        // If not add it and disable linking of debugging helper

        // Check for old link debugging helper setting in each buildConfiguration
        // We add a gdbmacrosbuildstep if at least one has it
        // TODO remove migration code from pre beta
        foreach(const QString &bc, buildConfigurations()) {
            QVariant v = value(bc, "addQDumper");
            if (v.isValid() && v.toBool()) {
                GdbMacrosBuildStep *gdbmacrostep = new GdbMacrosBuildStep(this);
                insertBuildStep(0, gdbmacrostep);

                GdbMacrosBuildStep *gdbmacrosCleanStep = new GdbMacrosBuildStep(this);
                gdbmacrosCleanStep ->setValue("clean", true);
                insertCleanStep(0, gdbmacrosCleanStep );
                break;
            }
        }

        foreach(const QString &bc, buildConfigurations()) {
            setValue(bc, "addQDumper", QVariant());
        }
    }
}

void Qt4Project::newBuildConfiguration(const QString &buildConfiguration)
{
    Q_UNUSED(buildConfiguration);
}

void Qt4Project::proFileParseError(const QString &errorMessage)
{
    Core::ICore::instance()->messageManager()->printToOutputPane(errorMessage);
}

Qt4ProFileNode *Qt4Project::rootProjectNode() const
{
    return m_rootProjectNode;
}

ProjectExplorer::Environment Qt4Project::baseEnvironment(const QString &buildConfiguration) const
{
    Environment env = useSystemEnvironment(buildConfiguration) ? Environment(QProcess::systemEnvironment()) : Environment();
    env = qtVersion(buildConfiguration)->addToEnvironment(env);
    return env;
}

ProjectExplorer::Environment Qt4Project::environment(const QString &buildConfiguration) const
{
    Environment env = baseEnvironment(buildConfiguration);
    env.modify(userEnvironmentChanges(buildConfiguration));
    return env;
}

QString Qt4Project::buildDirectory(const QString &buildConfiguration) const
{
    QString workingDirectory;
    if (value(buildConfiguration, "useShadowBuild").toBool())
        workingDirectory = value(buildConfiguration, "buildDirectory").toString();
    if (workingDirectory.isEmpty())
        workingDirectory = QFileInfo(file()->fileName()).absolutePath();
    return workingDirectory;
}

void Qt4Project::setUseSystemEnvironment(const QString &buildConfiguration, bool b)
{
    setValue(buildConfiguration, "clearSystemEnvironment", !b);
}

bool Qt4Project::useSystemEnvironment(const QString &buildConfiguration) const
{
    bool b = !(value(buildConfiguration, "clearSystemEnvironment").isValid() && value(buildConfiguration, "clearSystemEnvironment").toBool());
    return b;
}

QString Qt4Project::qtDir(const QString &buildConfiguration) const
{
    QtVersion *version = qtVersion(buildConfiguration);
    if (version)
        return version->path();
    return QString::null;
}

QtVersion *Qt4Project::qtVersion(const QString &buildConfiguration) const
{
    return m_manager->versionManager()->version(qtVersionId(buildConfiguration));
}

int Qt4Project::qtVersionId(const QString &buildConfiguration) const
{
    if (debug)
        qDebug()<<"Looking for qtVersion ID of "<<buildConfiguration;
    int id = 0;
    QVariant vid = value(buildConfiguration, "QtVersionId");
    if (vid.isValid()) {
        id = vid.toInt();
        if (m_manager->versionManager()->version(id)->isValid()) {
            return id;
        } else {
            const_cast<Qt4Project *>(this)->setValue(buildConfiguration, "QtVersionId", 0);
            return 0;
        }
    } else {
        // Backward compatibilty, we might have just the name:
        QString vname = value(buildConfiguration, "QtVersion").toString();
        if (debug)
            qDebug()<<"  Backward compatibility reading QtVersion"<<vname;
        if (!vname.isEmpty()) {
            const QList<QtVersion *> &versions = m_manager->versionManager()->versions();
            foreach (const QtVersion * const version, versions) {
                if (version->name() == vname) {
                    if (debug)
                        qDebug()<<"found name in versions";
                    const_cast<Qt4Project *>(this)->setValue(buildConfiguration, "QtVersionId", version->uniqueId());
                    return version->uniqueId();
                }
            }
        }
    }
    if (debug)
        qDebug()<<"  using qtversion with id ="<<id;
    // Nothing found, reset to default
    const_cast<Qt4Project *>(this)->setValue(buildConfiguration, "QtVersionId", id);
    return id;
}

void Qt4Project::setQtVersion(const QString &buildConfiguration, int id)
{
    setValue(buildConfiguration, "QtVersionId", id);
}

BuildStepConfigWidget *Qt4Project::createConfigWidget()
{
    return new Qt4BuildConfigWidget(this);
}

QList<BuildStepConfigWidget*> Qt4Project::subConfigWidgets()
{
    QList<BuildStepConfigWidget*> subWidgets;
    subWidgets << new Qt4BuildEnvironmentWidget(this);
    return subWidgets;
}

QList<ProjectExplorer::EnvironmentItem> Qt4Project::userEnvironmentChanges(const QString &buildConfig) const
{
    return EnvironmentItem::fromStringList(value(buildConfig, "userEnvironmentChanges").toStringList());
}

void Qt4Project::setUserEnvironmentChanges(const QString &buildConfig, const QList<ProjectExplorer::EnvironmentItem> &diff)
{
    setValue(buildConfig, "userEnvironmentChanges", EnvironmentItem::toStringList(diff));
}

/// **************************
/// Qt4ProjectBuildConfigWidget
/// **************************


void Qt4Project::collectApplicationProFiles(QList<Qt4ProFileNode *> &list, Qt4ProFileNode *node)
{
    if (node->projectType() == Internal::ApplicationTemplate
        || node->projectType() == Internal::ScriptTemplate) {
        list.append(node);
    }
    foreach (ProjectNode *n, node->subProjectNodes()) {
        Qt4ProFileNode *qt4ProFileNode = qobject_cast<Qt4ProFileNode *>(n);
        if (qt4ProFileNode)
            collectApplicationProFiles(list, qt4ProFileNode);
    }
}

void Qt4Project::foldersAboutToBeAdded(FolderNode *, const QList<FolderNode*> &nodes)
{
    QList<Qt4ProFileNode *> list;
    foreach (FolderNode *node, nodes) {
        Qt4ProFileNode *qt4ProFileNode = qobject_cast<Qt4ProFileNode *>(node);
        if (qt4ProFileNode)
            collectApplicationProFiles(list, qt4ProFileNode);
    }
    m_applicationProFileChange = list;
}

void Qt4Project::checkForNewApplicationProjects()
{
    // Check all new project nodes
    // against all runConfigurations

    foreach (Qt4ProFileNode *qt4proFile, m_applicationProFileChange) {
        bool found = false;
        foreach (QSharedPointer<RunConfiguration> rc, runConfigurations()) {
            QSharedPointer<Qt4RunConfiguration> qtrc = rc.dynamicCast<Qt4RunConfiguration>();
            if (qtrc && qtrc->proFilePath() == qt4proFile->path()) {
                found = true;
                break;
            }
        }
        if (!found) {
            QSharedPointer<Qt4RunConfiguration> newRc(new Qt4RunConfiguration(this, qt4proFile->path()));
            addRunConfiguration(newRc);
            m_isApplication = true;
        }
    }
}

void Qt4Project::checkForDeletedApplicationProjects()
{
    QStringList paths;
    foreach (Qt4ProFileNode * node, applicationProFiles())
        paths.append(node->path());

    qDebug()<<"Still existing paths :"<<paths;

    QList<QSharedPointer<Qt4RunConfiguration> > removeList;
    foreach (QSharedPointer<RunConfiguration> rc, runConfigurations()) {
        if (QSharedPointer<Qt4RunConfiguration> qt4rc = rc.dynamicCast<Qt4RunConfiguration>()) {
            if (!paths.contains(qt4rc->proFilePath())) {
                removeList.append(qt4rc);
                qDebug()<<"Removing runConfiguration for "<<qt4rc->proFilePath();
            }
        }
    }

    bool resetActiveRunConfiguration = false;
    QSharedPointer<RunConfiguration> rc(new ProjectExplorer::CustomExecutableRunConfiguration(this));
    foreach (QSharedPointer<Qt4RunConfiguration> qt4rc, removeList) {
        removeRunConfiguration(qt4rc);
        if (activeRunConfiguration() == qt4rc)
            resetActiveRunConfiguration = true;
    }

    if (runConfigurations().isEmpty()) {
        QSharedPointer<RunConfiguration> rc(new ProjectExplorer::CustomExecutableRunConfiguration(this));
        addRunConfiguration(rc);
        setActiveRunConfiguration(rc);
        m_isApplication = false;
    } else if (resetActiveRunConfiguration) {
        setActiveRunConfiguration(runConfigurations().first());
    }
}

QList<Qt4ProFileNode *> Qt4Project::applicationProFiles() const
{
    QList<Qt4ProFileNode *> list;
    collectApplicationProFiles(list, rootProjectNode());
    return list;
}

void Qt4Project::projectTypeChanged(Qt4ProFileNode *node, const Qt4ProjectType oldType, const Qt4ProjectType newType)
{
    if (oldType == Internal::ApplicationTemplate
        || oldType == Internal::ScriptTemplate) {
        // check wheter we need to delete a Run Configuration
        checkForDeletedApplicationProjects();
    }

    if (newType == Internal::ApplicationTemplate
        || newType == Internal::ScriptTemplate) {
        // add a new Run Configuration
        m_applicationProFileChange.clear();
        m_applicationProFileChange.append(node);
        checkForNewApplicationProjects();
    }
}

void Qt4Project::proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *node)
{
    foreach (QSharedPointer<RunConfiguration> rc, runConfigurations()) {
        if (QSharedPointer<Qt4RunConfiguration> qt4rc = rc.dynamicCast<Qt4RunConfiguration>()) {
            if (qt4rc->proFilePath() == node->path()) {
                qt4rc->updateCachedValues();
            }
        }
    }
}


QMakeStep *Qt4Project::qmakeStep() const
{
    QMakeStep *qs = 0;
    foreach(BuildStep *bs, buildSteps())
        if ( (qs = qobject_cast<QMakeStep *>(bs)) != 0)
            return qs;
    return 0;
}

MakeStep *Qt4Project::makeStep() const
{
    MakeStep *qs = 0;
    foreach(BuildStep *bs, buildSteps())
        if ((qs = qobject_cast<MakeStep *>(bs)) != 0)
            return qs;
    return 0;
}

bool Qt4Project::hasSubNode(Qt4PriFileNode *root, const QString &path)
{
    if (root->path() == path)
        return true;
    foreach (FolderNode *fn, root->subFolderNodes()) {
        if (qobject_cast<Qt4ProFileNode *>(fn)) {
            // we aren't interested in pro file nodes
        } else if (Qt4PriFileNode *qt4prifilenode = qobject_cast<Qt4PriFileNode *>(fn)) {
            if (hasSubNode(qt4prifilenode, path))
                return true;
        }
    }
    return false;
}

void Qt4Project::findProFile(const QString& fileName, Qt4ProFileNode *root, QList<Qt4ProFileNode *> &list)
{
    if (hasSubNode(root, fileName))
        list.append(root);

    foreach (FolderNode *fn, root->subFolderNodes())
        if (Qt4ProFileNode *qt4proFileNode =  qobject_cast<Qt4ProFileNode *>(fn))
            findProFile(fileName, qt4proFileNode, list);
}

void Qt4Project::notifyChanged(const QString &name)
{
    if (files(Qt4Project::ExcludeGeneratedFiles).contains(name)) {
        QList<Qt4ProFileNode *> list;
        findProFile(name, rootProjectNode(), list);
        foreach(Qt4ProFileNode *node, list)
            node->update();
    }
}
