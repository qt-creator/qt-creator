/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "generatecmakelists.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <qmlprojectmanager/qmlprojectmanagerconstants.h>

#include <utils/fileutils.h>

#include <QAction>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

using namespace Utils;

namespace QmlDesigner {
namespace GenerateCmakeLists {

const QDir::Filters FILES_ONLY = QDir::Files;
const QDir::Filters DIRS_ONLY = QDir::Dirs|QDir::Readable|QDir::NoDotAndDotDot;

const char CMAKEFILENAME[] = "CMakeLists.txt";
const char QMLDIRFILENAME[] = "qmldir";

void generateMenuEntry()
{
    Core::ActionContainer *buildMenu =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    const Core::Context projectCntext(QmlProjectManager::Constants::QML_PROJECT_ID);
    auto action = new QAction("Generate CMakeLists.txt files");
    QObject::connect(action, &QAction::triggered, GenerateCmakeLists::onGenerateCmakeLists);
    Core::Command *cmd = Core::ActionManager::registerAction(action, "QmlProject.CreateCMakeLists");
    buildMenu->addAction(cmd, ProjectExplorer::Constants::G_BUILD_RUN);

    action->setEnabled(ProjectExplorer::SessionManager::startupProject() != nullptr);
    QObject::connect(ProjectExplorer::SessionManager::instance(),
        &ProjectExplorer::SessionManager::startupProjectChanged, [action]() {
            action->setEnabled(ProjectExplorer::SessionManager::startupProject() != nullptr);
    });
}

void onGenerateCmakeLists()
{
    generateMainCmake(ProjectExplorer::SessionManager::startupProject()->projectDirectory());
}

QStringList processDirectory(const FilePath &dir)
{
    QStringList moduleNames;

    FilePaths files = dir.dirEntries(FILES_ONLY);
    for (FilePath &file : files) {
        if (!file.fileName().compare(CMAKEFILENAME))
            files.removeAll(file);
    }

    if (files.isEmpty()) {
        generateSubdirCmake(dir);
        FilePaths subDirs = dir.dirEntries(DIRS_ONLY);
        for (FilePath &subDir : subDirs) {
            QStringList subDirModules = processDirectory(subDir);
            moduleNames.append(subDirModules);
        }
    }
    else {
        QString moduleName = generateModuleCmake(dir);
        if (!moduleName.isEmpty()) {
            moduleNames.append(moduleName);
        }
    }

    return moduleNames;
}

const char MAINFILE_REQUIRED_VERSION[] = "cmake_minimum_required(VERSION 3.18)\n\n";
const char MAINFILE_PROJECT[] = "project(%1 LANGUAGES CXX)\n\n";
const char MAINFILE_CMAKE_OPTIONS[] = "set(CMAKE_INCLUDE_CURRENT_DIR ON)\nset(CMAKE_AUTOMOC ON)\n\n";
const char MAINFILE_PACKAGES[] = "find_package(Qt6 COMPONENTS Gui Qml Quick)\n";
const char MAINFILE_LIBRARIES[] = "set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)\n\n";
const char MAINFILE_CPP[] = "add_executable(%1 main.cpp)\n\n";
const char MAINFILE_MAINMODULE[] = "qt6_add_qml_module(%1\n\tURI \"Main\"\n\tVERSION 1.0\n\tNO_PLUGIN\n\tQML_FILES main.qml\n)\n\n";
const char MAINFILE_LINK_LIBRARIES[] = "target_link_libraries(%1 PRIVATE\n\tQt${QT_VERSION_MAJOR}::Core\n\tQt${QT_VERSION_MAJOR}::Gui\n\tQt${QT_VERSION_MAJOR}::Quick\n\tQt${QT_VERSION_MAJOR}::Qml\n)\n\n";

const char ADD_SUBDIR[] = "add_subdirectory(%1)\n";

void generateMainCmake(const FilePath &rootDir)
{
    //TODO startupProject() may be a terrible way to try to get "current project". It's not necessarily the same thing at all.
    QString projectName = ProjectExplorer::SessionManager::startupProject()->displayName();

    FilePaths subDirs = rootDir.dirEntries(DIRS_ONLY);

    QString fileContent;
    fileContent.append(MAINFILE_REQUIRED_VERSION);
    fileContent.append(QString(MAINFILE_PROJECT).arg(projectName));
    fileContent.append(MAINFILE_CMAKE_OPTIONS);
    fileContent.append(MAINFILE_PACKAGES);
    fileContent.append(QString(MAINFILE_CPP).arg(projectName));
    fileContent.append(QString(MAINFILE_MAINMODULE).arg(projectName));
    fileContent.append(MAINFILE_LIBRARIES);

    for (FilePath &subDir : subDirs) {
        QStringList subDirModules = processDirectory(subDir);
        if (!subDirModules.isEmpty())
            fileContent.append(QString(ADD_SUBDIR).arg(subDir.fileName()));
    }
    fileContent.append("\n");

    fileContent.append(QString(MAINFILE_LINK_LIBRARIES).arg(projectName));

    createCmakeFile(rootDir, fileContent);
}

const char MODULEFILE_PROPERTY_SINGLETON[] = "QT_QML_SINGLETON_TYPE";
const char MODULEFILE_PROPERTY_SET[] = "set_source_files_properties(%1\n\tPROPERTIES\n\t\t%2 %3\n)\n\n";
const char MODULEFILE_CREATE_MODULE[] = "qt6_add_qml_module(%1\n\tURI \"%1\"\n\tVERSION 1.0\n%2)\n\n";


QString generateModuleCmake(const FilePath &dir)
{
    QString fileContent;
    const QStringList qmlFilesOnly("*.qml");
    const QStringList qmldirFilesOnly(QMLDIRFILENAME);
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();

    FilePaths qmldirFileList = dir.dirEntries(qmldirFilesOnly, FILES_ONLY);
    if (!qmldirFileList.isEmpty()) {
        QStringList singletons = getSingletonsFromQmldirFile(qmldirFileList.first());
        for (QString &singleton : singletons) {
            fileContent.append(QString(MODULEFILE_PROPERTY_SET).arg(singleton).arg(MODULEFILE_PROPERTY_SINGLETON).arg("true"));
        }
    }

    FilePaths qmlFileList = dir.dirEntries(qmlFilesOnly, FILES_ONLY);
    QString qmlFiles;
    for (FilePath &qmlFile : qmlFileList) {
        if (project->isKnownFile(qmlFile))
            qmlFiles.append(QString("\t\t%1\n").arg(qmlFile.fileName()));
    }

    QStringList resourceFileList = getDirectoryTreeResources(dir);
    QString resourceFiles;
    for (QString &resourceFile : resourceFileList) {
        resourceFiles.append(QString("\t\t%1\n").arg(resourceFile));
    }

    QString moduleContent;
    if (!qmlFiles.isEmpty()) {
        moduleContent.append(QString("\tQML_FILES\n%1").arg(qmlFiles));
    }
    if (!resourceFiles.isEmpty()) {
        moduleContent.append(QString("\tRESOURCES\n%1").arg(resourceFiles));
    }

    QString moduleName = dir.fileName();

    fileContent.append(QString(MODULEFILE_CREATE_MODULE).arg(moduleName).arg(moduleContent));

    createCmakeFile(dir, fileContent);

    return moduleName;
}

void generateSubdirCmake(const FilePath &dir)
{
    QString fileContent;
    FilePaths subDirs = dir.dirEntries(DIRS_ONLY);

    for (FilePath &subDir : subDirs) {
        fileContent.append(QString(ADD_SUBDIR).arg(subDir.fileName()));
    }

    createCmakeFile(dir, fileContent);
}

QStringList getSingletonsFromQmldirFile(const FilePath &filePath)
{
    QStringList singletons;
    QFile f(filePath.toString());
    f.open(QIODevice::ReadOnly);
    QTextStream stream(&f);

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (line.startsWith("singleton", Qt::CaseInsensitive)) {
            QStringList tokenizedLine = line.split(QRegularExpression("\\s+"));
            QString fileName = tokenizedLine.last();
            if (fileName.endsWith(".qml", Qt::CaseInsensitive)) {
                singletons.append(fileName);
            }
        }
    }

