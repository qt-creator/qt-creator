/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
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

#include "qmljsplugindumper.h"
#include "qmljsmodelmanagerinterface.h"
#include "qmljsutils.h"

#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsviewercontext.h>
//#include <projectexplorer/session.h>
//#include <coreplugin/messagemanager.h>
#include <utils/filesystemwatcher.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QRegularExpression>

using namespace LanguageUtils;
using namespace QmlJS;

PluginDumper::PluginDumper(ModelManagerInterface *modelManager)
    : QObject(modelManager)
    , m_modelManager(modelManager)
    , m_pluginWatcher(0)
{
    qRegisterMetaType<QmlJS::ModelManagerInterface::ProjectInfo>("QmlJS::ModelManagerInterface::ProjectInfo");
}

Utils::FileSystemWatcher *PluginDumper::pluginWatcher()
{
    if (!m_pluginWatcher) {
        m_pluginWatcher = new Utils::FileSystemWatcher(this);
        m_pluginWatcher->setObjectName(QLatin1String("PluginDumperWatcher"));
        connect(m_pluginWatcher, &Utils::FileSystemWatcher::fileChanged,
                this, &PluginDumper::pluginChanged);
    }
    return m_pluginWatcher;
}

void PluginDumper::loadBuiltinTypes(const QmlJS::ModelManagerInterface::ProjectInfo &info)
{
    // move to the owning thread
    metaObject()->invokeMethod(this, "onLoadBuiltinTypes",
                               Q_ARG(QmlJS::ModelManagerInterface::ProjectInfo, info));
}

void PluginDumper::loadPluginTypes(const QString &libraryPath, const QString &importPath, const QString &importUri, const QString &importVersion)
{
    // move to the owning thread
    metaObject()->invokeMethod(this, "onLoadPluginTypes",
                               Q_ARG(QString, libraryPath),
                               Q_ARG(QString, importPath),
                               Q_ARG(QString, importUri),
                               Q_ARG(QString, importVersion));
}

void PluginDumper::scheduleRedumpPlugins()
{
    // move to the owning thread
    metaObject()->invokeMethod(this, "dumpAllPlugins", Qt::QueuedConnection);
}

void PluginDumper::scheduleMaybeRedumpBuiltins(const QmlJS::ModelManagerInterface::ProjectInfo &info)
{
    // move to the owning thread
    metaObject()->invokeMethod(this, "dumpBuiltins", Qt::QueuedConnection,
                               Q_ARG(QmlJS::ModelManagerInterface::ProjectInfo, info));
}

void PluginDumper::onLoadBuiltinTypes(const QmlJS::ModelManagerInterface::ProjectInfo &info, bool force)
{
    const QString baseImportsPath = info.qtQmlPath.isEmpty() ? info.qtImportsPath : info.qtQmlPath;
    if (info.qmlDumpPath.isEmpty() || baseImportsPath.isEmpty())
        return;

    const QString importsPath = QDir::cleanPath(baseImportsPath);
    if (m_runningQmldumps.values().contains(importsPath))
        return;

    LibraryInfo builtinInfo;
    if (!force) {
        const Snapshot snapshot = m_modelManager->snapshot();
        builtinInfo = snapshot.libraryInfo(baseImportsPath);
        if (builtinInfo.isValid())
            return;
    }
    builtinInfo = LibraryInfo(LibraryInfo::Found);
    m_modelManager->updateLibraryInfo(baseImportsPath, builtinInfo);

    // prefer QTDIR/qml/builtins.qmltypes if available
    const QString builtinQmltypesPath = baseImportsPath + QLatin1String("/builtins.qmltypes");
    if (QFile::exists(builtinQmltypesPath)) {
        loadQmltypesFile(QStringList(builtinQmltypesPath), baseImportsPath, builtinInfo);
        return;
    }
    // QTDIR/imports/QtQuick1/builtins.qmltypes was used in developer builds of 5.0.0, 5.0.1
    const QString builtinQmltypesPath2 = info.qtImportsPath
            + QLatin1String("/QtQuick1/builtins.qmltypes");
    if (QFile::exists(builtinQmltypesPath2)) {
        loadQmltypesFile(QStringList(builtinQmltypesPath2), baseImportsPath, builtinInfo);
        return;
    }

    runQmlDump(info, QStringList(QLatin1String("--builtins")), baseImportsPath);
    m_qtToInfo.insert(baseImportsPath, info);
}

