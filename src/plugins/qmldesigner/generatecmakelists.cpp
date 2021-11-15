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
#include "cmakegeneratordialog.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/session.h>

#include <qmlprojectmanager/qmlprojectmanagerconstants.h>
#include <qmlprojectmanager/qmlmainfileaspect.h>

#include <utils/fileutils.h>

#include <QAction>
#include <QtConcurrent>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

using namespace Utils;

namespace QmlDesigner {

namespace GenerateCmake {

bool operator==(const GeneratableFile &left, const GeneratableFile &right)
{
    return (left.filePath == right.filePath && left.content == right.content);
}

QVector<GeneratableFile> queuedFiles;

void generateMenuEntry()
{
    Core::ActionContainer *buildMenu =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    auto action = new QAction("Generate CMakeLists.txt files");
    QObject::connect(action, &QAction::triggered, GenerateCmake::onGenerateCmakeLists);
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
    queuedFiles.clear();
    FilePath rootDir = ProjectExplorer::SessionManager::startupProject()->projectDirectory();
    GenerateCmakeLists::generateCmakes(rootDir);
    GenerateEntryPoints::generateMainCpp(rootDir);
    GenerateEntryPoints::generateMainQml(rootDir);
    if (showConfirmationDialog(rootDir))
        writeQueuedFiles();
}

void removeUnconfirmedQueuedFiles(const Utils::FilePaths confirmedFiles)
{
    QtConcurrent::blockingFilter(queuedFiles, [confirmedFiles](const GeneratableFile &qf) {
        return confirmedFiles.contains(qf.filePath);
    });
}

bool showConfirmationDialog(const Utils::FilePath &rootDir)
{
    Utils::FilePaths files;
    for (GeneratableFile &file: queuedFiles)
        files.append(file.filePath);

    CmakeGeneratorDialog dialog(rootDir, files);
    if (dialog.exec()) {
        Utils::FilePaths confirmedFiles = dialog.getFilePaths();
        removeUnconfirmedQueuedFiles(confirmedFiles);

        return true;
    }

    return false;
}

bool queueFile(const FilePath &filePath, const QString &fileContent)
{
    GeneratableFile file;
    file.filePath = filePath;
    file.content = fileContent;
    queuedFiles.append(file);

    return true;
}

bool writeQueuedFiles()
{
    for (GeneratableFile &file: queuedFiles)
        if (!writeFile(file))
            return false;

    return true;
}

bool writeFile(const GeneratableFile &file)
{
    QFile fileHandle(file.filePath.toString());
    fileHandle.open(QIODevice::WriteOnly);
    QTextStream stream(&fileHandle);
    stream << file.content;
    fileHandle.close();

    return true;
}

QString readTemplate(const QString &templatePath)
{
    QFile templatefile(templatePath);
    templatefile.open(QIODevice::ReadOnly);
    QTextStream stream(&templatefile);
    QString content = stream.readAll();
    templatefile.close();

    return content;
}

}

namespace GenerateCmakeLists {

QStringList moduleNames;

const QDir::Filters FILES_ONLY = QDir::Files;
const QDir::Filters DIRS_ONLY = QDir::Dirs|QDir::Readable|QDir::NoDotAndDotDot;

const char CMAKEFILENAME[] = "CMakeLists.txt";
const char QMLDIRFILENAME[] = "qmldir";
const char MODULEFILENAME[] = "qmlmodules";

bool generateCmakes(const FilePath &rootDir)
{
    moduleNames.clear();

    FilePath contentDir = rootDir.pathAppended("content");
    FilePath importDir = rootDir.pathAppended("imports");

    generateModuleCmake(contentDir);
    generateImportCmake(importDir);
    generateMainCmake(rootDir);

    return true;
}

const char MAIN_CMAKEFILE_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmaincmakelists.tpl";
const char QMLMODULES_FILE_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmodules.tpl";

void generateMainCmake(const FilePath &rootDir)
{
    //TODO startupProject() may be a terrible way to try to get "current project". It's not necessarily the same thing at all.
    QString projectName = ProjectExplorer::SessionManager::startupProject()->displayName();
    QString appName = projectName + "App";

    QString cmakeFileContent = GenerateCmake::readTemplate(MAIN_CMAKEFILE_TEMPLATE_PATH).arg(appName);
    queueCmakeFile(rootDir, cmakeFileContent);

    QString modulesAsPlugins;
    for (const QString &moduleName : moduleNames)
        modulesAsPlugins.append("    " + moduleName + "plugin\n");

    QString moduleFileContent = GenerateCmake::readTemplate(QMLMODULES_FILE_TEMPLATE_PATH).arg(appName).arg(modulesAsPlugins);
    GenerateCmake::queueFile(rootDir.pathAppended(MODULEFILENAME), moduleFileContent);
}

const char DO_NOT_EDIT_FILE_COMMENT[] = "### This file is automatically generated by Qt Design Studio.\n### Do not change\n\n";
const char ADD_SUBDIR[] = "add_subdirectory(%1)\n";

void generateImportCmake(const FilePath &dir)
{
    QString fileContent;

    fileContent.append(DO_NOT_EDIT_FILE_COMMENT);

    FilePaths subDirs = dir.dirEntries(DIRS_ONLY);
    for (FilePath &subDir : subDirs) {
        fileContent.append(QString(ADD_SUBDIR).arg(subDir.fileName()));
        generateModuleCmake(subDir);
    }

    queueCmakeFile(dir, fileContent);
}

const char MODULEFILE_PROPERTY_SINGLETON[] = "QT_QML_SINGLETON_TYPE";
const char MODULEFILE_PROPERTY_SET[] = "set_source_files_properties(%1\n    PROPERTIES\n        %2 %3\n)\n\n";
const char MODULEFILE_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmodulecmakelists.tpl";

void generateModuleCmake(const FilePath &dir)
{
    QString fileTemplate = GenerateCmake::readTemplate(MODULEFILE_TEMPLATE_PATH);
    const QStringList qmldirFilesOnly(QMLDIRFILENAME);

    QString singletonContent;
    FilePaths qmldirFileList = dir.dirEntries(qmldirFilesOnly, FILES_ONLY);
    if (!qmldirFileList.isEmpty()) {
        QStringList singletons = getSingletonsFromQmldirFile(qmldirFileList.first());
        for (QString &singleton : singletons) {
            singletonContent.append(QString(MODULEFILE_PROPERTY_SET).arg(singleton).arg(MODULEFILE_PROPERTY_SINGLETON).arg("true"));
        }
    }

    QStringList qmlFileList = getDirectoryTreeQmls(dir);
    QString qmlFiles;
    for (QString &qmlFile : qmlFileList)
        qmlFiles.append(QString("        %1\n").arg(qmlFile));

    QStringList resourceFileList = getDirectoryTreeResources(dir);
    QString resourceFiles;
    for (QString &resourceFile : resourceFileList)
        resourceFiles.append(QString("        %1\n").arg(resourceFile));

    QString moduleContent;
    if (!qmlFiles.isEmpty()) {
        moduleContent.append(QString("    QML_FILES\n%1").arg(qmlFiles));
    }
    if (!resourceFiles.isEmpty()) {
        moduleContent.append(QString("    RESOURCES\n%1").arg(resourceFiles));
    }

    QString moduleName = dir.fileName();

    moduleNames.append(moduleName);

    QString fileContent;
    fileContent.append(fileTemplate.arg(singletonContent).arg(moduleName).arg(moduleContent));
    queueCmakeFile(dir, fileContent);
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

QStringList getDirectoryTreeQmls(const FilePath &dir)
{
    const QStringList qmlFilesOnly("*.qml");
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QStringList qmlFileList;

    FilePaths thisDirFiles = dir.dirEntries(qmlFilesOnly, FILES_ONLY);
    for (FilePath &file : thisDirFiles) {
        if (!isFileBlacklisted(file.fileName()) &&
            project->isKnownFile(file)) {
            qmlFileList.append(file.fileName());
        }
    }

    FilePaths subDirsList = dir.dirEntries(DIRS_ONLY);
    for (FilePath &subDir : subDirsList) {
        QStringList subDirQmlFiles = getDirectoryTreeQmls(subDir);
        for (QString &qmlFile : subDirQmlFiles) {
            qmlFileList.append(subDir.fileName().append('/').append(qmlFile));
        }
    }

    return qmlFileList;
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

void queueCmakeFile(const FilePath &dir, const QString &content)
{
    FilePath filePath = dir.pathAppended(CMAKEFILENAME);
    GenerateCmake::queueFile(filePath, content);
}

bool isFileBlacklisted(const QString &fileName)
{
    return (!fileName.compare(QMLDIRFILENAME) ||
            !fileName.compare(CMAKEFILENAME));
}

}

namespace GenerateEntryPoints {
bool generateEntryPointFiles(const FilePath &dir)
{
    bool cppOk = generateMainCpp(dir);
    bool qmlOk = generateMainQml(dir);

    return cppOk && qmlOk;
}

const char MAIN_CPPFILE_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmaincpp.tpl";
const char MAIN_CPPFILE_DIR[] = "src";
const char MAIN_CPPFILE_NAME[] = "main.cpp";
const char MAIN_CPPFILE_HEADER_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmaincppheader.tpl";
const char MAIN_CPPFILE_HEADER_NAME[] = "import_qml_plugins.h";
const char MAIN_CPPFILE_HEADER_PLUGIN_LINE[] = "Q_IMPORT_QML_PLUGIN(%1)\n";

bool generateMainCpp(const FilePath &dir)
{
    FilePath srcDir = dir.pathAppended(MAIN_CPPFILE_DIR);

    QString cppContent = GenerateCmake::readTemplate(MAIN_CPPFILE_TEMPLATE_PATH);
    FilePath cppFilePath = srcDir.pathAppended(MAIN_CPPFILE_NAME);
    bool cppOk = GenerateCmake::queueFile(cppFilePath, cppContent);

    QString modulesAsPlugins;
    for (const QString &moduleName : GenerateCmakeLists::moduleNames)
        modulesAsPlugins.append(
                    QString(MAIN_CPPFILE_HEADER_PLUGIN_LINE).arg(moduleName + "plugin"));

    QString headerContent = GenerateCmake::readTemplate(MAIN_CPPFILE_HEADER_TEMPLATE_PATH)
            .arg(modulesAsPlugins);
    FilePath headerFilePath = srcDir.pathAppended(MAIN_CPPFILE_HEADER_NAME);
    bool headerOk = GenerateCmake::queueFile(headerFilePath, headerContent);

    return cppOk && headerOk;
}

const char MAIN_QMLFILE_PATH[] = ":/boilerplatetemplates/qmlprojectmainqml.tpl";
const char MAIN_QMLFILE_NAME[] = "main.qml";

bool generateMainQml(const FilePath &dir)
{
    QString content = GenerateCmake::readTemplate(MAIN_QMLFILE_PATH);
    FilePath filePath = dir.pathAppended(MAIN_QMLFILE_NAME);
    return GenerateCmake::queueFile(filePath, content);
}

}

}