    f.close();

    return singletons;
}

QStringList getDirectoryTreeResources(const FilePath &dir)
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QStringList resourceFileList;

    FilePaths thisDirFiles = dir.dirEntries(FILES_ONLY);
    for (FilePath &file : thisDirFiles) {
        if (!isFileBlacklisted(file.fileName()) &&
            !file.fileName().endsWith(".qml", Qt::CaseInsensitive) &&
            project->isKnownFile(file)) {
            resourceFileList.append(file.fileName());
        }
    }

    FilePaths subDirsList = dir.dirEntries(DIRS_ONLY);
    for (FilePath &subDir : subDirsList) {
        QStringList subDirResources = getDirectoryTreeResources(subDir);
        for (QString &resource : subDirResources) {
            resourceFileList.append(subDir.fileName().append('/').append(resource));
        }

    }

    return resourceFileList;
}

void createCmakeFile(const FilePath &dir, const QString &content)
{
    FilePath filePath = dir.pathAppended(CMAKEFILENAME);
    QFile cmakeFile(filePath.toString());
    cmakeFile.open(QIODevice::WriteOnly);
    QTextStream stream(&cmakeFile);
    stream << content;
    cmakeFile.close();
}

bool isFileBlacklisted(const QString &fileName)
{
    return (!fileName.compare(QMLDIRFILENAME) ||
            !fileName.compare(CMAKEFILENAME));
}

}
}
