// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeprojectconverter.h"
#include "cmakeprojectconverterdialog.h"
#include "generatecmakelists.h"
#include "generatecmakelistsconstants.h"
#include "../qmlprojectmanagertr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qmlprojectmanager/qmlprojectmanagerconstants.h>

#include <QAction>
#include <QMessageBox>
#include <QRegularExpression>

using namespace Utils;
using namespace QmlProjectManager::GenerateCmake::Constants;

namespace QmlProjectManager {
namespace GenerateCmake {

const QString MENU_ITEM_CONVERT = Tr::tr("Export as Latest Project Format...");
const QString ERROR_TITLE = Tr::tr("Creating Project");
const QString SUCCESS_TITLE = Tr::tr("Creating Project");
const QString ERROR_TEXT = Tr::tr("Creating project failed.\n%1");
const QString SUCCESS_TEXT = Tr::tr("Creating project succeeded.");

void CmakeProjectConverter::generateMenuEntry(QObject *parent)
{
    Core::ActionContainer *exportMenu = Core::ActionManager::actionContainer(
        QmlProjectManager::Constants::EXPORT_MENU);
    auto action = new QAction(MENU_ITEM_CONVERT, parent);
    QObject::connect(action, &QAction::triggered, CmakeProjectConverter::onConvertProject);
    Core::Command *cmd = Core::ActionManager::registerAction(action, "QmlProject.ConvertToCmakeProject");
    exportMenu->addAction(cmd, QmlProjectManager::Constants::G_EXPORT_CONVERT);

    action->setEnabled(isProjectConvertable(ProjectExplorer::ProjectManager::startupProject()));
    QObject::connect(ProjectExplorer::ProjectManager::instance(),
        &ProjectExplorer::ProjectManager::startupProjectChanged, [action]() {
            action->setEnabled(isProjectConvertable(ProjectExplorer::ProjectManager::startupProject()));
    });
}

bool CmakeProjectConverter::isProjectConvertable(const ProjectExplorer::Project *project)
{
    if (!project)
        return false;

    return !isProjectCurrentFormat(project);
}

const QStringList sanityCheckFiles({FILENAME_CMAKELISTS,
                                    FILENAME_MODULES,
                                    FILENAME_MAINQML,
                                    QString(DIRNAME_CONTENT)+'/'+FILENAME_CMAKELISTS,
                                    QString(DIRNAME_IMPORT)+'/'+FILENAME_CMAKELISTS,
                                    QString(DIRNAME_CPP)+'/'+FILENAME_MAINCPP,
                                    QString(DIRNAME_CPP)+'/'+FILENAME_ENV_HEADER,
                                    QString(DIRNAME_CPP)+'/'+FILENAME_MAINCPP_HEADER
                                   });

bool CmakeProjectConverter::isProjectCurrentFormat(const ProjectExplorer::Project *project)
{
    const QmlProjectManager::QmlProject *qmlprj = qobject_cast<const QmlProjectManager::QmlProject*>(project);

    if (!qmlprj)
        return false;

    FilePath rootDir = qmlprj->rootProjectDirectory();
    for (const QString &file : sanityCheckFiles)
        if (!rootDir.pathAppended(file).exists())
            return false;

    return true;
}

void CmakeProjectConverter::onConvertProject()
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    const QmlProjectManager::QmlProject *qmlProject =
            qobject_cast<const QmlProjectManager::QmlProject*>(project);
    if (qmlProject) {
        CmakeProjectConverterDialog dialog(qmlProject);
        if (dialog.exec()) {
            FilePath newProjectPath = dialog.newPath();
            CmakeProjectConverter converter;
            converter.convertProject(qmlProject, newProjectPath);
        }
    }
}

bool CmakeProjectConverter::convertProject(const QmlProjectManager::QmlProject *project,
                                           const FilePath &targetDir)
{
    m_converterObjects.clear();
    m_projectDir = project->projectDirectory();
    m_newProjectDir = targetDir;
    m_project = project;

    m_rootDirFiles = QStringList(FILENAME_FILTER_QMLPROJECT);
    const QString confFile = GenerateCmake::projectEnvironmentVariable(ENV_VARIABLE_CONTROLCONF);
    if (!confFile.isEmpty())
        m_rootDirFiles.append(confFile);

    bool retVal = prepareAndExecute();

    if (retVal) {
        QMessageBox::information(Core::ICore::dialogParent(), SUCCESS_TITLE, SUCCESS_TEXT);
        ProjectExplorer::ProjectExplorerPlugin::OpenProjectResult result
                = ProjectExplorer::ProjectExplorerPlugin::openProject(newProjectFile());
        if (!result)
            ProjectExplorer::ProjectExplorerPlugin::showOpenProjectError(result);
    }
    else {
        QMessageBox::critical(Core::ICore::dialogParent(), ERROR_TITLE, ERROR_TEXT.arg(m_errorText));
    }

    return retVal;
}

bool CmakeProjectConverter::prepareAndExecute()
{
    GenerateCmake::CmakeFileGenerator cmakeGenerator;

    if (!performSanityCheck())
        return false;
    if (!prepareBaseDirectoryStructure())
        return false;
    if (!prepareCopy())
        return false;
    if (!createPreparedProject())
        return false;
    if (!cmakeGenerator.prepare(m_newProjectDir, false))
        return false;
    if (!cmakeGenerator.execute())
        return false;
    if (!modifyNewFiles())
        return false;

    return true;
}

bool CmakeProjectConverter::isFileBlacklisted(const Utils::FilePath &file) const
{
    if (!file.fileName().compare(FILENAME_CMAKELISTS))
        return true;
    if (!file.suffix().compare(FILENAME_SUFFIX_QMLPROJECT))
        return true;
    if (!file.suffix().compare(FILENAME_SUFFIX_USER))
        return true;
    if (m_rootDirFiles.contains(file.fileName()))
        return true;

    return false;
}

bool CmakeProjectConverter::isDirBlacklisted(const Utils::FilePath &dir) const
{
    if (!dir.isDir())
        return true;

    return false;
}

const QString ERROR_CANNOT_WRITE_DIR = Tr::tr("Unable to write to directory\n%1.");

bool CmakeProjectConverter::performSanityCheck()
{
    if (!m_newProjectDir.parentDir().isWritableDir()) {
        m_errorText = ERROR_CANNOT_WRITE_DIR.arg(m_newProjectDir.parentDir().toString());
        return false;
    }

    return true;
}

bool CmakeProjectConverter::prepareBaseDirectoryStructure()
{
    addDirectory(m_newProjectDir);
    addDirectory(contentDir());
    addDirectory(sourceDir());
    addDirectory(importDir());
    addDirectory(assetDir());
    addDirectory(assetImportDir());
    addFile(contentDir().pathAppended(FILENAME_APPMAINQML));

    return true;
}

bool CmakeProjectConverter::prepareCopy()
{
    FilePaths rootFiles = m_projectDir.dirEntries({m_rootDirFiles, QDir::Files});
    for (const FilePath &file : rootFiles) {
        addFile(file, m_newProjectDir.pathAppended(file.fileName()));
    }

    prepareCopyDirFiles(m_projectDir, contentDir());

    FilePaths subDirs = m_projectDir.dirEntries(QDir::Dirs|QDir::NoDotAndDotDot);
    for (FilePath &subDir : subDirs) {
        if (subDir.fileName() == DIRNAME_IMPORT) {
            prepareCopyDirTree(subDir, importDir());
        }
        else if (subDir.fileName() == DIRNAME_CPP) {
            prepareCopyDirTree(subDir, sourceDir());
        }
        else if (subDir.fileName() == DIRNAME_ASSET) {
            prepareCopyDirTree(subDir, assetDir());
        }
        else if (subDir.fileName() == DIRNAME_ASSETIMPORT) {
            prepareCopyDirTree(subDir, assetImportDir());
        }
        else {
            prepareCopyDirTree(subDir, contentDir().pathAppended(subDir.fileName()));
        }
    }

    return true;
}

bool CmakeProjectConverter::prepareCopyDirFiles(const FilePath &dir, const FilePath &targetDir)
{
    FilePaths dirFiles = dir.dirEntries(QDir::Files);
    for (FilePath file : dirFiles) {
        if (!isFileBlacklisted(file))
            addFile(file, targetDir.pathAppended(file.fileName()));
    }

    return true;
}

bool CmakeProjectConverter::prepareCopyDirTree(const FilePath &dir, const FilePath &targetDir)
{
    prepareCopyDirFiles(dir, targetDir);
    FilePaths subDirs = dir.dirEntries(QDir::Dirs|QDir::NoDotAndDotDot);
    for (FilePath &subDir : subDirs) {
        if (isDirBlacklisted(subDir))
            continue;
        addDirectory(targetDir.pathAppended(subDir.fileName()));
        prepareCopyDirFiles(subDir, targetDir.pathAppended(subDir.fileName()));
        prepareCopyDirTree(subDir, targetDir.pathAppended(subDir.fileName()));
    }

    return true;
}

bool CmakeProjectConverter::addDirectory(const Utils::FilePath &target)
{
    return addObject(ProjectConverterObjectType::Directory, FilePath(), target);
}

bool CmakeProjectConverter::addFile(const Utils::FilePath &target)
{
    return addFile(FilePath(), target);
}

bool CmakeProjectConverter::addFile(const Utils::FilePath &original, const Utils::FilePath &target)
{
    addDirectory(target.parentDir());
    return addObject(ProjectConverterObjectType::File, original, target);
}

bool CmakeProjectConverter::addObject(ProjectConverterObjectType type,
                                      const Utils::FilePath &original, const Utils::FilePath &target)
{
    if (target.isChildOf(m_projectDir))
        return false;

    if (!target.isChildOf(m_newProjectDir) &&
            ((type == ProjectConverterObjectType::Directory) && (target != m_newProjectDir))) {
        return false;
    }

    for (ProjectConverterObject &o : m_converterObjects) {
        if (o.target == target)
            return false;
    }

    ProjectConverterObject object;
    object.type = type;
    object.target = target;
    object.original = original;

    m_converterObjects.append(object);

    return true;
}

bool CmakeProjectConverter::createPreparedProject()
{
    for (ProjectConverterObject &pco : m_converterObjects) {
        if (pco.type == ProjectConverterObjectType::Directory) {
            pco.target.createDir();
        }
        else if (pco.type == ProjectConverterObjectType::File) {
            if (pco.original.isEmpty()) {
                QFile newFile(pco.target.toString());
                newFile.open(QIODevice::WriteOnly);
                newFile.close();
            }
            else {
                pco.original.copyFile(pco.target);
            }
        }
    }

    return true;
}

const FilePath CmakeProjectConverter::contentDir() const
{
    return m_newProjectDir.pathAppended(DIRNAME_CONTENT);
}

const FilePath CmakeProjectConverter::sourceDir() const
{
    return m_newProjectDir.pathAppended(DIRNAME_CPP);
}

const FilePath CmakeProjectConverter::importDir() const
{
    return m_newProjectDir.pathAppended(DIRNAME_IMPORT);
}

const FilePath CmakeProjectConverter::assetDir() const
{
    return contentDir().pathAppended(DIRNAME_ASSET);
}

const FilePath CmakeProjectConverter::assetImportDir() const
{
    return m_newProjectDir.pathAppended(DIRNAME_ASSETIMPORT);
}

const FilePath CmakeProjectConverter::newProjectFile() const
{
    return m_newProjectDir.pathAppended(m_project->projectFilePath().fileName());
}

const FilePath CmakeProjectConverter::projectMainFile() const
{
    auto *target = m_project->activeTarget();
    if (target && target->buildSystem()) {
        auto buildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(target->buildSystem());
        if (buildSystem) {
            return buildSystem->mainFilePath();
        }
    }
    return {};
}

const QString CmakeProjectConverter::projectMainClass() const
{
    return projectMainFile().baseName();
}

bool CmakeProjectConverter::modifyNewFiles()
{
    return modifyAppMainQml() && modifyProjectFile();
}

const char APPMAIN_QMLFILE_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectappmainqml.tpl";

bool CmakeProjectConverter::modifyAppMainQml()
{
    QString appMainQmlPath = contentDir().pathAppended(FILENAME_APPMAINQML).toString();
    QFile appMainQml(appMainQmlPath);
    appMainQml.open(QIODevice::ReadWrite);
    if (!appMainQml.isOpen())
        return false;

    QString templateContent = GenerateCmake::readTemplate(APPMAIN_QMLFILE_TEMPLATE_PATH);
    QString appMainQmlContent = templateContent.arg(projectMainClass());

    appMainQml.reset();
    appMainQml.write(appMainQmlContent.toUtf8());
    appMainQml.close();

    return true;
}

bool CmakeProjectConverter::modifyProjectFile()
{
    QString projectFileName = m_project->projectFilePath().fileName();
    FilePath projectFilePath = m_newProjectDir.pathAppended(projectFileName);
    QFile projectFile(projectFilePath.toString());
    projectFile.open(QIODevice::ReadOnly);
    if (!projectFile.isOpen())
        return false;
    QString projectFileContent = QString::fromUtf8(projectFile.readAll());
    projectFile.close();

    const QRegularExpression mainFilePattern("^\\s*mainFile:\\s*\".*\"", QRegularExpression::MultilineOption);
    const QString mainFileString("    mainFile: \"content/App.qml\"");

    projectFileContent.replace(mainFilePattern, mainFileString);

    projectFile.open(QIODevice::WriteOnly|QIODevice::Truncate);
    if (!projectFile.isOpen())
        return false;
    projectFile.write(projectFileContent.toUtf8());
    projectFile.close();

    return true;
}

} //GenerateCmake
} //QmlProjectManager
