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

#include "qmljsplugindumper.h"
#include "qmljsmodelmanager.h"

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsinterpreter.h>
#include <projectexplorer/filewatcher.h>
#include <projectexplorer/projectexplorer.h>
#include <coreplugin/messagemanager.h>

#include <QtCore/QDir>

using namespace LanguageUtils;
using namespace QmlJS;
using namespace QmlJSTools;
using namespace QmlJSTools::Internal;

PluginDumper::PluginDumper(ModelManager *modelManager)
    : QObject(modelManager)
    , m_modelManager(modelManager)
    , m_pluginWatcher(new ProjectExplorer::FileWatcher(this))
{
    connect(m_pluginWatcher, SIGNAL(fileChanged(QString)), SLOT(pluginChanged(QString)));
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

void PluginDumper::onLoadPluginTypes(const QString &libraryPath, const QString &importPath, const QString &importUri, const QString &importVersion)
{
    const QString canonicalLibraryPath = QDir::cleanPath(libraryPath);
    if (m_runningQmldumps.values().contains(canonicalLibraryPath))
        return;
    const Snapshot snapshot = m_modelManager->snapshot();
    const LibraryInfo libraryInfo = snapshot.libraryInfo(canonicalLibraryPath);
    if (libraryInfo.dumpStatus() != LibraryInfo::DumpNotStartedOrRunning)
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

    // watch plugin libraries
    foreach (const QmlDirParser::Plugin &plugin, snapshot.libraryInfo(canonicalLibraryPath).plugins()) {
        const QString pluginLibrary = resolvePlugin(canonicalLibraryPath, plugin.path, plugin.name);
        m_pluginWatcher->addFile(pluginLibrary);
        m_libraryToPluginIndex.insert(pluginLibrary, index);
    }

    // watch library xml file
    if (plugin.hasPredumpedQmlTypesFile()) {
        const QString &path = plugin.predumpedQmlTypesFilePath();
        m_pluginWatcher->addFile(path);
        m_libraryToPluginIndex.insert(path, index);
    }

    dump(plugin);
}

void PluginDumper::scheduleCompleteRedump()
{
    metaObject()->invokeMethod(this, "dumpAllPlugins", Qt::QueuedConnection);
}

void PluginDumper::dumpAllPlugins()
{
    foreach (const Plugin &plugin, m_plugins)
        dump(plugin);
}

static QString qmldumpErrorMessage(const QString &libraryPath, const QString &error)
{
    return PluginDumper::tr("Type dump of QML plugin in %0 failed.\nErrors:\n%1\n").arg(libraryPath, error);
}

static QString qmldumpFailedMessage(const QString &error)
{
    QString firstLines =
            QStringList(error.split(QLatin1Char('\n')).mid(0, 10)).join(QLatin1String("\n"));
    return PluginDumper::tr("Type dump of C++ plugin failed.\n"
                            "First 10 lines or errors:\n"
                            "\n"
                            "%1"
                            "\n"
                            "Check 'General Messages' output pane for details."
                            ).arg(firstLines);
}

static QList<FakeMetaObject::ConstPtr> parseHelper(const QByteArray &qmlTypeDescriptions, QString *error)
{
    QList<FakeMetaObject::ConstPtr> ret;
    QHash<QString, FakeMetaObject::ConstPtr> newObjects;
    *error = Interpreter::CppQmlTypesLoader::parseQmlTypeDescriptions(qmlTypeDescriptions, &newObjects);

    if (error->isEmpty()) {
        ret = newObjects.values();
    }
    return ret;
}

void PluginDumper::qmlPluginTypeDumpDone(int exitCode)
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;
    process->deleteLater();

    const QString libraryPath = m_runningQmldumps.take(process);
    const Snapshot snapshot = m_modelManager->snapshot();
    LibraryInfo libraryInfo = snapshot.libraryInfo(libraryPath);

    if (exitCode != 0) {
        Core::MessageManager *messageManager = Core::MessageManager::instance();
        const QString errorMessages = process->readAllStandardError();
        messageManager->printToOutputPane(qmldumpErrorMessage(libraryPath, errorMessages));
        libraryInfo.setDumpStatus(LibraryInfo::DumpError, qmldumpFailedMessage(errorMessages));
    }

    const QByteArray output = process->readAllStandardOutput();
    QString error;
    QList<FakeMetaObject::ConstPtr> objectsList = parseHelper(output, &error);
    if (exitCode == 0 && !error.isEmpty()) {
        libraryInfo.setDumpStatus(LibraryInfo::DumpError, tr("Type dump of C++ plugin failed. Parse error:\n'%1'").arg(error));
    }

    if (exitCode == 0 && error.isEmpty()) {
        libraryInfo.setMetaObjects(objectsList);
// ### disabled code path for running qmldump to get Qt's builtins
//        if (libraryPath.isEmpty())
//            Interpreter::CppQmlTypesLoader::builtinObjects.append(objectsList);
        libraryInfo.setDumpStatus(LibraryInfo::DumpDone);
    }

    if (!libraryPath.isEmpty())
        m_modelManager->updateLibraryInfo(libraryPath, libraryInfo);
}