static QString makeAbsolute(const QString &path, const QString &base)
{
    if (QFileInfo(path).isAbsolute())
        return path;
    return QString::fromLatin1("%1/%3").arg(base, path);
}

void PluginDumper::onLoadPluginTypes(const QString &libraryPath, const QString &importPath, const QString &importUri, const QString &importVersion)
{
    const QString canonicalLibraryPath = QDir::cleanPath(libraryPath);
    if (m_runningQmldumps.values().contains(canonicalLibraryPath))
        return;
    const Snapshot snapshot = m_modelManager->snapshot();
    const LibraryInfo libraryInfo = snapshot.libraryInfo(canonicalLibraryPath);
    if (libraryInfo.pluginTypeInfoStatus() != LibraryInfo::NoTypeInfo)
        return;

    // avoid inserting the same plugin twice
    int index;
    for (index = 0; index < m_plugins.size(); ++index) {
        if (m_plugins.at(index).qmldirPath == libraryPath)
            break;
    }
    if (index == m_plugins.size())
        m_plugins.append(Plugin());

    Plugin &plugin = m_plugins[index];
    plugin.qmldirPath = canonicalLibraryPath;
    plugin.importPath = importPath;
    plugin.importUri = importUri;
    plugin.importVersion = importVersion;

    // add default qmltypes file if it exists
    const QLatin1String defaultQmltypesFileName("plugins.qmltypes");
    const QString defaultQmltypesPath = makeAbsolute(defaultQmltypesFileName, canonicalLibraryPath);
    if (!plugin.typeInfoPaths.contains(defaultQmltypesPath) && QFile::exists(defaultQmltypesPath))
        plugin.typeInfoPaths += defaultQmltypesPath;

    // add typeinfo files listed in qmldir
    foreach (const QmlDirParser::TypeInfo &typeInfo, libraryInfo.typeInfos()) {
        QString pathNow = makeAbsolute(typeInfo.fileName, canonicalLibraryPath);
        if (!plugin.typeInfoPaths.contains(pathNow) && QFile::exists(pathNow))
            plugin.typeInfoPaths += pathNow;
    }

    // watch plugin libraries
    foreach (const QmlDirParser::Plugin &plugin, snapshot.libraryInfo(canonicalLibraryPath).plugins()) {
        const QString pluginLibrary = resolvePlugin(canonicalLibraryPath, plugin.path, plugin.name);
        if (!pluginLibrary.isEmpty()) {
            if (!pluginWatcher()->watchesFile(pluginLibrary))
                pluginWatcher()->addFile(pluginLibrary, Utils::FileSystemWatcher::WatchModifiedDate);
            m_libraryToPluginIndex.insert(pluginLibrary, index);
        }
    }

    // watch library qmltypes file
    if (!plugin.typeInfoPaths.isEmpty()) {
        foreach (const QString &path, plugin.typeInfoPaths) {
            if (!QFile::exists(path))
                continue;
            if (!pluginWatcher()->watchesFile(path))
                pluginWatcher()->addFile(path, Utils::FileSystemWatcher::WatchModifiedDate);
            m_libraryToPluginIndex.insert(path, index);
        }
    }

    dump(plugin);
}

void PluginDumper::dumpBuiltins(const QmlJS::ModelManagerInterface::ProjectInfo &info)
{
    // if the builtin types were generated with a different qmldump, regenerate!
    if (m_qtToInfo.contains(info.qtImportsPath)) {
        QmlJS::ModelManagerInterface::ProjectInfo oldInfo = m_qtToInfo.value(info.qtImportsPath);
        if (oldInfo.qmlDumpPath != info.qmlDumpPath
                || oldInfo.qmlDumpEnvironment != info.qmlDumpEnvironment) {
            m_qtToInfo.remove(info.qtImportsPath);
            onLoadBuiltinTypes(info, true);
        }
    }
}

void PluginDumper::dumpAllPlugins()
{
    foreach (const Plugin &plugin, m_plugins) {
        dump(plugin);
    }
}

