/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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
#include "cmakeprojectconverter.h"
#include "cmakeprojectconverterdialog.h"
#include "generatecmakelists.h"
#include "generatecmakelistsconstants.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>

#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <QAction>
#include <QMessageBox>
#include <QRegularExpression>

using namespace Utils;
using namespace QmlDesigner::GenerateCmake::Constants;

namespace QmlDesigner {
namespace GenerateCmake {

const QString MENU_ITEM_CONVERT = QCoreApplication::translate("QmlDesigner::CmakeProjectConverter",
                                                      "Export as Latest Project Format");
const QString ERROR_TITLE = QCoreApplication::translate("QmlDesigner::CmakeProjectConverter",
                                                        "Creating Project");
const QString SUCCESS_TITLE = QCoreApplication::translate("QmlDesigner::CmakeProjectConverter",
                                                          "Creating Project");
const QString ERROR_TEXT = QCoreApplication::translate("QmlDesigner::CmakeProjectConverter",
                                                       "Creating project failed.\n%1");
const QString SUCCESS_TEXT = QCoreApplication::translate("QmlDesigner::CmakeProjectConverter",
                                                         "Creating project succeeded.");

void CmakeProjectConverter::generateMenuEntry()
{
    Core::ActionContainer *menu =
            Core::ActionManager::actionContainer(Core::Constants::M_FILE);
    auto action = new QAction(MENU_ITEM_CONVERT);
    QObject::connect(action, &QAction::triggered, CmakeProjectConverter::onConvertProject);
    Core::Command *cmd = Core::ActionManager::registerAction(action, "QmlProject.ConvertToCmakeProject");
    menu->addAction(cmd, Core::Constants::G_FILE_EXPORT);

    action->setEnabled(ProjectExplorer::SessionManager::startupProject() != nullptr);
    QObject::connect(ProjectExplorer::SessionManager::instance(),
        &ProjectExplorer::SessionManager::startupProjectChanged, [action]() {
            action->setEnabled(ProjectExplorer::SessionManager::startupProject() != nullptr);
    });
}

void CmakeProjectConverter::onConvertProject()
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
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

    if (retVal)
        QMessageBox::information(nullptr, SUCCESS_TITLE, SUCCESS_TEXT);
    else
        QMessageBox::critical(nullptr, ERROR_TITLE, ERROR_TEXT.arg(m_errorText));

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

const QString ERROR_CANNOT_WRITE_DIR = QCoreApplication::translate("QmlDesigner::CmakeProjectConverter",
                                                                   "Unable to write to directory\n%1.");

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

const FilePath CmakeProjectConverter::contentDir()
{
    return m_newProjectDir.pathAppended(DIRNAME_CONTENT);
}

const FilePath CmakeProjectConverter::sourceDir()
{
    return m_newProjectDir.pathAppended(DIRNAME_CPP);
}

const FilePath CmakeProjectConverter::importDir()
{
    return m_newProjectDir.pathAppended(DIRNAME_IMPORT);
}

const FilePath CmakeProjectConverter::assetDir()
{
    return contentDir().pathAppended(DIRNAME_ASSET);
}

const FilePath CmakeProjectConverter::assetImportDir()
{
    return m_newProjectDir.pathAppended(DIRNAME_ASSETIMPORT);
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
    projectFile.open(QIODevice::ReadWrite);
    if (!projectFile.isOpen())
        return false;

    QString projectFileContent = QString::fromUtf8(projectFile.readAll());
    const QRegularExpression mainFilePattern("^\\s*mainFile:\\s*\".*\"", QRegularExpression::MultilineOption);
    const QString mainFileString("    mainFile: \"content/App.qml\"");

    projectFileContent.replace(mainFilePattern, mainFileString);

    projectFile.reset();
    projectFile.write(projectFileContent.toUtf8());
    projectFile.close();

    return true;
}

} //GenerateCmake
} //QmlDesigner
