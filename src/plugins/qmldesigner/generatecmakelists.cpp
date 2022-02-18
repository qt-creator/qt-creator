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
    MissingAssetDir = 1<<3,
    MissingAssetImportDir = 1<<4,
    MissingCppDir = 1<<5,
    MissingMainCMake = 1<<6,
    MissingMainQml = 1<<7,
    MissingAppMainQml = 1<<8,
    MissingQmlModules = 1<<9,
    MissingMainCpp = 1<<10,
    MissingMainCppHeader = 1<<11,
    MissingEnvHeader = 1<<12
};

const QString MENU_ITEM_GENERATE = QCoreApplication::translate("QmlDesigner::GenerateCmake",
                                                               "Generate CMake Build Files");

void generateMenuEntry()
{
    Core::ActionContainer *menu =
            Core::ActionManager::actionContainer(Core::Constants::M_FILE);
    auto action = new QAction(MENU_ITEM_GENERATE);
    QObject::connect(action, &QAction::triggered, GenerateCmake::onGenerateCmakeLists);
    Core::Command *cmd = Core::ActionManager::registerAction(action, "QmlProject.CreateCMakeLists");
    menu->addAction(cmd, Core::Constants::G_FILE_EXPORT);

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

    CmakeFileGenerator cmakeGen;
    cmakeGen.prepare(rootDir);

    FilePaths allFiles;
    for (const GeneratableFile &file: cmakeGen.fileQueue().queuedFiles())
        allFiles.append(file.filePath);

    CmakeGeneratorDialog dialog(rootDir, allFiles);
    if (dialog.exec()) {
        FilePaths confirmedFiles = dialog.getFilePaths();
        cmakeGen.filterFileQueue(confirmedFiles);
        cmakeGen.execute();
    }
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
    if (!rootDir.pathAppended(DIRNAME_ASSETIMPORT).exists())
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

bool FileQueue::queueFile(const FilePath &filePath, const QString &fileContent)
{
    GeneratableFile file;
    file.filePath = filePath;
    file.content = fileContent;
    file.fileExists = filePath.exists();
    m_queuedFiles.append(file);

    return true;
}

const QVector<GeneratableFile> FileQueue::queuedFiles() const
{
    return m_queuedFiles;
}

bool FileQueue::writeQueuedFiles()
{
    for (GeneratableFile &file: m_queuedFiles)
        if (!writeFile(file))
            return false;

    return true;
}

bool FileQueue::writeFile(const GeneratableFile &file)
{
    QFile fileHandle(file.filePath.toString());
    fileHandle.open(QIODevice::WriteOnly);
    QTextStream stream(&fileHandle);
    stream << file.content;
    fileHandle.close();

    return true;
}

void FileQueue::filterFiles(const Utils::FilePaths keepFiles)
{
    QtConcurrent::blockingFilter(m_queuedFiles, [keepFiles](const GeneratableFile &qf) {
        return keepFiles.contains(qf.filePath);
    });
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

const QString projectEnvironmentVariable(const QString &key)
{
    QString value = {};

    auto *target = ProjectExplorer::SessionManager::startupProject()->activeTarget();
    if (target && target->buildSystem()) {
        auto buildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(target->buildSystem());
        if (buildSystem) {
            auto envItems = buildSystem->environment();
            auto confEnv = std::find_if(envItems.begin(), envItems.end(),
                                     [key](NameValueItem &item){return item.name == key;});
            if (confEnv != envItems.end())
                value = confEnv->value;
        }
    }
    return value;
}

const QDir::Filters FILES_ONLY = QDir::Files;
const QDir::Filters DIRS_ONLY = QDir::Dirs|QDir::Readable|QDir::NoDotAndDotDot;

const char MAIN_CMAKEFILE_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmaincmakelists.tpl";
const char QMLMODULES_FILE_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmodules.tpl";

bool CmakeFileGenerator::prepare(const FilePath &rootDir, bool checkFileBelongs)
{
    m_checkFileIsInProject = checkFileBelongs;

    FilePath contentDir = rootDir.pathAppended(DIRNAME_CONTENT);
    FilePath importDir = rootDir.pathAppended(DIRNAME_IMPORT);
    FilePath assetImportDir = rootDir.pathAppended(DIRNAME_ASSETIMPORT);

    generateModuleCmake(contentDir);
    generateImportCmake(importDir);
    generateImportCmake(assetImportDir);
    generateMainCmake(rootDir);
    generateEntryPointFiles(rootDir);

    return true;
}

const FileQueue CmakeFileGenerator::fileQueue() const
{
    return m_fileQueue;
}

void CmakeFileGenerator::filterFileQueue(const Utils::FilePaths &keepFiles)
{
    m_fileQueue.filterFiles(keepFiles);
}

bool CmakeFileGenerator::execute()
{
    return m_fileQueue.writeQueuedFiles();
}

const char DO_NOT_EDIT_FILE_COMMENT[] = "### This file is automatically generated by Qt Design Studio.\n### Do not change\n\n";
const char ADD_SUBDIR[] = "add_subdirectory(%1)\n";

void CmakeFileGenerator::generateMainCmake(const FilePath &rootDir)
{
    //TODO startupProject() may be a terrible way to try to get "current project". It's not necessarily the same thing at all.
    QString projectName = ProjectExplorer::SessionManager::startupProject()->displayName();
    QString appName = projectName + "App";

    QString fileSection = "";
    const QString qtcontrolsConfFile = GenerateCmake::projectEnvironmentVariable(ENV_VARIABLE_CONTROLCONF);
    if (!qtcontrolsConfFile.isEmpty())
        fileSection = QString("    FILES\n        %1").arg(qtcontrolsConfFile);

    QString cmakeFileContent = GenerateCmake::readTemplate(MAIN_CMAKEFILE_TEMPLATE_PATH)
            .arg(appName)
            .arg(fileSection);

    queueCmakeFile(rootDir, cmakeFileContent);

    QString subdirIncludes;
    subdirIncludes.append(QString(ADD_SUBDIR).arg(DIRNAME_CONTENT));
    subdirIncludes.append(QString(ADD_SUBDIR).arg(DIRNAME_IMPORT));
    if (rootDir.pathAppended(DIRNAME_ASSETIMPORT).exists())
        subdirIncludes.append(QString(ADD_SUBDIR).arg(DIRNAME_ASSETIMPORT));

    QString modulesAsPlugins;
    for (const QString &moduleName : m_moduleNames)
        modulesAsPlugins.append("    " + moduleName + "plugin\n");

    QString moduleFileContent = GenerateCmake::readTemplate(QMLMODULES_FILE_TEMPLATE_PATH)
                                                        .arg(appName)
                                                        .arg(subdirIncludes)
                                                        .arg(modulesAsPlugins);

    m_fileQueue.queueFile(rootDir.pathAppended(FILENAME_MODULES), moduleFileContent);
}

void CmakeFileGenerator::generateImportCmake(const FilePath &dir, const QString &modulePrefix)
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

void CmakeFileGenerator::generateModuleCmake(const FilePath &dir, const QString &uri)
{
    QString fileTemplate = GenerateCmake::readTemplate(MODULEFILE_TEMPLATE_PATH);

    QString singletonContent;
    FilePaths qmldirFileList = dir.dirEntries({QStringList(FILENAME_QMLDIR), FILES_ONLY});
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

    QString moduleName = QString(moduleUri).replace('.', '_');
    m_moduleNames.append(moduleName);

    QString fileContent;
    fileContent.append(fileTemplate.arg(singletonContent, moduleName, moduleUri, moduleContent));
    queueCmakeFile(dir, fileContent);
}

QStringList CmakeFileGenerator::getSingletonsFromQmldirFile(const FilePath &filePath)
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

QStringList CmakeFileGenerator::getDirectoryQmls(const FilePath &dir)
{
    QStringList moduleFiles;

    const QStringList qmlFilesOnly(FILENAME_FILTER_QML);
    FilePaths allFiles = dir.dirEntries({qmlFilesOnly, FILES_ONLY});
    for (FilePath &file : allFiles) {
        if (includeFile(file)) {
            moduleFiles.append(file.fileName());
        }
    }

    return moduleFiles;
}

QStringList CmakeFileGenerator::getDirectoryResources(const FilePath &dir)
{
    QStringList moduleFiles;

    FilePaths allFiles = dir.dirEntries(FILES_ONLY);
    for (FilePath &file : allFiles) {
        if (!file.fileName().endsWith(".qml", Qt::CaseInsensitive) &&
            includeFile(file)) {
            moduleFiles.append(file.fileName());
        }
    }

    return moduleFiles;
}

QStringList CmakeFileGenerator::getDirectoryTreeQmls(const FilePath &dir)
{
    QStringList qmlFileList;

    qmlFileList.append(getDirectoryQmls(dir));

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

QStringList CmakeFileGenerator::getDirectoryTreeResources(const FilePath &dir)
{
    QStringList resourceFileList;

    resourceFileList.append(getDirectoryResources(dir));

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

void CmakeFileGenerator::queueCmakeFile(const FilePath &dir, const QString &content)
{
    FilePath filePath = dir.pathAppended(FILENAME_CMAKELISTS);
    m_fileQueue.queueFile(filePath, content);
}

bool CmakeFileGenerator::isFileBlacklisted(const QString &fileName)
{
    return (!fileName.compare(FILENAME_QMLDIR) ||
            !fileName.compare(FILENAME_CMAKELISTS));
}

bool CmakeFileGenerator::isDirBlacklisted(const FilePath &dir)
{
    return (!dir.fileName().compare(DIRNAME_DESIGNER));
}

bool CmakeFileGenerator::includeFile(const FilePath &filePath)
{
    if (m_checkFileIsInProject) {
        ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
        if (!project->isKnownFile(filePath))
            return false;
    }

    return !isFileBlacklisted(filePath.fileName());
}


bool CmakeFileGenerator::generateEntryPointFiles(const FilePath &dir)
{
    const QString qtcontrolsConf = GenerateCmake::projectEnvironmentVariable(ENV_VARIABLE_CONTROLCONF);
    if (!qtcontrolsConf.isEmpty())
        m_resourceFileLocations.append(qtcontrolsConf);

    bool cppOk = generateMainCpp(dir);
    bool qmlOk = generateMainQml(dir);

    return cppOk && qmlOk;
}

const char MAIN_CPPFILE_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmaincpp.tpl";
const char MAIN_CPPFILE_HEADER_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmaincppheader.tpl";
const char MAIN_CPPFILE_HEADER_PLUGIN_LINE[] = "Q_IMPORT_QML_PLUGIN(%1)\n";
const char ENV_HEADER_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectenvheader.tpl";
const char ENV_HEADER_VARIABLE_LINE[] = "    qputenv(\"%1\", \"%2\");\n";

bool CmakeFileGenerator::generateMainCpp(const FilePath &dir)
{
    FilePath srcDir = dir.pathAppended(DIRNAME_CPP);

    QString cppContent = GenerateCmake::readTemplate(MAIN_CPPFILE_TEMPLATE_PATH);
    FilePath cppFilePath = srcDir.pathAppended(FILENAME_MAINCPP);
    bool cppOk = m_fileQueue.queueFile(cppFilePath, cppContent);

    QString modulesAsPlugins;
    for (const QString &moduleName : m_moduleNames)
        modulesAsPlugins.append(
                    QString(MAIN_CPPFILE_HEADER_PLUGIN_LINE).arg(moduleName + "Plugin"));

    QString headerContent = GenerateCmake::readTemplate(MAIN_CPPFILE_HEADER_TEMPLATE_PATH)
            .arg(modulesAsPlugins);
    FilePath headerFilePath = srcDir.pathAppended(FILENAME_MAINCPP_HEADER);
    bool pluginHeaderOk = m_fileQueue.queueFile(headerFilePath, headerContent);

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
            envHeaderOk = m_fileQueue.queueFile(envHeaderPath, envHeaderContent);
        }
    }

    return cppOk && pluginHeaderOk && envHeaderOk;
}

const char MAIN_QMLFILE_TEMPLATE_PATH[] = ":/boilerplatetemplates/qmlprojectmainqml.tpl";

bool CmakeFileGenerator::generateMainQml(const FilePath &dir)
{
    QString content = GenerateCmake::readTemplate(MAIN_QMLFILE_TEMPLATE_PATH);
    FilePath filePath = dir.pathAppended(FILENAME_MAINQML);
    return m_fileQueue.queueFile(filePath, content);
}

bool CmakeFileGenerator::isFileResource(const QString &relativeFilePath)
{
    if (m_resourceFileLocations.contains(relativeFilePath))
        return true;

    return false;
}

} //GenerateCmake
} //QmlDesigner