static QString noTypeinfoError(const QString &libraryPath)
{
    return PluginDumper::tr("QML module does not contain information about components contained in plugins.\n\n"
                            "Module path: %1\n"
                            "See \"Using QML Modules with Plugins\" in the documentation.").arg(
                libraryPath);
}

static QString qmldumpErrorMessage(const QString &libraryPath, const QString &error)
{
    return noTypeinfoError(libraryPath) + QLatin1String("\n\n") +
            PluginDumper::tr("Automatic type dump of QML module failed.\nErrors:\n%1").
            arg(error) + QLatin1Char('\n');
}

static QString qmldumpFailedMessage(const QString &libraryPath, const QString &error)
{
    QString firstLines =
            QStringList(error.split(QLatin1Char('\n')).mid(0, 10)).join(QLatin1Char('\n'));
    return noTypeinfoError(libraryPath) + QLatin1String("\n\n") +
            PluginDumper::tr("Automatic type dump of QML module failed.\n"
                             "First 10 lines or errors:\n"
                             "\n"
                             "%1"
                             "\n"
                             "Check 'General Messages' output pane for details."
                             ).arg(firstLines);
}

static void printParseWarnings(const QString &libraryPath, const QString &warning)
{
    ModelManagerInterface::writeWarning(
                PluginDumper::tr("Warnings while parsing QML type information of %1:\n"
                                 "%2").arg(libraryPath, warning));
}

static QString qmlPluginDumpErrorMessage(QProcess *process)
{
    QString errorMessage;
    const QString binary = QDir::toNativeSeparators(process->program());
    switch (process->error()) {
    case QProcess::FailedToStart:
        errorMessage = PluginDumper::tr("\"%1\" failed to start: %2").arg(binary, process->errorString());
        break;
    case QProcess::Crashed:
        errorMessage = PluginDumper::tr("\"%1\" crashed.").arg(binary);
        break;
    case QProcess::Timedout:
        errorMessage = PluginDumper::tr("\"%1\" timed out.").arg(binary);
        break;
    case QProcess::ReadError:
    case QProcess::WriteError:
        errorMessage = PluginDumper::tr("I/O error running \"%1\".").arg(binary);
        break;
    case QProcess::UnknownError:
        if (process->exitCode())
            errorMessage = PluginDumper::tr("\"%1\" returned exit code %2.").arg(binary).arg(process->exitCode());
        break;
    }
    errorMessage += QLatin1Char('\n') + PluginDumper::tr("Arguments: %1").arg(process->arguments().join(QLatin1Char(' ')));
    if (process->error() != QProcess::FailedToStart) {
        const QString stdErr = QString::fromLocal8Bit(process->readAllStandardError());
        if (!stdErr.isEmpty()) {
            errorMessage += QLatin1Char('\n');
            errorMessage += stdErr;
        }
    }
    return errorMessage;
}

void PluginDumper::qmlPluginTypeDumpDone(int exitCode)
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;
    process->deleteLater();

    const QString libraryPath = m_runningQmldumps.take(process);
    if (libraryPath.isEmpty())
        return;
    const Snapshot snapshot = m_modelManager->snapshot();
    LibraryInfo libraryInfo = snapshot.libraryInfo(libraryPath);
    bool privatePlugin = libraryPath.endsWith(QLatin1String("private"));

    if (exitCode != 0) {
        const QString errorMessages = qmlPluginDumpErrorMessage(process);
        if (!privatePlugin)
            ModelManagerInterface::writeWarning(qmldumpErrorMessage(libraryPath, errorMessages));
        libraryInfo.setPluginTypeInfoStatus(LibraryInfo::DumpError, qmldumpFailedMessage(libraryPath, errorMessages));
    }

    const QByteArray output = process->readAllStandardOutput();
    QString error;
    QString warning;
    CppQmlTypesLoader::BuiltinObjects objectsList;
    QList<ModuleApiInfo> moduleApis;
    QStringList dependencies;
    CppQmlTypesLoader::parseQmlTypeDescriptions(output, &objectsList, &moduleApis, &dependencies,
                                                &error, &warning,
                                                QLatin1String("<dump of ") + libraryPath + QLatin1Char('>'));
    if (exitCode == 0) {
        if (!error.isEmpty()) {
            libraryInfo.setPluginTypeInfoStatus(LibraryInfo::DumpError,
                                                qmldumpErrorMessage(libraryPath, error));
            if (!privatePlugin)
                printParseWarnings(libraryPath, libraryInfo.pluginTypeInfoError());
        } else {
            libraryInfo.setMetaObjects(objectsList.values());
            libraryInfo.setModuleApis(moduleApis);
            libraryInfo.setPluginTypeInfoStatus(LibraryInfo::DumpDone);
        }

        if (!warning.isEmpty())
            printParseWarnings(libraryPath, warning);
    }
    libraryInfo.updateFingerprint();

    m_modelManager->updateLibraryInfo(libraryPath, libraryInfo);
}