void PluginDumper::qmlPluginTypeDumpError(QProcess::ProcessError)
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;
    process->deleteLater();

    const QString libraryPath = m_runningQmldumps.take(process);

    Core::MessageManager *messageManager = Core::MessageManager::instance();
    const QString errorMessages = process->readAllStandardError();
    messageManager->printToOutputPane(qmldumpErrorMessage(libraryPath, errorMessages));

    if (!libraryPath.isEmpty()) {
        const Snapshot snapshot = m_modelManager->snapshot();
        LibraryInfo libraryInfo = snapshot.libraryInfo(libraryPath);
        libraryInfo.setDumpStatus(LibraryInfo::DumpError, qmldumpFailedMessage(errorMessages));
        m_modelManager->updateLibraryInfo(libraryPath, libraryInfo);
    }
}

void PluginDumper::pluginChanged(const QString &pluginLibrary)
{
    const int pluginIndex = m_libraryToPluginIndex.value(pluginLibrary, -1);
    if (pluginIndex == -1)
        return;

    const Plugin &plugin = m_plugins.at(pluginIndex);
    dump(plugin);
}

void PluginDumper::dump(const Plugin &plugin)
{
    if (plugin.hasPredumpedQmlTypesFile()) {
        const Snapshot snapshot = m_modelManager->snapshot();
        LibraryInfo libraryInfo = snapshot.libraryInfo(plugin.qmldirPath);
        if (!libraryInfo.isValid())
            return;

        const QString &path = plugin.predumpedQmlTypesFilePath();
        QFile libraryQmlTypesFile(path);
        if (!libraryQmlTypesFile.open(QFile::ReadOnly | QFile::Text)) {
            libraryInfo.setDumpStatus(LibraryInfo::DumpError,
                                      tr("Could not open file '%1' for reading.").arg(path));
            m_modelManager->updateLibraryInfo(plugin.qmldirPath, libraryInfo);
            return;
        }

        const QByteArray qmlTypeDescriptions = libraryQmlTypesFile.readAll();
        libraryQmlTypesFile.close();

        QString error;
        const QList<FakeMetaObject::ConstPtr> objectsList = parseHelper(qmlTypeDescriptions, &error);

        if (error.isEmpty()) {
            libraryInfo.setMetaObjects(objectsList);
            libraryInfo.setDumpStatus(LibraryInfo::DumpDone);
        } else {
            libraryInfo.setDumpStatus(LibraryInfo::DumpError,
                                      tr("Failed to parse '%1'.\nError: %2").arg(path, error));
        }
        m_modelManager->updateLibraryInfo(plugin.qmldirPath, libraryInfo);
        return;
    }

    ProjectExplorer::Project *activeProject = ProjectExplorer::ProjectExplorerPlugin::instance()->startupProject();
    if (!activeProject)
        return;

    ModelManagerInterface::ProjectInfo info = m_modelManager->projectInfo(activeProject);

    if (info.qmlDumpPath.isEmpty()) {
        const Snapshot snapshot = m_modelManager->snapshot();
        LibraryInfo libraryInfo = snapshot.libraryInfo(plugin.qmldirPath);
        if (!libraryInfo.isValid())
            return;

        libraryInfo.setDumpStatus(LibraryInfo::DumpError,
                                  tr("Could not locate the helper application for dumping type information from C++ plugins.\n"
                                     "Please build the debugging helpers on the Qt version options page."));
        m_modelManager->updateLibraryInfo(plugin.qmldirPath, libraryInfo);
        return;
    }

    QProcess *process = new QProcess(this);
    process->setEnvironment(info.qmlDumpEnvironment.toStringList());
    connect(process, SIGNAL(finished(int)), SLOT(qmlPluginTypeDumpDone(int)));
    connect(process, SIGNAL(error(QProcess::ProcessError)), SLOT(qmlPluginTypeDumpError(QProcess::ProcessError)));
    QStringList args;
    args << QLatin1String("--notrelocatable"); // ### temporary until relocatable libraries work
    args << plugin.importUri;
    args << plugin.importVersion;
    args << plugin.importPath;
    process->start(info.qmlDumpPath, args);
    m_runningQmldumps.insert(process, plugin.qmldirPath);
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
  \header \i Platform \i Valid suffixes
  \row \i Windows     \i \c .dll
  \row \i Unix/Linux  \i \c .so
  \row \i AIX  \i \c .a
  \row \i HP-UX       \i \c .sl, \c .so (HP-UXi)
  \row \i Mac OS X    \i \c .dylib, \c .bundle, \c .so
  \row \i Symbian     \i \c .dll
  \endtable

  Version number on unix are ignored.
*/
QString PluginDumper::resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath,
                                    const QString &baseName)
{
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    return resolvePlugin(qmldirPath, qmldirPluginPath, baseName,
                         QStringList()
                         << QLatin1String("d.dll") // try a qmake-style debug build first
                         << QLatin1String(".dll"));
#elif defined(Q_OS_DARWIN)
    return resolvePlugin(qmldirPath, qmldirPluginPath, baseName,
                         QStringList()
                         << QLatin1String("_debug.dylib") // try a qmake-style debug build first
                         << QLatin1String(".dylib")
                         << QLatin1String(".so")
                         << QLatin1String(".bundle"),
                         QLatin1String("lib"));
#else  // Generic Unix
    QStringList validSuffixList;

#  if defined(Q_OS_HPUX)
/*
    See "HP-UX Linker and Libraries User's Guide", section "Link-time Differences between PA-RISC and IPF":
    "In PA-RISC (PA-32 and PA-64) shared libraries are suffixed with .sl. In IPF (32-bit and 64-bit),
    the shared libraries are suffixed with .so. For compatibility, the IPF linker also supports the .sl suffix."
 */
    validSuffixList << QLatin1String(".sl");
#   if defined __ia64
    validSuffixList << QLatin1String(".so");
#   endif
#  elif defined(Q_OS_AIX)
    validSuffixList << QLatin1String(".a") << QLatin1String(".so");
#  elif defined(Q_OS_UNIX)
    validSuffixList << QLatin1String(".so");
#  endif

    // Examples of valid library names:
    //  libfoo.so

    return resolvePlugin(qmldirPath, qmldirPluginPath, baseName, validSuffixList, QLatin1String("lib"));
#endif
}

bool PluginDumper::Plugin::hasPredumpedQmlTypesFile() const
{
    return QFileInfo(predumpedQmlTypesFilePath()).isFile();
}

QString PluginDumper::Plugin::predumpedQmlTypesFilePath() const
{
    return QString("%1%2plugins.qmltypes").arg(qmldirPath, QDir::separator());
}
