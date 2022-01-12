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
#include "generatecmakelistsconstants.h"
#include "cmakegeneratordialog.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qmlprojectmanager/qmlproject.h>
#include <qmlprojectmanager/qmlprojectmanagerconstants.h>
#include <qmlprojectmanager/qmlmainfileaspect.h>

#include <utils/fileutils.h>

#include <QAction>
#include <QMessageBox>
#include <QtConcurrent>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

using namespace Utils;
using namespace QmlDesigner::GenerateCmake::Constants;

namespace QmlDesigner {

namespace GenerateCmake {

bool operator==(const GeneratableFile &left, const GeneratableFile &right)
{
    return (left.filePath == right.filePath && left.content == right.content);
}

enum ProjectDirectoryError {
    NoError = 0,
    MissingContentDir = 1<<1,
    MissingImportDir = 1<<2,
    MissingAssetImportDir = 1<<3,
    MissingCppDir = 1<<4,
    MissingMainCMake = 1<<5,
    MissingMainQml = 1<<6,
    MissingAppMainQml = 1<<7,
    MissingQmlModules = 1<<8,
    MissingMainCpp = 1<<9,
    MissingMainCppHeader = 1<<10,
    MissingEnvHeader = 1<<11
};

QVector<GeneratableFile> queuedFiles;

void generateMenuEntry()
{
    Core::ActionContainer *buildMenu =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    auto action = new QAction(QCoreApplication::translate("QmlDesigner::GenerateCmake", "Generate CMakeLists.txt Files"));
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
    FilePath rootDir = ProjectExplorer::SessionManager::startupProject()->projectDirectory();

    int projectDirErrors = isProjectCorrectlyFormed(rootDir);
    if (projectDirErrors != NoError) {
        showProjectDirErrorDialog(projectDirErrors);
        if (isErrorFatal(projectDirErrors))
            return;
    }

    queuedFiles.clear();
    GenerateCmakeLists::generateCmakes(rootDir);
    GenerateEntryPoints::generateEntryPointFiles(rootDir);
    if (showConfirmationDialog(rootDir))
        writeQueuedFiles();
}

bool isErrorFatal(int error)
{
    if (error & MissingContentDir ||
        error & MissingImportDir ||
        error & MissingCppDir ||
        error & MissingAppMainQml)
        return true;

    return false;
}

int isProjectCorrectlyFormed(const FilePath &rootDir)
{
    int errors = NoError;

    if (!rootDir.pathAppended(DIRNAME_CONTENT).exists())
        errors |= MissingContentDir;
    if (!rootDir.pathAppended(DIRNAME_CONTENT).pathAppended(FILENAME_APPMAINQML).exists())
        errors |= MissingAppMainQml;

    if (!rootDir.pathAppended(DIRNAME_IMPORT).exists())
        errors |= MissingImportDir;
    if (!rootDir.pathAppended(DIRNAME_ASSET).exists())
        errors |= MissingAssetImportDir;

    if (!rootDir.pathAppended(DIRNAME_CPP).exists())
        errors |= MissingCppDir;
    if (!rootDir.pathAppended(DIRNAME_CPP).pathAppended(FILENAME_MAINCPP).exists())
        errors |= MissingMainCpp;
    if (!rootDir.pathAppended(DIRNAME_CPP).pathAppended(FILENAME_MAINCPP_HEADER).exists())
        errors |= MissingMainCppHeader;
    if (!rootDir.pathAppended(DIRNAME_CPP).pathAppended(FILENAME_ENV_HEADER).exists())
        errors |= MissingEnvHeader;

    if (!rootDir.pathAppended(FILENAME_CMAKELISTS).exists())
        errors |= MissingMainCMake;
    if (!rootDir.pathAppended(FILENAME_MODULES).exists())
        errors |= MissingQmlModules;
    if (!rootDir.pathAppended(FILENAME_MAINQML).exists())
        errors |= MissingMainQml;

    return errors;
}

void removeUnconfirmedQueuedFiles(const Utils::FilePaths confirmedFiles)
{
    QtConcurrent::blockingFilter(queuedFiles, [confirmedFiles](const GeneratableFile &qf) {
        return confirmedFiles.contains(qf.filePath);
    });
}

const QString WARNING_MISSING_STRUCTURE_FATAL = QCoreApplication::translate("QmlDesigner::GenerateCmake",
                                                    "The project is not properly structured for automatically generating CMake files.\n\nAborting process.\n\nThe following files or directories are missing:\n\n%1");
//const QString WARNING_MISSING_STRUCTURE_NONFATAL = QCoreApplication::translate("QmlDesigner::GenerateCmake",
//                                                    "The project is not properly structured for automatically generating CMake files.\n\nThe following files or directories are missing and may be created:\n\n%1");
const QString WARNING_TITLE_FATAL = QCoreApplication::translate("QmlDesigner::GenerateCmake",
                                        "Cannot Generate CMake Files");
//const QString WARNING_TITLE_NONFATAL = QCoreApplication::translate("QmlDesigner::GenerateCmake",
//                                            "Problems with Generating CMake Files");

void showProjectDirErrorDialog(int error)
{
    bool isFatal = isErrorFatal(error);

    if (isFatal) {
        QString fatalList;

        if (error & MissingContentDir)
            fatalList.append(QString(DIRNAME_CONTENT) + "\n");
        if (error & MissingAppMainQml)
            fatalList.append(QString(DIRNAME_CONTENT)
                             + QDir::separator()
                             + QString(FILENAME_APPMAINQML)
                             + "\n");
        if (error & MissingCppDir)
            fatalList.append(QString(DIRNAME_CPP) + "\n");
        if (error & MissingImportDir)
            fatalList.append(QString(DIRNAME_IMPORT) + "\n");

        QMessageBox::critical(nullptr,
                              WARNING_TITLE_FATAL,
                              WARNING_MISSING_STRUCTURE_FATAL.arg(fatalList));
    }
}

bool showConfirmationDialog(const Utils::FilePath &rootDir)
{
    Utils::FilePaths files;
    for (GeneratableFile &file: queuedFiles)
        files.append(file.filePath);

    CmakeGeneratorDialog dialog(rootDir, files);
    dialog.setMinimumWidth(600);
    dialog.setMinimumHeight(640);
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
    file.fileExists = filePath.exists();
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

const char MAIN_CMAKEFILE_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmaincmakelists.tpl";
const char QMLMODULES_FILE_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmodules.tpl";

bool generateCmakes(const FilePath &rootDir)
{
    moduleNames.clear();

    FilePath contentDir = rootDir.pathAppended(DIRNAME_CONTENT);
    FilePath importDir = rootDir.pathAppended(DIRNAME_IMPORT);
    FilePath assetDir = rootDir.pathAppended(DIRNAME_ASSET);

    generateModuleCmake(contentDir);
    generateImportCmake(importDir);
    generateImportCmake(assetDir);
    generateMainCmake(rootDir);

    return true;
}

const char DO_NOT_EDIT_FILE_COMMENT[] = "### This file is automatically generated by Qt Design Studio.\n### Do not change\n\n";
const char ADD_SUBDIR[] = "add_subdirectory(%1)\n";

void generateMainCmake(const FilePath &rootDir)
{
    //TODO startupProject() may be a terrible way to try to get "current project". It's not necessarily the same thing at all.
    QString projectName = ProjectExplorer::SessionManager::startupProject()->displayName();
    QString appName = projectName + "App";

    QString cmakeFileContent = GenerateCmake::readTemplate(MAIN_CMAKEFILE_TEMPLATE_PATH).arg(appName);
    queueCmakeFile(rootDir, cmakeFileContent);

    QString subdirIncludes;
    subdirIncludes.append(QString(ADD_SUBDIR).arg(DIRNAME_CONTENT));
    subdirIncludes.append(QString(ADD_SUBDIR).arg(DIRNAME_IMPORT));
    if (rootDir.pathAppended(DIRNAME_ASSET).exists())
        subdirIncludes.append(QString(ADD_SUBDIR).arg(DIRNAME_ASSET));

    QString modulesAsPlugins;
    for (const QString &moduleName : moduleNames)
        modulesAsPlugins.append("    " + moduleName + "plugin\n");

    QString moduleFileContent = GenerateCmake::readTemplate(QMLMODULES_FILE_TEMPLATE_PATH)
                                                        .arg(appName)
                                                        .arg(subdirIncludes)
                                                        .arg(modulesAsPlugins);

    GenerateCmake::queueFile(rootDir.pathAppended(FILENAME_MODULES), moduleFileContent);
}

void generateImportCmake(const FilePath &dir, const QString &modulePrefix)
{
    if (!dir.exists())
        return;

    QString fileContent;

    fileContent.append(DO_NOT_EDIT_FILE_COMMENT);
    FilePaths subDirs = dir.dirEntries(DIRS_ONLY);
    for (FilePath &subDir : subDirs) {
        if (isDirBlacklisted(subDir))
            continue;
        if (getDirectoryTreeQmls(subDir).isEmpty() && getDirectoryTreeResources(subDir).isEmpty())
            continue;
        fileContent.append(QString(ADD_SUBDIR).arg(subDir.fileName()));
        QString prefix = modulePrefix.isEmpty() ?
                modulePrefix % subDir.fileName() :
                QString(modulePrefix + '.') + subDir.fileName();
        if (getDirectoryQmls(subDir).isEmpty()) {
            generateImportCmake(subDir, prefix);
        } else {
            generateModuleCmake(subDir, prefix);
        }
    }

    queueCmakeFile(dir, fileContent);
}

const char MODULEFILE_PROPERTY_SINGLETON[] = "QT_QML_SINGLETON_TYPE";
const char MODULEFILE_PROPERTY_SET[] = "set_source_files_properties(%1\n    PROPERTIES\n        %2 %3\n)\n\n";
const char MODULEFILE_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmodulecmakelists.tpl";

void generateModuleCmake(const FilePath &dir, const QString &uri)
{
    QString fileTemplate = GenerateCmake::readTemplate(MODULEFILE_TEMPLATE_PATH);

    QString singletonContent;
    FilePaths qmldirFileList = dir.dirEntries(QStringList(FILENAME_QMLDIR), FILES_ONLY);
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

    QString moduleUri = uri.isEmpty() ?
                dir.fileName() :
                uri;
    QString moduleName = QString(moduleUri).remove('.');
    moduleNames.append(moduleName);

    QString fileContent;
    fileContent.append(fileTemplate.arg(singletonContent, moduleName, moduleUri, moduleContent));
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

FilePaths getDirectoryQmls(const FilePath &dir)
{
    const QStringList qmlFilesOnly("*.qml");
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    FilePaths allFiles = dir.dirEntries(qmlFilesOnly, FILES_ONLY);
    FilePaths moduleFiles;
    for (FilePath &file : allFiles) {
        if (!isFileBlacklisted(file.fileName()) &&
            project->isKnownFile(file)) {
            moduleFiles.append(file);
        }
    }

    return moduleFiles;
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
        if (isDirBlacklisted(subDir))
            continue;
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
        if (isDirBlacklisted(subDir))
            continue;
        QStringList subDirResources = getDirectoryTreeResources(subDir);
        for (QString &resource : subDirResources) {
            resourceFileList.append(subDir.fileName().append('/').append(resource));
        }

    }

    return resourceFileList;
}

void queueCmakeFile(const FilePath &dir, const QString &content)
{
    FilePath filePath = dir.pathAppended(FILENAME_CMAKELISTS);
    GenerateCmake::queueFile(filePath, content);
}

bool isFileBlacklisted(const QString &fileName)
{
    return (!fileName.compare(FILENAME_QMLDIR) ||
            !fileName.compare(FILENAME_CMAKELISTS));
}

bool isDirBlacklisted(const FilePath &dir)
{
    return (!dir.fileName().compare(DIRNAME_DESIGNER));
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
const char MAIN_CPPFILE_HEADER_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmaincppheader.tpl";
const char MAIN_CPPFILE_HEADER_PLUGIN_LINE[] = "Q_IMPORT_QML_PLUGIN(%1)\n";
const char ENV_HEADER_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectenvheader.tpl";
const char ENV_HEADER_VARIABLE_LINE[] = "    qputenv(\"%1\", \"%2\");\n";

bool generateMainCpp(const FilePath &dir)
{
    FilePath srcDir = dir.pathAppended(DIRNAME_CPP);

    QString cppContent = GenerateCmake::readTemplate(MAIN_CPPFILE_TEMPLATE_PATH);
    FilePath cppFilePath = srcDir.pathAppended(FILENAME_MAINCPP);
    bool cppOk = GenerateCmake::queueFile(cppFilePath, cppContent);

    QString modulesAsPlugins;
    for (const QString &moduleName : GenerateCmakeLists::moduleNames)
        modulesAsPlugins.append(
                    QString(MAIN_CPPFILE_HEADER_PLUGIN_LINE).arg(moduleName + "Plugin"));

    QString headerContent = GenerateCmake::readTemplate(MAIN_CPPFILE_HEADER_TEMPLATE_PATH)
            .arg(modulesAsPlugins);
    FilePath headerFilePath = srcDir.pathAppended(FILENAME_MAINCPP_HEADER);
    bool pluginHeaderOk = GenerateCmake::queueFile(headerFilePath, headerContent);

    bool envHeaderOk = true;
    QString environment;
    auto *target = ProjectExplorer::SessionManager::startupProject()->activeTarget();
    if (target && target->buildSystem()) {
        auto buildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(target->buildSystem());
        if (buildSystem) {
            for (EnvironmentItem &envItem : buildSystem->environment()) {
                QString key = envItem.name;
                QString value = envItem.value;
                if (isFileResource(value))
                    value.prepend(":/");
                environment.append(QString(ENV_HEADER_VARIABLE_LINE).arg(key).arg(value));
            }
            QString envHeaderContent = GenerateCmake::readTemplate(ENV_HEADER_TEMPLATE_PATH)
                    .arg(environment);
            FilePath envHeaderPath = srcDir.pathAppended(FILENAME_ENV_HEADER);
            envHeaderOk = GenerateCmake::queueFile(envHeaderPath, envHeaderContent);
        }
    }

    return cppOk && pluginHeaderOk && envHeaderOk;
}

const char MAIN_QMLFILE_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmainqml.tpl";

bool generateMainQml(const FilePath &dir)
{
    QString content = GenerateCmake::readTemplate(MAIN_QMLFILE_TEMPLATE_PATH);
    FilePath filePath = dir.pathAppended(FILENAME_MAINQML);
    return GenerateCmake::queueFile(filePath, content);
}

const QStringList resourceFileLocations = {"qtquickcontrols2.conf"};

bool isFileResource(const QString &relativeFilePath)
{
    if (resourceFileLocations.contains(relativeFilePath))
        return true;

    return false;
}

} //GenerateEntryPoints

} //QmlDesigner