void PluginDumper::qmlPluginTypeDumpError(QProcess::ProcessError)
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;
    process->deleteLater();

    const QString libraryPath = m_runningQmldumps.take(process);
    if (libraryPath.isEmpty())
        return;
    const QString errorMessages = qmlPluginDumpErrorMessage(process);
    const Snapshot snapshot = m_modelManager->snapshot();
    LibraryInfo libraryInfo = snapshot.libraryInfo(libraryPath);
    if (!libraryPath.endsWith(QLatin1String("private"), Qt::CaseInsensitive))
        ModelManagerInterface::writeWarning(qmldumpErrorMessage(libraryPath, errorMessages));
    libraryInfo.setPluginTypeInfoStatus(LibraryInfo::DumpError, qmldumpFailedMessage(libraryPath, errorMessages));
    libraryInfo.updateFingerprint();
    m_modelManager->updateLibraryInfo(libraryPath, libraryInfo);
}

void PluginDumper::pluginChanged(const QString &pluginLibrary)
{
    const int pluginIndex = m_libraryToPluginIndex.value(pluginLibrary, -1);
    if (pluginIndex == -1)
        return;

    const Plugin &plugin = m_plugins.at(pluginIndex);
    dump(plugin);
}

void PluginDumper::loadQmlTypeDescription(const QStringList &paths,
                                          QStringList &errors,
                                          QStringList &warnings,
                                          QList<FakeMetaObject::ConstPtr> &objects,
                                          QList<ModuleApiInfo> *moduleApi,
                                          QStringList *dependencies) const {
    for (const QString &p: paths) {
        Utils::FileReader reader;
        if (!reader.fetch(p, QFile::Text)) {
            errors += reader.errorString();
            continue;
        }
        QString error;
        QString warning;
        CppQmlTypesLoader::BuiltinObjects objs;
        QList<ModuleApiInfo> apis;
        QStringList deps;
        CppQmlTypesLoader::parseQmlTypeDescriptions(reader.data(), &objs, &apis, &deps,
                                                    &error, &warning, p);
        if (!error.isEmpty()) {
            errors += tr("Failed to parse \"%1\".\nError: %2").arg(p, error);
        } else {
            objects += objs.values();
            if (moduleApi)
                *moduleApi += apis;
            if (!deps.isEmpty())
                *dependencies += deps;
        }
        if (!warning.isEmpty())
            warnings += warning;
    }
}
/*!
 * \brief Build the path of an existing qmltypes file from a module name.
 * \param name
 * \return the module's qmltypes file path or an empty string if not found
 *
 * Look for \a name qmltypes file in model manager's import paths.
 *
 * \sa QmlJs::modulePath
 * \sa LinkPrivate::importNonFile
 */
QString PluginDumper::buildQmltypesPath(const QString &name) const
{
    QString qualifiedName;
    QString version;

    QRegularExpression import("^(?<name>[\\w|\\.]+)\\s+(?<major>\\d+)\\.(?<minor>\\d+)$");
    QRegularExpressionMatch m = import.match(name);
    if (m.hasMatch()) {
        qualifiedName = m.captured("name");
        version = m.captured("major") + QLatin1Char('.') + m.captured("minor");
    }

    const QString path = modulePath(qualifiedName, version, m_modelManager->importPathsNames());

    if (path.isEmpty())
        return QString();

    const QString filename = path + QLatin1String("/plugins.qmltypes");

    if (QFile::exists(filename))
        return filename;

    return QString();
}

/*!
 * \brief Recursively load dependencies.
 * \param dependencies
 * \param errors
 * \param warnings
 * \param objects
 *
 * Recursively load type descriptions of dependencies, collecting results
 * in \a objects.
 */
