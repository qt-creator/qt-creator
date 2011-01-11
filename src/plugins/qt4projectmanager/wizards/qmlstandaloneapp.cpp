/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlstandaloneapp.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>

#ifndef CREATORLESSTEST
#include <coreplugin/icore.h>
#endif // CREATORLESSTEST

namespace Qt4ProjectManager {
namespace Internal {

const QString qmldir(QLatin1String("qmldir"));
const QString qmldir_plugin(QLatin1String("plugin"));
const QString appViewerBaseName(QLatin1String("qmlapplicationviewer"));
const QString appViewerPriFileName(appViewerBaseName + QLatin1String(".pri"));
const QString appViewerCppFileName(appViewerBaseName + QLatin1String(".cpp"));
const QString appViewerHFileName(appViewerBaseName + QLatin1String(".h"));
const QString appViewerOriginsSubDir(appViewerBaseName + QLatin1Char('/'));

QmlModule::QmlModule(const QString &uri, const QFileInfo &rootDir, const QFileInfo &qmldir,
                     bool isExternal, QmlStandaloneApp *qmlStandaloneApp)
    : uri(uri)
    , rootDir(rootDir)
    , qmldir(qmldir)
    , isExternal(isExternal)
    , qmlStandaloneApp(qmlStandaloneApp)
{}

QString QmlModule::path(Path path) const
{
    switch (path) {
        case Root: {
            return rootDir.canonicalFilePath();
        }
        case ContentDir: {
            const QDir proFile(qmlStandaloneApp->path(QmlStandaloneApp::AppProPath));
            return proFile.relativeFilePath(qmldir.canonicalPath());
        }
        case ContentBase: {
            const QString localRoot = rootDir.canonicalFilePath() + QLatin1Char('/');
            QDir contentDir = qmldir.dir();
            contentDir.cdUp();
            const QString localContentDir = contentDir.canonicalPath();
            return localContentDir.right(localContentDir.length() - localRoot.length());
        }
        case DeployedContentBase: {
            const QString modulesDir = qmlStandaloneApp->path(QmlStandaloneApp::ModulesDir);
            return modulesDir + QLatin1Char('/') + this->path(ContentBase);
        }
        default: qFatal("QmlModule::path() needs more work");
    }
    return QString();
}

QmlCppPlugin::QmlCppPlugin(const QString &name, const QFileInfo &path,
                           const QmlModule *module, const QFileInfo &proFile)
    : name(name)
    , path(path)
    , module(module)
    , proFile(proFile)
{}

bool QmlAppGeneratedFileInfo::isOutdated() const
{
    return version < AbstractMobileApp::makeStubVersion(QmlStandaloneApp::StubVersion);
}

QmlStandaloneApp::QmlStandaloneApp() : AbstractMobileApp()
{
}

QmlStandaloneApp::~QmlStandaloneApp()
{
    clearModulesAndPlugins();
}

void QmlStandaloneApp::setMainQmlFile(const QString &qmlFile)
{
    m_mainQmlFile.setFile(qmlFile);
}

QString QmlStandaloneApp::mainQmlFile() const
{
    return path(MainQml);
}

bool QmlStandaloneApp::setExternalModules(const QStringList &uris,
                                          const QStringList &importPaths)
{
    clearModulesAndPlugins();
    m_importPaths.clear();
    foreach (const QFileInfo &importPath, importPaths) {
        if (!importPath.exists()) {
            m_error = QCoreApplication::translate(
                        "Qt4ProjectManager::Internal::QmlStandaloneApp",
                        "The QML import path '%1' cannot be found.")
                      .arg(QDir::toNativeSeparators(importPath.filePath()));
            return false;
        } else {
            m_importPaths.append(importPath.canonicalFilePath());
        }
    }
    foreach (const QString &uri, uris) {
        QString uriPath = uri;
        uriPath.replace(QLatin1Char('.'), QLatin1Char('/'));
        const int modulesCount = m_modules.count();
        foreach (const QFileInfo &importPath, m_importPaths) {
            const QFileInfo qmlDirFile(
                    importPath.absoluteFilePath() + QLatin1Char('/')
                    + uriPath + QLatin1Char('/') + qmldir);
            if (qmlDirFile.exists()) {
                if (!addExternalModule(uri, importPath, qmlDirFile))
                    return false;
                break;
            }
        }
        if (modulesCount == m_modules.count()) { // no module was added
            m_error = QCoreApplication::translate(
                      "Qt4ProjectManager::Internal::QmlStandaloneApp",
                      "The QML module '%1' cannot be found.").arg(uri);
            return false;
        }
    }
    m_error.clear();
    return true;
}

QString QmlStandaloneApp::pathExtended(int fileType) const
{
    QString cleanProjectName = projectName().replace(QLatin1Char('-'), QString());
    const QString qmlSubDir = QLatin1String("qml/")
                              + (useExistingMainQml() ? m_mainQmlFile.dir().dirName() : cleanProjectName)
                              + QLatin1Char('/');   
    const QString appViewerTargetSubDir = appViewerOriginsSubDir;
    const QString mainQml = QLatin1String("main.qml");
    const QString pathBase = outputPathBase();
    const QDir appProFilePath(pathBase);

    switch (fileType) {
        case MainQml:                       return useExistingMainQml() ? m_mainQmlFile.canonicalFilePath()
                                                : pathBase + qmlSubDir + mainQml;
        case MainQmlDeployed:               return useExistingMainQml() ? qmlSubDir + m_mainQmlFile.fileName()
                                                : QString(qmlSubDir + mainQml);
        case MainQmlOrigin:                 return originsRoot() + QLatin1String("qml/app/") + mainQml;
        case AppViewerPri:                  return pathBase + appViewerTargetSubDir + appViewerPriFileName;
        case AppViewerPriOrigin:            return originsRoot() + appViewerOriginsSubDir + appViewerPriFileName;
        case AppViewerCpp:                  return pathBase + appViewerTargetSubDir + appViewerCppFileName;
        case AppViewerCppOrigin:            return originsRoot() + appViewerOriginsSubDir + appViewerCppFileName;
        case AppViewerH:                    return pathBase + appViewerTargetSubDir + appViewerHFileName;
        case AppViewerHOrigin:              return originsRoot() + appViewerOriginsSubDir + appViewerHFileName;
        case QmlDir:                        return pathBase + qmlSubDir;
        case QmlDirProFileRelative:         return useExistingMainQml() ? appProFilePath.relativeFilePath(m_mainQmlFile.canonicalPath())
                                                : QString(qmlSubDir).remove(qmlSubDir.length() - 1, 1);
        case ModulesDir:                    return QLatin1String("modules");
        default:                            qFatal("QmlStandaloneApp::pathExtended() needs more work");
    }
    return QString();
}

QString QmlStandaloneApp::originsRoot() const
{
    return templatesRoot() + QLatin1String("qmlapp/");
}

QString QmlStandaloneApp::mainWindowClassName() const
{
    return QLatin1String("QmlApplicationViewer");
}

bool QmlStandaloneApp::adaptCurrentMainCppTemplateLine(QString &line) const
{
    const QLatin1Char quote('"');
    bool adaptLine = true;
    if (line.contains(QLatin1String("// MAINQML"))) {
        insertParameter(line, quote + path(MainQmlDeployed) + quote);
    } else if (line.contains(QLatin1String("// ADDIMPORTPATH"))) {
        if (m_modules.isEmpty())
            adaptLine = false;
        else
            insertParameter(line, quote + path(ModulesDir) + quote);
    }
    return adaptLine;
}

void QmlStandaloneApp::handleCurrentProFileTemplateLine(const QString &line,
    QTextStream &proFileTemplate, QTextStream &proFile,
    bool &uncommentNextLine) const
{
    if (line.contains(QLatin1String("# DEPLOYMENTFOLDERS"))) {
        // Eat lines
        QString nextLine;
        while (!(nextLine = proFileTemplate.readLine()).isNull()
            && !nextLine.contains(QLatin1String("# DEPLOYMENTFOLDERS_END")))
        { }
        if (nextLine.isNull())
            return;
        QStringList folders;
        proFile << "folder_01.source = " << path(QmlDirProFileRelative) << endl;
        proFile << "folder_01.target = qml" << endl;
        folders.append(QLatin1String("folder_01"));
        int foldersCount = 1;
        foreach (const QmlModule *module, m_modules) {
            if (module->isExternal) {
                foldersCount ++;
                const QString folder =
                    QString::fromLatin1("folder_%1").arg(foldersCount, 2, 10, QLatin1Char('0'));
                folders.append(folder);
                proFile << folder << ".source = " << module->path(QmlModule::ContentDir) << endl;
                proFile << folder << ".target = " << module->path(QmlModule::DeployedContentBase) << endl;
            }
        }
        proFile << "DEPLOYMENTFOLDERS = " << folders.join(QLatin1String(" ")) << endl;
    } else if (line.contains(QLatin1String("# QMLJSDEBUGGER"))) {
        // ### disabled for now; figure out the private headers problem first.
        //uncommentNextLine = true;
        Q_UNUSED(uncommentNextLine);
    } else if (line.contains(QLatin1String("# QML_IMPORT_PATH"))) {
        QString nextLine = proFileTemplate.readLine(); // eats 'QML_IMPORT_PATH ='
        if (!nextLine.startsWith(QLatin1String("QML_IMPORT_PATH =")))
            return;

        proFile << nextLine;

        const QLatin1String separator(" \\\n    ");
        const QDir proPath(path(AppProPath));
        foreach (const QString &importPath, m_importPaths) {
            const QString relativePath = proPath.relativeFilePath(importPath);
            proFile << separator << relativePath;
        }

        proFile << endl;
    } else if (line.contains(QLatin1String("# INCLUDE_DEPLOYMENT_PRI"))) {
        proFileTemplate.readLine(); // eats 'include(deployment.pri)'
    }
}

void QmlStandaloneApp::clearModulesAndPlugins()
{
    qDeleteAll(m_modules);
    m_modules.clear();
    qDeleteAll(m_cppPlugins);
    m_cppPlugins.clear();
}

bool QmlStandaloneApp::addCppPlugin(const QString &qmldirLine, QmlModule *module)
{
    const QStringList qmldirLineElements =
            qmldirLine.split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (qmldirLineElements.count() < 2) {
        m_error = QCoreApplication::translate(
                      "Qt4ProjectManager::Internal::QmlStandaloneApp",
                      "Invalid '%1' entry in '%2' of module '%3'.")
                  .arg(qmldir_plugin).arg(qmldir).arg(module->uri);
        return false;
    }
    const QString name = qmldirLineElements.at(1);
    const QFileInfo path(module->qmldir.dir(), qmldirLineElements.value(2, QString()));

    // TODO: Add more magic to find a good .pro file..
    const QString proFileName = name + QLatin1String(".pro");
    const QFileInfo proFile_guess1(module->qmldir.dir(), proFileName);
    const QFileInfo proFile_guess2(QString(module->qmldir.dir().absolutePath() + QLatin1String("/../")),
                                   proFileName);
    const QFileInfo proFile_guess3(module->qmldir.dir(),
                                   QFileInfo(module->qmldir.path()).fileName() + QLatin1String(".pro"));
    const QFileInfo proFile_guess4(proFile_guess3.absolutePath() + QLatin1String("/../")
                                   + proFile_guess3.fileName());

    QFileInfo foundProFile;
    if (proFile_guess1.exists()) {
        foundProFile = proFile_guess1.canonicalFilePath();
    } else if (proFile_guess2.exists()) {
        foundProFile = proFile_guess2.canonicalFilePath();
    } else if (proFile_guess3.exists()) {
        foundProFile = proFile_guess3.canonicalFilePath();
    } else if (proFile_guess4.exists()) {
        foundProFile = proFile_guess4.canonicalFilePath();
    } else {
        m_error = QCoreApplication::translate(
                    "Qt4ProjectManager::Internal::QmlStandaloneApp",
                    "No .pro file for plugin '%1' cannot be found.").arg(name);
        return false;
    }
    QmlCppPlugin *plugin =
            new QmlCppPlugin(name, path, module, foundProFile);
    m_cppPlugins.append(plugin);
    module->cppPlugins.insert(name, plugin);
    return true;
}

bool QmlStandaloneApp::addCppPlugins(QmlModule *module)
{
    QFile qmlDirFile(module->qmldir.absoluteFilePath());
    if (qmlDirFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&qmlDirFile);
        QString line;
        while (!(line = in.readLine()).isNull()) {
            line = line.trimmed();
            if (line.startsWith(qmldir_plugin) && !addCppPlugin(line, module))
                return false;
        };
    }
    return true;
}

bool QmlStandaloneApp::addExternalModule(const QString &name, const QFileInfo &dir,
                                         const QFileInfo &contentDir)
{
    QmlModule *module = new QmlModule(name, dir, contentDir, true, this);
    m_modules.append(module);
    return addCppPlugins(module);
}

#ifndef CREATORLESSTEST
Core::GeneratedFiles QmlStandaloneApp::generateFiles(QString *errorMessage) const
{
    Core::GeneratedFiles files = AbstractMobileApp::generateFiles(errorMessage);
    if (!useExistingMainQml()) {
        files.append(file(generateFile(QmlAppGeneratedFileInfo::MainQmlFile, errorMessage), path(MainQml)));
        files.last().setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    }

    files.append(file(generateFile(QmlAppGeneratedFileInfo::AppViewerPriFile, errorMessage), path(AppViewerPri)));
    files.append(file(generateFile(QmlAppGeneratedFileInfo::AppViewerCppFile, errorMessage), path(AppViewerCpp)));
    files.append(file(generateFile(QmlAppGeneratedFileInfo::AppViewerHFile, errorMessage), path(AppViewerH)));

    return files;
}
#endif // CREATORLESSTEST

bool QmlStandaloneApp::useExistingMainQml() const
{
    return !m_mainQmlFile.filePath().isEmpty();
}

const QList<QmlModule*> QmlStandaloneApp::modules() const
{
    return m_modules;
}

QByteArray QmlStandaloneApp::generateFileExtended(int fileType,
    bool *versionAndCheckSum, QString *comment, QString *errorMessage) const
{
    QByteArray data;
    switch (fileType) {
        case QmlAppGeneratedFileInfo::MainQmlFile:
            data = readBlob(path(MainQmlOrigin), errorMessage);
            break;
        case QmlAppGeneratedFileInfo::AppViewerPriFile:
            data = readBlob(path(AppViewerPriOrigin), errorMessage);
            data.append(readBlob(path(DeploymentPriOrigin), errorMessage));
            *comment = ProFileComment;
            *versionAndCheckSum = true;
            break;
        case QmlAppGeneratedFileInfo::AppViewerCppFile:
            data = readBlob(path(AppViewerCppOrigin), errorMessage);
            *versionAndCheckSum = true;
            break;
        case QmlAppGeneratedFileInfo::AppViewerHFile:
        default:
            data = readBlob(path(AppViewerHOrigin), errorMessage);
            *versionAndCheckSum = true;
            break;
    }
    return data;
}

int QmlStandaloneApp::stubVersionMinor() const
{
    return StubVersion;
}

static QList<QmlAppGeneratedFileInfo> updateableFiles(const QString &mainProFile)
{
    QList<QmlAppGeneratedFileInfo> result;
    static const struct {
        int fileType;
        QString fileName;
    } files[] = {
        {QmlAppGeneratedFileInfo::AppViewerPriFile, appViewerPriFileName},
        {QmlAppGeneratedFileInfo::AppViewerHFile, appViewerHFileName},
        {QmlAppGeneratedFileInfo::AppViewerCppFile, appViewerCppFileName}
    };
    const QFileInfo mainProFileInfo(mainProFile);
    const int size = sizeof(files) / sizeof(files[0]);
    for (int i = 0; i < size; ++i) {
        const QString fileName = mainProFileInfo.dir().absolutePath()
                + QLatin1Char('/') + appViewerOriginsSubDir + files[i].fileName;
        if (!QFile::exists(fileName))
            continue;
        QmlAppGeneratedFileInfo file;
        file.fileType = files[i].fileType;
        file.fileInfo = QFileInfo(fileName);
        result.append(file);
    }
    if (result.count() != size)
        result.clear(); // All files must be found. No wrong/partial updates, please.
    return result;
}

QList<QmlAppGeneratedFileInfo> QmlStandaloneApp::fileUpdates(const QString &mainProFile)
{
    QList<QmlAppGeneratedFileInfo> result;
    foreach (const QmlAppGeneratedFileInfo &file, updateableFiles(mainProFile)) {
        QmlAppGeneratedFileInfo newFile = file;
        QFile readFile(newFile.fileInfo.absoluteFilePath());
        if (!readFile.open(QIODevice::ReadOnly))
           continue;
        const QString firstLine = readFile.readLine();
        const QStringList elements = firstLine.split(QLatin1Char(' '));
        if (elements.count() != 5 || elements.at(1) != FileChecksum
                || elements.at(3) != FileStubVersion)
            continue;
        const QString versionString = elements.at(4);
        newFile.version = versionString.startsWith(QLatin1String("0x"))
            ? versionString.toInt(0, 16) : 0;
        newFile.statedChecksum = elements.at(2).toUShort(0, 16);
        QByteArray data = readFile.readAll();
        data.replace('\x0D', "");
        data.replace('\x0A', "");
        newFile.dataChecksum = qChecksum(data.constData(), data.length());
        if (!newFile.isUpToDate())
            result.append(newFile);
    }
    return result;
}

bool QmlStandaloneApp::updateFiles(const QList<QmlAppGeneratedFileInfo> &list, QString &error)
{
    error.clear();
    const QmlStandaloneApp app;
    foreach (const QmlAppGeneratedFileInfo &info, list) {
        const QByteArray data = app.generateFile(info.fileType, &error);
        if (!error.isEmpty())
            return false;
        QFile file(info.fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::WriteOnly) || file.write(data) == -1) {
            error = QCoreApplication::translate(
                        "Qt4ProjectManager::Internal::QmlStandaloneApp",
                        "Could not write file '%1'.").
                    arg(QDir::toNativeSeparators(info.fileInfo.canonicalFilePath()));
            return false;
        }
    }
    return true;
}

const int QmlStandaloneApp::StubVersion = 10;

} // namespace Internal
} // namespace Qt4ProjectManager
