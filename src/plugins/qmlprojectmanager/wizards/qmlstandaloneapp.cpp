/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlstandaloneapp.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>

#ifndef CREATORLESSTEST
#include <coreplugin/icore.h>
#endif // CREATORLESSTEST

namespace QmlProjectManager {
namespace Internal {

const QLatin1String qmldir("qmldir");
const QLatin1String qmldir_plugin("plugin");

QmlModule::QmlModule(const QString &name, const QFileInfo &rootDir, const QFileInfo &qmldir,
                     bool isExternal, QmlStandaloneApp *qmlStandaloneApp)
    : name(name)
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
            const QDir proFile(qmlStandaloneApp->path(QmlStandaloneApp::AppProfilePath));
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

QmlStandaloneApp::QmlStandaloneApp()
    : m_loadDummyData(false)
    , m_orientation(Auto)
    , m_networkEnabled(false)
{
}

QmlStandaloneApp::~QmlStandaloneApp()
{
    clearModulesAndPlugins();
}

QString QmlStandaloneApp::symbianUidForPath(const QString &path)
{
    quint32 hash = 5381;
    for (int i = 0; i < path.size(); ++i) {
        const char c = path.at(i).toAscii();
        hash ^= c + ((c - i) << i % 20) + ((c + i) << (i + 5) % 20) + ((c - 2 * i) << (i + 10) % 20) + ((c + 2 * i) << (i + 15) % 20);
    }
    return QString::fromLatin1("0xE")
            + QString::fromLatin1("%1").arg(hash, 7, 16, QLatin1Char('0')).right(7);
}

void QmlStandaloneApp::setMainQmlFile(const QString &qmlFile)
{
    m_mainQmlFile.setFile(qmlFile);
}

QString QmlStandaloneApp::mainQmlFile() const
{
    return path(MainQml);
}

void QmlStandaloneApp::setOrientation(Orientation orientation)
{
    m_orientation = orientation;
}

QmlStandaloneApp::Orientation QmlStandaloneApp::orientation() const
{
    return m_orientation;
}

void QmlStandaloneApp::setProjectName(const QString &name)
{
    m_projectName = name;
}

QString QmlStandaloneApp::projectName() const
{
    return m_projectName;
}

void QmlStandaloneApp::setProjectPath(const QString &path)
{
    m_projectPath.setFile(path);
}

void QmlStandaloneApp::setSymbianSvgIcon(const QString &icon)
{
    m_symbianSvgIcon = icon;
}

QString QmlStandaloneApp::symbianSvgIcon() const
{
    return path(SymbianSvgIconOrigin);
}

void QmlStandaloneApp::setSymbianTargetUid(const QString &uid)
{
    m_symbianTargetUid = uid;
}

QString QmlStandaloneApp::symbianTargetUid() const
{
    return !m_symbianTargetUid.isEmpty() ? m_symbianTargetUid
        : symbianUidForPath(path(AppProfile));
}

void QmlStandaloneApp::setLoadDummyData(bool loadIt)
{
    m_loadDummyData = loadIt;
}

bool QmlStandaloneApp::loadDummyData() const
{
    return m_loadDummyData;
}

void QmlStandaloneApp::setNetworkEnabled(bool enabled)
{
    m_networkEnabled = enabled;
}

bool QmlStandaloneApp::networkEnabled() const
{
    return m_networkEnabled;
}

bool QmlStandaloneApp::setExternalModules(const QStringList &moduleNames,
                                          const QStringList &importPaths)
{
    clearModulesAndPlugins();
    m_importPaths.clear();
    foreach (const QFileInfo &importPath, importPaths) {
        if (!importPath.exists()) {
            m_error = tr("The Qml import path '%1' cannot be found.")
                      .arg(QDir::toNativeSeparators(importPath.filePath()));
            return false;
        } else {
            m_importPaths.append(importPath.canonicalFilePath());
        }
    }
    foreach (const QString &moduleName, moduleNames) {
        QString modulePath = moduleName;
        modulePath.replace(QLatin1Char('.'), QLatin1Char('/'));
        const int modulesCount = m_modules.count();
        foreach (const QFileInfo &importPath, m_importPaths) {
            const QFileInfo qmlDirFile(
                    importPath.absoluteFilePath() + QLatin1Char('/')
                    + modulePath + QLatin1Char('/') + qmldir);
            if (qmlDirFile.exists()) {
                if (!addExternalModule(moduleName, importPath, qmlDirFile))
                    return false;
                break;
            }
        }
        if (modulesCount == m_modules.count()) { // no module was added
            m_error = tr("The Qml module '%1' cannot be found.").arg(moduleName);
            return false;
        }
    }
    m_error.clear();
    return true;
}

QString QmlStandaloneApp::path(Path path) const
{
    const QString qmlSubDir = QLatin1String("qml/")
                              + (useExistingMainQml() ? m_mainQmlFile.dir().dirName() : m_projectName)
                              + QLatin1Char('/');
    const QString originsRoot = templatesRoot();
    const QString cppOriginsSubDir = QLatin1String("cpp/");
    const QString cppTargetSubDir = cppOriginsSubDir;
    const QString qmlExtension = QLatin1String(".qml");
    const QString appPriFileName = QLatin1String("qmlapplication.pri");
    const QString mainCppFileName = QLatin1String("main.cpp");
    const QString appViewCppFileName = QLatin1String("qmlapplicationview.cpp");
    const QString appViewHFileName = QLatin1String("qmlapplicationview.h");
    const QString symbianIconFileName = QLatin1String("symbianicon.svg");
    const char* const errorMessage = "QmlStandaloneApp::path() needs more work";
    const QString pathBase = m_projectPath.absoluteFilePath() + QLatin1Char('/')
                             + m_projectName + QLatin1Char('/');
    const QDir appProFilePath(pathBase);

    switch (path) {
        case MainQml:                       return useExistingMainQml() ? m_mainQmlFile.canonicalFilePath()
                                                : pathBase + qmlSubDir + m_projectName + qmlExtension;
        case MainQmlDeployed:               return useExistingMainQml() ? qmlSubDir + m_mainQmlFile.fileName()
                                                : QString(qmlSubDir + m_projectName + qmlExtension);
        case MainQmlOrigin:                 return originsRoot + QLatin1String("qml/app/app.qml");
        case MainCpp:                       return pathBase + cppTargetSubDir + mainCppFileName;
        case MainCppOrigin:                 return originsRoot + cppOriginsSubDir + mainCppFileName;
        case MainCppProFileRelative:        return cppTargetSubDir + mainCppFileName;
        case AppProfile:                    return pathBase + m_projectName + QLatin1String(".pro");
        case AppProfileOrigin:              return originsRoot + QLatin1String("app.pro");
        case AppProfilePath:                return pathBase;
        case AppPri:                        return pathBase + appPriFileName;
        case AppPriOrigin:                  return originsRoot + appPriFileName;
        case AppViewerCpp:                  return pathBase + cppTargetSubDir + appViewCppFileName;
        case AppViewerCppOrigin:            return originsRoot + cppOriginsSubDir + appViewCppFileName;
        case AppViewerCppProFileRelative:   return cppTargetSubDir + appViewCppFileName;
        case AppViewerH:                    return pathBase + cppTargetSubDir + appViewHFileName;
        case AppViewerHOrigin:              return originsRoot + cppOriginsSubDir + appViewHFileName;
        case AppViewerHProFileRelative:     return cppTargetSubDir + appViewHFileName;
        case SymbianSvgIcon:                return pathBase + cppTargetSubDir + symbianIconFileName;
        case SymbianSvgIconOrigin:          return !m_symbianSvgIcon.isEmpty() ? m_symbianSvgIcon
                                                : originsRoot + cppOriginsSubDir + symbianIconFileName;
        case SymbianSvgIconProFileRelative: return cppTargetSubDir + symbianIconFileName;
        case QmlDir:                        return pathBase + qmlSubDir;
        case QmlDirProFileRelative:         return useExistingMainQml() ? appProFilePath.relativeFilePath(m_mainQmlFile.canonicalPath())
                                                : QString(qmlSubDir).remove(qmlSubDir.length() - 1, 1);
        case ModulesDir:                    return QLatin1String("modules");
        default:                            qFatal(errorMessage);
    }
    return QString();
}

static QString insertParameter(const QString &line, const QString &parameter)
{
    return QString(line).replace(QRegExp(QLatin1String("\\([^()]+\\)")),
                                 QLatin1Char('(') + parameter + QLatin1Char(')'));
}

QByteArray QmlStandaloneApp::generateMainCpp(const QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    QFile sourceFile(path(MainCppOrigin));
    sourceFile.open(QIODevice::ReadOnly);
    Q_ASSERT(sourceFile.isOpen());
    QTextStream in(&sourceFile);

    QByteArray mainCppContent;
    QTextStream out(&mainCppContent, QIODevice::WriteOnly);

    QString line;
    do {
        line = in.readLine();
        if (line.contains(QLatin1String("// MAINQML"))) {
            line = insertParameter(line, QLatin1Char('"') + path(MainQmlDeployed) + QLatin1Char('"'));
        } else if (line.contains(QLatin1String("// ADDIMPORTPATH"))) {
            if (m_modules.isEmpty())
                continue;
            else
                line = insertParameter(line, QLatin1Char('"') + path(ModulesDir) + QLatin1Char('"'));
        } else if (line.contains(QLatin1String("// ORIENTATION"))) {
            if (m_orientation == Auto)
                continue;
            else
                line = insertParameter(line, QLatin1String("QmlApplicationView::")
                                       + QLatin1String(m_orientation == LockLandscape ?
                                                       "LockLandscape" : "LockPortrait"));
        } else if (line.contains(QLatin1String("// LOADDUMMYDATA"))) {
            continue;
        }
        const int commentIndex = line.indexOf(QLatin1String(" //"));
        if (commentIndex != -1)
            line.truncate(commentIndex);
        out << line << endl;
    } while (!line.isNull());

    return mainCppContent;
}

QByteArray QmlStandaloneApp::generateProFile(const QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const QChar comment = QLatin1Char('#');
    QFile proFile(path(AppProfileOrigin));
    proFile.open(QIODevice::ReadOnly);
    Q_ASSERT(proFile.isOpen());
    QTextStream in(&proFile);

    QByteArray proFileContent;
    QTextStream out(&proFileContent, QIODevice::WriteOnly);

    QString line;
    QString valueOnNextLine;
    bool uncommentNextLine = false;
    do {
        line = in.readLine();

        if (line.contains(QLatin1String("# TARGETUID3"))) {
            valueOnNextLine = symbianTargetUid();
        } else if (line.contains(QLatin1String("# DEPLOYMENTFOLDERS"))) {
            // Eat lines
            do {
                line = in.readLine();
            } while (!(line.isNull() || line.contains(QLatin1String("# DEPLOYMENTFOLDERS_END"))));
            QStringList folders;
            out << "folder_01.source = " << path(QmlDirProFileRelative) << endl;
            out << "folder_01.target = qml" << endl;
            folders.append(QLatin1String("folder_01"));
            int foldersCount = 1;
            foreach (const QmlModule *module, m_modules) {
                if (module->isExternal) {
                    foldersCount ++;
                    const QString folder =
                            QString::fromLatin1("folder_%1").arg(foldersCount, 2, 10, QLatin1Char('0'));
                    folders.append(folder);
                    out << folder << ".source = " << module->path(QmlModule::ContentDir) << endl;
                    out << folder << ".target = " << module->path(QmlModule::DeployedContentBase) << endl;
                }
            }
            out << "DEPLOYMENTFOLDERS = " << folders.join(QLatin1String(" ")) << endl;
        } else if (line.contains(QLatin1String("# ORIENTATIONLOCK")) && m_orientation == QmlStandaloneApp::Auto) {
            uncommentNextLine = true;
        } else if (line.contains(QLatin1String("# NETWORKACCESS")) && !m_networkEnabled) {
            uncommentNextLine = true;
        } else if (line.contains(QLatin1String("# QMLINSPECTOR"))) {
            // ### disabled for now; figure out the private headers problem first.
            //uncommentNextLine = true;
        }

        // Remove all marker comments
        if (line.trimmed().startsWith(comment)
            && line.trimmed().endsWith(comment))
            continue;

        if (!valueOnNextLine.isEmpty()) {
            out << line.left(line.indexOf(QLatin1Char('=')) + 2)
                << QDir::fromNativeSeparators(valueOnNextLine) << endl;
            valueOnNextLine.clear();
            continue;
        }

        if (uncommentNextLine) {
            out << comment << line << endl;
            uncommentNextLine = false;
            continue;
        }
        out << line << endl;
    } while (!line.isNull());

    return proFileContent;
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
        m_error = tr("Invalid '%1' entry in '%2' of module '%3'.")
                  .arg(qmldir_plugin).arg(qmldir).arg(module->name);
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
        m_error = tr("No .pro file for plugin '%1' cannot be found.").arg(name);
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
        QTextStream ts(&qmlDirFile);
        QString line;
        do {
            line = ts.readLine().trimmed();
            if (line.startsWith(qmldir_plugin) && !addCppPlugin(line, module))
                return false;
        } while (!line.isNull());
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
QString QmlStandaloneApp::templatesRoot()
{
    return Core::ICore::instance()->resourcePath() + QLatin1String("/templates/qmlapp/");
}

static Core::GeneratedFile generateFileCopy(const QString &source,
                                            const QString &target,
                                            bool openEditor = false)
{
    QFile sourceFile(source);
    sourceFile.open(QIODevice::ReadOnly);
    Q_ASSERT(sourceFile.isOpen());
    Core::GeneratedFile generatedFile(target);
    generatedFile.setBinary(true);
    generatedFile.setBinaryContents(sourceFile.readAll());
    if (openEditor)
        generatedFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    return generatedFile;
}

Core::GeneratedFiles QmlStandaloneApp::generateFiles(QString *errorMessage) const
{
    Core::GeneratedFiles files;

    Core::GeneratedFile generatedProFile(path(AppProfile));
    generatedProFile.setContents(generateProFile(errorMessage));
    generatedProFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    files.append(generatedProFile);
    files.append(generateFileCopy(path(AppPriOrigin), path(AppPri)));

    if (!useExistingMainQml())
        files.append(generateFileCopy(path(MainQmlOrigin), path(MainQml), true));

    Core::GeneratedFile generatedMainCppFile(path(MainCpp));
    generatedMainCppFile.setContents(generateMainCpp(errorMessage));
    files.append(generatedMainCppFile);

    files.append(generateFileCopy(path(AppViewerCppOrigin), path(AppViewerCpp)));
    files.append(generateFileCopy(path(AppViewerHOrigin), path(AppViewerH)));
    files.append(generateFileCopy(path(SymbianSvgIconOrigin), path(SymbianSvgIcon)));

    return files;
}
#endif // CREATORLESSTEST

bool QmlStandaloneApp::useExistingMainQml() const
{
    return !m_mainQmlFile.filePath().isEmpty();
}

QString QmlStandaloneApp::error() const
{
    return m_error;
}

const QList<QmlModule*> QmlStandaloneApp::modules() const
{
    return m_modules;
}

} // namespace Internal
} // namespace QmlProjectManager