void PluginDumper::loadDependencies(const QStringList &dependencies,
                                    QStringList &errors,
                                    QStringList &warnings,
                                    QList<FakeMetaObject::ConstPtr> &objects,
                                    QSet<QString> *visited) const
{
    if (dependencies.isEmpty())
        return;

    QScopedPointer<QSet<QString>> visitedPtr(visited ? visited : new QSet<QString>());

    QStringList dependenciesPaths;
    QString path;
    for (const QString &name: dependencies) {
        path = buildQmltypesPath(name);
        if (!path.isNull())
            dependenciesPaths << path;
        visitedPtr->insert(name);
    }
    QStringList newDependencies;
    loadQmlTypeDescription(dependenciesPaths, errors, warnings, objects, 0, &newDependencies);
    newDependencies = (newDependencies.toSet() - *visitedPtr).toList();
    if (!newDependencies.isEmpty())
        loadDependencies(newDependencies, errors, warnings, objects, visitedPtr.take());
}

void PluginDumper::loadQmltypesFile(const QStringList &qmltypesFilePaths,
                                    const QString &libraryPath,
                                    QmlJS::LibraryInfo libraryInfo)
{
    QStringList errors;
    QStringList warnings;
    QList<FakeMetaObject::ConstPtr> objects;
    QList<ModuleApiInfo> moduleApis;
    QStringList dependencies;

    loadQmlTypeDescription(qmltypesFilePaths, errors, warnings, objects, &moduleApis, &dependencies);
    loadDependencies(dependencies, errors, warnings, objects);

    libraryInfo.setMetaObjects(objects);
    libraryInfo.setModuleApis(moduleApis);
    libraryInfo.setDependencies(dependencies);
    if (errors.isEmpty()) {
        libraryInfo.setPluginTypeInfoStatus(LibraryInfo::TypeInfoFileDone);
    } else {
        printParseWarnings(libraryPath, errors.join(QLatin1Char('\n')));
        errors.prepend(tr("Errors while reading typeinfo files:"));
        libraryInfo.setPluginTypeInfoStatus(LibraryInfo::TypeInfoFileError, errors.join(QLatin1Char('\n')));
    }

    if (!warnings.isEmpty())
        printParseWarnings(libraryPath, warnings.join(QLatin1String("\n")));

    libraryInfo.updateFingerprint();
    m_modelManager->updateLibraryInfo(libraryPath, libraryInfo);
}

void PluginDumper::runQmlDump(const QmlJS::ModelManagerInterface::ProjectInfo &info,
    const QStringList &arguments, const QString &importPath)
{
    QDir wd = QDir(importPath);
    wd.cdUp();
    QProcess *process = new QProcess(this);
    process->setEnvironment(info.qmlDumpEnvironment.toStringList());
    QString workingDir = wd.canonicalPath();
    process->setWorkingDirectory(workingDir);
    connect(process, static_cast<void (QProcess::*)(int)>(&QProcess::finished),
            this, &PluginDumper::qmlPluginTypeDumpDone);
    connect(process, &QProcess::errorOccurred, this, &PluginDumper::qmlPluginTypeDumpError);
    process->start(info.qmlDumpPath, arguments);
    m_runningQmldumps.insert(process, importPath);
}

void PluginDumper::dump(const Plugin &plugin)
{
    ModelManagerInterface::ProjectInfo info = m_modelManager->defaultProjectInfo();
    const Snapshot snapshot = m_modelManager->snapshot();
    LibraryInfo libraryInfo = snapshot.libraryInfo(plugin.qmldirPath);

    // if there are type infos, don't dump!
    if (!plugin.typeInfoPaths.isEmpty()) {
        if (!libraryInfo.isValid())
            return;

        loadQmltypesFile(plugin.typeInfoPaths, plugin.qmldirPath, libraryInfo);
        return;
    }

    if (plugin.importUri.isEmpty())
        return; // initial scan without uri, ignore

    if (!info.tryQmlDump || info.qmlDumpPath.isEmpty()) {
        if (!libraryInfo.isValid())
            return;

        QString errorMessage;
        if (!info.tryQmlDump) {
            errorMessage = noTypeinfoError(plugin.qmldirPath);
        } else {
            errorMessage = qmldumpErrorMessage(plugin.qmldirPath,
                    tr("Could not locate the helper application for dumping type information from C++ plugins.\n"
                       "Please build the qmldump application on the Qt version options page."));
        }

        libraryInfo.setPluginTypeInfoStatus(LibraryInfo::DumpError, errorMessage);
        libraryInfo.updateFingerprint();
        m_modelManager->updateLibraryInfo(plugin.qmldirPath, libraryInfo);
        return;
    }

    QStringList args;
    if (info.qmlDumpHasRelocatableFlag)
        args << QLatin1String("-nonrelocatable");
    args << plugin.importUri;
    args << plugin.importVersion;
    args << (plugin.importPath.isEmpty() ? QLatin1String(".") : plugin.importPath);
    runQmlDump(info, args, plugin.qmldirPath);
}

