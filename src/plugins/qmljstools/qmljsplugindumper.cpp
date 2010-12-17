/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

void PluginDumper::loadPluginTypes(const QString &libraryPath, const QString &importPath, const QString &importUri)
{
    // move to the owning thread
    metaObject()->invokeMethod(this, "onLoadPluginTypes",
                               Q_ARG(QString, libraryPath),
                               Q_ARG(QString, importPath),
                               Q_ARG(QString, importUri));
}

void PluginDumper::onLoadPluginTypes(const QString &libraryPath, const QString &importPath, const QString &importUri)
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

    // watch plugin libraries
    foreach (const QmlDirParser::Plugin &plugin, snapshot.libraryInfo(canonicalLibraryPath).plugins()) {
        const QString pluginLibrary = resolvePlugin(canonicalLibraryPath, plugin.path, plugin.name);
        m_pluginWatcher->addFile(pluginLibrary);
        m_libraryToPluginIndex.insert(pluginLibrary, index);
    }

    // watch library xml file
    if (plugin.hasPredumpedXmlFile()) {
        const QString &path = plugin.predumpedXmlFilePath();
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

static QString qmldumpFailedMessage()
{
    return PluginDumper::tr("Type dump of C++ plugin failed.\nCheck 'General Messages' output pane for details.");
}

static QList<const Interpreter::FakeMetaObject *> parseHelper(const QByteArray &xml, QString *error)
{
    QList<const Interpreter::FakeMetaObject *> ret;
    QMap<QString, Interpreter::FakeMetaObject *> newObjects;
    *error = Interpreter::CppQmlTypesLoader::parseQmlTypeXml(xml, &newObjects);

    if (error->isEmpty()) {
        // convert from QList<T *> to QList<const T *>
        QMapIterator<QString, Interpreter::FakeMetaObject *> it(newObjects);
        while (it.hasNext()) {
            it.next();
            ret.append(it.value());
        }
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
        messageManager->printToOutputPane(qmldumpErrorMessage(libraryPath, process->readAllStandardError()));
        libraryInfo.setDumpStatus(LibraryInfo::DumpError, qmldumpFailedMessage());
    }

    const QByteArray output = process->readAllStandardOutput();
    QString error;
    QList<const Interpreter::FakeMetaObject *> objectsList = parseHelper(output, &error);
    if (exitCode == 0 && !error.isEmpty()) {
        libraryInfo.setDumpStatus(LibraryInfo::DumpError, tr("Type dump of C++ plugin failed. Parse error:\n'%1'").arg(error));
    }

    if (exitCode == 0 && error.isEmpty()) {
        libraryInfo.setMetaObjects(objectsList);
        if (libraryPath.isEmpty())
            Interpreter::CppQmlTypesLoader::builtinObjects.append(objectsList);
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
    messageManager->printToOutputPane(qmldumpErrorMessage(libraryPath, process->readAllStandardError()));

    if (!libraryPath.isEmpty()) {
        const Snapshot snapshot = m_modelManager->snapshot();
        LibraryInfo libraryInfo = snapshot.libraryInfo(libraryPath);
        libraryInfo.setDumpStatus(LibraryInfo::DumpError, qmldumpFailedMessage());
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
    if (plugin.hasPredumpedXmlFile()) {
        const Snapshot snapshot = m_modelManager->snapshot();
        LibraryInfo libraryInfo = snapshot.libraryInfo(plugin.qmldirPath);
        if (!libraryInfo.isValid())
            return;

        const QString &path = plugin.predumpedXmlFilePath();
        QFile libraryXmlFile(path);
        if (!libraryXmlFile.open(QFile::ReadOnly | QFile::Text)) {
            libraryInfo.setDumpStatus(LibraryInfo::DumpError,
                                      tr("Could not open file '%1' for reading.").arg(path));
            m_modelManager->updateLibraryInfo(plugin.qmldirPath, libraryInfo);
            return;
        }

        const QByteArray xml = libraryXmlFile.readAll();
        libraryXmlFile.close();

        QString error;
        const QList<const Interpreter::FakeMetaObject *> objectsList = parseHelper(xml, &error);

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
    args << plugin.importPath;
    args << plugin.importUri;
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

bool PluginDumper::Plugin::hasPredumpedXmlFile() const
{
    return QFileInfo(predumpedXmlFilePath()).isFile();
}

QString PluginDumper::Plugin::predumpedXmlFilePath() const
{
    return QString("%1%2library.xml").arg(qmldirPath, QDir::separator());
}
