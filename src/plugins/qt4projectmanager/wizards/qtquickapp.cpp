/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qtquickapp.h"

#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QTextStream>
#include <QCoreApplication>

#ifndef CREATORLESSTEST
#include <coreplugin/icore.h>
#endif // CREATORLESSTEST

namespace Qt4ProjectManager {
namespace Internal {

const QString qmldir(QLatin1String("qmldir"));
const QString qmldir_plugin(QLatin1String("plugin"));

QmlModule::QmlModule(const QString &uri, const QFileInfo &rootDir, const QFileInfo &qmldir,
                     bool isExternal, QtQuickApp *qtQuickApp)
    : uri(uri)
    , rootDir(rootDir)
    , qmldir(qmldir)
    , isExternal(isExternal)
    , qtQuickApp(qtQuickApp)
{}

QString QmlModule::path(Path path) const
{
    switch (path) {
        case Root: {
            return rootDir.canonicalFilePath();
        }
        case ContentDir: {
            const QDir proFile(qtQuickApp->path(QtQuickApp::AppProPath));
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
            const QString modulesDir = qtQuickApp->path(QtQuickApp::ModulesDir);
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
{
}

QtQuickApp::QtQuickApp()
    : AbstractMobileApp()
    , m_mainQmlMode(ModeGenerate)
    , m_componentSet(QtQuick10Components)
{
    m_canSupportMeegoBooster = true;
}

QtQuickApp::~QtQuickApp()
{
    clearModulesAndPlugins();
}

void QtQuickApp::setComponentSet(ComponentSet componentSet)
{
    m_componentSet = componentSet;
}

QtQuickApp::ComponentSet QtQuickApp::componentSet() const
{
    return m_componentSet;
}

void QtQuickApp::setMainQml(Mode mode, const QString &file)
{
    Q_ASSERT(mode != ModeGenerate || file.isEmpty());
    m_mainQmlMode = mode;
    m_mainQmlFile.setFile(file);
}

QtQuickApp::Mode QtQuickApp::mainQmlMode() const
{
    return m_mainQmlMode;
}

bool QtQuickApp::setExternalModules(const QStringList &uris,
                                    const QStringList &importPaths)
{
    clearModulesAndPlugins();
    m_importPaths.clear();
    foreach (const QFileInfo &importPath, importPaths) {
        if (!importPath.exists()) {
            m_error = QCoreApplication::translate(
                        "Qt4ProjectManager::Internal::QtQuickApp",
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
                      "Qt4ProjectManager::Internal::QtQuickApp",
                      "The QML module '%1' cannot be found.").arg(uri);
            return false;
        }
    }
    m_error.clear();
    return true;
}

QString QtQuickApp::pathExtended(int fileType) const
{
    const bool importQmlFile = m_mainQmlMode == ModeImport;
    const QString qmlSubDir = QLatin1String("qml/")
                              + (importQmlFile ? m_mainQmlFile.dir().dirName() : projectName())
                              + QLatin1Char('/');
    const QString appViewerTargetSubDir = appViewerOriginSubDir();

    const QString mainQmlFile = QLatin1String("main.qml");
    const QString mainPageQmlFile = QLatin1String("MainPage.qml");

    const QString qmlOriginDir = originsRoot() + QLatin1String("qml/app/")
                        + componentSetDir(componentSet()) + QLatin1Char('/');

    const QString pathBase = outputPathBase();
    const QDir appProFilePath(pathBase);

    switch (fileType) {
        case MainQml:
            return importQmlFile ? m_mainQmlFile.canonicalFilePath() : pathBase + qmlSubDir + mainQmlFile;
        case MainQmlDeployed:               return importQmlFile ? qmlSubDir + m_mainQmlFile.fileName()
                                                                 : QString(qmlSubDir + mainQmlFile);
        case MainQmlOrigin:                 return qmlOriginDir + mainQmlFile;
        case MainPageQml:                   return pathBase + qmlSubDir + mainPageQmlFile;
        case MainPageQmlOrigin:             return qmlOriginDir + mainPageQmlFile;
        case AppViewerPri:                  return pathBase + appViewerTargetSubDir + fileName(AppViewerPri);
        case AppViewerPriOrigin:            return originsRoot() + appViewerOriginSubDir() + fileName(AppViewerPri);
        case AppViewerCpp:                  return pathBase + appViewerTargetSubDir + fileName(AppViewerCpp);
        case AppViewerCppOrigin:            return originsRoot() + appViewerOriginSubDir() + fileName(AppViewerCpp);
        case AppViewerH:                    return pathBase + appViewerTargetSubDir + fileName(AppViewerH);
        case AppViewerHOrigin:              return originsRoot() + appViewerOriginSubDir() + fileName(AppViewerH);
        case QmlDir:                        return pathBase + qmlSubDir;
        case QmlDirProFileRelative:         return importQmlFile ? appProFilePath.relativeFilePath(m_mainQmlFile.canonicalPath())
                                                                 : QString(qmlSubDir).remove(qmlSubDir.length() - 1, 1);
        case ModulesDir:                    return QLatin1String("modules");
        default:                            qFatal("QtQuickApp::pathExtended() needs more work");
    }
    return QString();
}

QString QtQuickApp::originsRoot() const
{
    const bool isQtQuick2 = m_componentSet == QtQuick20Components;
    return templatesRoot() + QLatin1String(isQtQuick2 ? "qtquick2app/" : "qtquickapp/");
}

QString QtQuickApp::mainWindowClassName() const
{
    return QLatin1String("QmlApplicationViewer");
}

bool QtQuickApp::adaptCurrentMainCppTemplateLine(QString &line) const
{
    const QLatin1Char quote('"');

    if (line.contains(QLatin1String("// MAINQML"))) {
        insertParameter(line, quote + path(MainQmlDeployed) + quote);
    } else if (line.contains(QLatin1String("// ADDIMPORTPATH"))) {
        if (m_modules.isEmpty())
            return false;
        else
            insertParameter(line, quote + path(ModulesDir) + quote);
    }
    return true;
}

void QtQuickApp::handleCurrentProFileTemplateLine(const QString &line,
    QTextStream &proFileTemplate, QTextStream &proFile,
    bool &commentOutNextLine) const
{
    Q_UNUSED(commentOutNextLine)
    if (line.contains(QLatin1String("# QML_IMPORT_PATH"))) {
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
    } else if (line.contains(QLatin1String("# HARMATTAN_BOOSTABLE"))) {
        QString nextLine = proFileTemplate.readLine(); // eats '# CONFIG += qdeclarative-boostable'
        if (supportsMeegoBooster())
            nextLine.remove(0, 2); // remove comment
        proFile << nextLine << endl;
    }
}

void QtQuickApp::clearModulesAndPlugins()
{
    qDeleteAll(m_modules);
    m_modules.clear();
    qDeleteAll(m_cppPlugins);
    m_cppPlugins.clear();
}

bool QtQuickApp::addCppPlugin(const QString &qmldirLine, QmlModule *module)
{
    const QStringList qmldirLineElements =
            qmldirLine.split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (qmldirLineElements.count() < 2) {
        m_error = QCoreApplication::translate(
                      "Qt4ProjectManager::Internal::QtQuickApp",
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
                    "Qt4ProjectManager::Internal::QtQuickApp",
                    "No .pro file for plugin '%1' can be found.").arg(name);
        return false;
    }
    QmlCppPlugin *plugin =
            new QmlCppPlugin(name, path, module, foundProFile);
    m_cppPlugins.append(plugin);
    module->cppPlugins.insert(name, plugin);
    return true;
}

bool QtQuickApp::addCppPlugins(QmlModule *module)
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

bool QtQuickApp::addExternalModule(const QString &name, const QFileInfo &dir,
                                         const QFileInfo &contentDir)
{
    QmlModule *module = new QmlModule(name, dir, contentDir, true, this);
    m_modules.append(module);
    return addCppPlugins(module);
}

#ifndef CREATORLESSTEST
Core::GeneratedFiles QtQuickApp::generateFiles(QString *errorMessage) const
{
    Core::GeneratedFiles files = AbstractMobileApp::generateFiles(errorMessage);
    if (!useExistingMainQml()) {
        files.append(file(generateFile(QtQuickAppGeneratedFileInfo::MainQmlFile, errorMessage), path(MainQml)));
        if ((componentSet() == QtQuickApp::Meego10Components))
            files.append(file(generateFile(QtQuickAppGeneratedFileInfo::MainPageQmlFile, errorMessage), path(MainPageQml)));
        files.last().setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    }

    files.append(file(generateFile(QtQuickAppGeneratedFileInfo::AppViewerPriFile, errorMessage), path(AppViewerPri)));
    files.append(file(generateFile(QtQuickAppGeneratedFileInfo::AppViewerCppFile, errorMessage), path(AppViewerCpp)));
    files.append(file(generateFile(QtQuickAppGeneratedFileInfo::AppViewerHFile, errorMessage), path(AppViewerH)));

    return files;
}
#endif // CREATORLESSTEST

bool QtQuickApp::useExistingMainQml() const
{
    return !m_mainQmlFile.filePath().isEmpty();
}

const QList<QmlModule*> QtQuickApp::modules() const
{
    return m_modules;
}

QString QtQuickApp::appViewerBaseName() const
{
    return QLatin1String(m_componentSet == QtQuick20Components ?
                             "qtquick2applicationviewer" : "qmlapplicationviewer");
}

QString QtQuickApp::fileName(QtQuickApp::ExtendedFileType type) const
{
    switch (type) {
        case AppViewerPri:      return appViewerBaseName() + QLatin1String(".pri");
        case AppViewerH:        return appViewerBaseName() + QLatin1String(".h");
        case AppViewerCpp:      return appViewerBaseName() + QLatin1String(".cpp");
        default:                return QString();
    }
}

QString QtQuickApp::appViewerOriginSubDir() const
{
    return appViewerBaseName() + QLatin1Char('/');
}

QByteArray QtQuickApp::generateFileExtended(int fileType,
    bool *versionAndCheckSum, QString *comment, QString *errorMessage) const
{
    QByteArray data;
    switch (fileType) {
        case QtQuickAppGeneratedFileInfo::MainQmlFile:
            data = readBlob(path(MainQmlOrigin), errorMessage);
            break;
        case QtQuickAppGeneratedFileInfo::MainPageQmlFile:
            data = readBlob(path(MainPageQmlOrigin), errorMessage);
            break;
        case QtQuickAppGeneratedFileInfo::AppViewerPriFile:
            data = readBlob(path(AppViewerPriOrigin), errorMessage);
            data.append(readBlob(path(DeploymentPriOrigin), errorMessage));
            *comment = ProFileComment;
            *versionAndCheckSum = true;
            break;
        case QtQuickAppGeneratedFileInfo::AppViewerCppFile:
            data = readBlob(path(AppViewerCppOrigin), errorMessage);
            *versionAndCheckSum = true;
            break;
        case QtQuickAppGeneratedFileInfo::AppViewerHFile:
        default:
            data = readBlob(path(AppViewerHOrigin), errorMessage);
            *versionAndCheckSum = true;
            break;
    }
    return data;
}

int QtQuickApp::stubVersionMinor() const
{
    return m_componentSet == QtQuick20Components ? 1 : 22;
}

QList<AbstractGeneratedFileInfo> QtQuickApp::updateableFiles(const QString &mainProFile) const
{
    QList<AbstractGeneratedFileInfo> result;
    static const struct {
        int fileType;
        QString fileName;
    } files[] = {
        {QtQuickAppGeneratedFileInfo::AppViewerPriFile, fileName(AppViewerPri)},
        {QtQuickAppGeneratedFileInfo::AppViewerHFile, fileName(AppViewerH)},
        {QtQuickAppGeneratedFileInfo::AppViewerCppFile, fileName(AppViewerCpp)}
    };
    const QFileInfo mainProFileInfo(mainProFile);
    const int size = sizeof(files) / sizeof(files[0]);
    for (int i = 0; i < size; ++i) {
        const QString fileName = mainProFileInfo.dir().absolutePath()
                + QLatin1Char('/') + appViewerOriginSubDir() + files[i].fileName;
        if (!QFile::exists(fileName))
            continue;
        QtQuickAppGeneratedFileInfo file;
        file.fileType = files[i].fileType;
        file.fileInfo = QFileInfo(fileName);
        file.currentVersion = AbstractMobileApp::makeStubVersion(stubVersionMinor());
        result.append(file);
    }
    if (result.count() != size)
        result.clear(); // All files must be found. No wrong/partial updates, please.
    return result;
}

QList<DeploymentFolder> QtQuickApp::deploymentFolders() const
{
    QList<DeploymentFolder> result;
    result.append(DeploymentFolder(path(QmlDirProFileRelative), QLatin1String("qml")));
    foreach (const QmlModule *module, m_modules)
        if (module->isExternal)
            result.append(DeploymentFolder(module->path(QmlModule::ContentDir), module->path(QmlModule::DeployedContentBase)));
    return result;
}

QString QtQuickApp::componentSetDir(ComponentSet componentSet) const
{
    switch (componentSet) {
    case Meego10Components:
        return QLatin1String("meego10");
    case QtQuick20Components:
        return QLatin1String("qtquick20");
    case QtQuick10Components:
    default:
        return QLatin1String("qtquick10");
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