/*!
  Returns the result of the merge of \a baseName with \a path, \a suffixes, and \a prefix.
  The \a prefix must contain the dot.

  \a qmldirPath is the location of the qmldir file.

  Adapted from QDeclarativeImportDatabase::resolvePlugin.
*/
QString PluginDumper::resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath,
                                    const QString &baseName, const QStringList &suffixes,
                                    const QString &prefix)
{
    QStringList searchPaths;
    searchPaths.append(QLatin1String("."));

    bool qmldirPluginPathIsRelative = QDir::isRelativePath(qmldirPluginPath);
    if (!qmldirPluginPathIsRelative)
        searchPaths.prepend(qmldirPluginPath);

    foreach (const QString &pluginPath, searchPaths) {

        QString resolvedPath;

        if (pluginPath == QLatin1String(".")) {
            if (qmldirPluginPathIsRelative)
                resolvedPath = qmldirPath.absoluteFilePath(qmldirPluginPath);
            else
                resolvedPath = qmldirPath.absolutePath();
        } else {
            resolvedPath = pluginPath;
        }

        QDir dir(resolvedPath);
        foreach (const QString &suffix, suffixes) {
            QString pluginFileName = prefix;

            pluginFileName += baseName;
            pluginFileName += suffix;

            QFileInfo fileInfo(dir, pluginFileName);

            if (fileInfo.exists())
                return fileInfo.absoluteFilePath();
        }
    }

    return QString();
}

/*!
  Returns the result of the merge of \a baseName with \a dir and the platform suffix.

  Adapted from QDeclarativeImportDatabase::resolvePlugin.

  \table
  \header \li Platform \li Valid suffixes
  \row \li Windows     \li \c .dll
  \row \li Unix/Linux  \li \c .so
  \row \li AIX  \li \c .a
  \row \li HP-UX       \li \c .sl, \c .so (HP-UXi)
  \row \li Mac OS X    \li \c .dylib, \c .bundle, \c .so
  \endtable

  Version number on unix are ignored.
*/
QString PluginDumper::resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath,
                                    const QString &baseName)
{
    QStringList validSuffixList;
    QString prefix;
    if (Utils::HostOsInfo::isWindowsHost()) {
        // try a qmake-style debug build first
        validSuffixList = QStringList({"d.dll",  ".dll"});
    } else if (Utils::HostOsInfo::isMacHost()) {
        // try a qmake-style debug build first
        validSuffixList = QStringList({"_debug.dylib", ".dylib", ".so", ".bundle", "lib"});
    } else {
        // Examples of valid library names:
        //  libfoo.so
        prefix = "lib";
#if defined(Q_OS_HPUX)
/*
    See "HP-UX Linker and Libraries User's Guide", section "Link-time Differences between PA-RISC and IPF":
    "In PA-RISC (PA-32 and PA-64) shared libraries are suffixed with .sl. In IPF (32-bit and 64-bit),
    the shared libraries are suffixed with .so. For compatibility, the IPF linker also supports the .sl suffix."
 */
        validSuffixList << QLatin1String(".sl");
# if defined __ia64
        validSuffixList << QLatin1String(".so");
# endif
#elif defined(Q_OS_AIX)
        validSuffixList << QLatin1String(".a") << QLatin1String(".so");
#else
        validSuffixList << QLatin1String(".so");
#endif
    }
    return resolvePlugin(qmldirPath, qmldirPluginPath, baseName, validSuffixList, prefix);
}
