// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "externaleditors.h"

#include <coreplugin/coreplugintr.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitaspect.h>

#include <designer/designerconstants.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QMap>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>

#include <functional>

using namespace ProjectExplorer;
using namespace Utils;

enum { debug = 0 };

namespace QtSupport::Internal {

struct Tr {
    Q_DECLARE_TR_FUNCTIONS(QtC::QmakeProjectManager)
};

// Locate a binary in a directory, applying all kinds of
// extensions the operating system supports.
static QString checkBinary(const QDir &dir, const QString &binary)
{
    // naive UNIX approach
    const QFileInfo info(dir.filePath(binary));
    if (info.isFile() && info.isExecutable())
        return info.absoluteFilePath();

    // Does the OS have some weird extension concept or does the
    // binary have a 3 letter extension?
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
        return {};
    const int dotIndex = binary.lastIndexOf(QLatin1Char('.'));
    if (dotIndex != -1 && dotIndex == binary.size() - 4)
        return {};

    switch (HostOsInfo::hostOs()) {
    case OsTypeLinux:
    case OsTypeOtherUnix:
    case OsTypeOther:
        break;
    case OsTypeWindows: {
            static const char *windowsExtensions[] = {".cmd", ".bat", ".exe", ".com"};
            // Check the Windows extensions using the order
            const int windowsExtensionCount = sizeof(windowsExtensions)/sizeof(const char*);
            for (int e = 0; e < windowsExtensionCount; e ++) {
                const QFileInfo windowsBinary(dir.filePath(binary + QLatin1String(windowsExtensions[e])));
                if (windowsBinary.isFile() && windowsBinary.isExecutable())
                    return windowsBinary.absoluteFilePath();
            }
        }
        break;
    case OsTypeMac: {
            // Check for Mac app folders
            const QFileInfo appFolder(dir.filePath(binary + QLatin1String(".app")));
            if (appFolder.isDir()) {
                QString macBinaryPath = appFolder.absoluteFilePath();
                macBinaryPath += QLatin1String("/Contents/MacOS/");
                macBinaryPath += binary;
                const QFileInfo macBinary(macBinaryPath);
                if (macBinary.isFile() && macBinary.isExecutable())
                    return macBinary.absoluteFilePath();
            }
        }
        break;
    }
    return {};
}

static QString locateBinary(const QString &path, const QString &binary)
{
    // Absolute file?
    const QFileInfo absInfo(binary);
    if (absInfo.isAbsolute())
        return checkBinary(absInfo.dir(), absInfo.fileName());

    // Windows finds binaries  in the current directory
    if (HostOsInfo::isWindowsHost()) {
        const QString currentDirBinary = checkBinary(QDir::current(), binary);
        if (!currentDirBinary.isEmpty())
            return currentDirBinary;
    }

    const QStringList paths = path.split(HostOsInfo::pathListSeparator());
    if (paths.empty())
        return {};
    const QStringList::const_iterator cend = paths.constEnd();
    for (QStringList::const_iterator it = paths.constBegin(); it != cend; ++it) {
        const QDir dir(*it);
        const QString rc = checkBinary(dir, binary);
        if (!rc.isEmpty())
            return rc;
    }
    return {};
}

static QString msgStartFailed(const QString &binary, QStringList arguments)
{
    arguments.push_front(binary);
    return Tr::tr("Unable to start \"%1\"").arg(arguments.join(QLatin1Char(' ')));
}

static QString designerBinary(const QtSupport::QtVersion *qtVersion)
{
    if (qtVersion)
        return qtVersion->designerFilePath().toString();
    return QLatin1String(HostOsInfo::isMacHost() ? "Designer" : "designer");
}

// Data required to launch the editor
struct LaunchData
{
    QString binary;
    QStringList arguments;
    FilePath workingDirectory;
};

// Mac: Change the call 'Foo.app/Contents/MacOS/Foo <filelist>' to
// 'open -a Foo.app <filelist>'. doesn't support generic command line arguments
static LaunchData createMacOpenCommand(const LaunchData &data)
{
    LaunchData openData = data;
    const int appFolderIndex = data.binary.lastIndexOf(QLatin1String("/Contents/MacOS/"));
    if (appFolderIndex != -1) {
        openData.binary = "open";
        openData.arguments = QStringList({QString("-a"), data.binary.left(appFolderIndex)})
                + data.arguments;
    }
    return openData;
}

using CommandForQtVersion = std::function<QString(const QtSupport::QtVersion *)>;

static QString findFirstCommand(const QVector<QtSupport::QtVersion *> &qtVersions,
                                CommandForQtVersion command)
{
    for (QtSupport::QtVersion *qt : qtVersions) {
        if (qt) {
            const QString binary = command(qt);
            if (!binary.isEmpty())
                return binary;
        }
    }
    return QString();
}

static bool getEditorLaunchData(const CommandForQtVersion &commandForQtVersion,
                                const FilePath &filePath,
                                LaunchData *data,
                                QString *errorMessage)
{
    // Check in order for Qt version with the binary:
    // - active kit of project
    // - any other of the project
    // - default kit
    // - any other kit
    // As fallback check PATH
    if (!KitManager::waitForLoaded()) {
        *errorMessage = Tr::tr("Could not load kits in a reasonable amount of time.");
        return false;
    }
    data->workingDirectory.clear();
    QVector<QtSupport::QtVersion *> qtVersionsToCheck; // deduplicated after being filled
    if (const Project *project = ProjectManager::projectForFile(filePath)) {
        data->workingDirectory = project->projectDirectory();
        // active kit
        if (const Target *target = project->activeTarget()) {
            qtVersionsToCheck << QtSupport::QtKitAspect::qtVersion(target->kit());
        }
        // all kits of project
        qtVersionsToCheck += Utils::transform<QVector>(project->targets(), [](Target *t) {
            return QTC_GUARD(t) ? QtSupport::QtKitAspect::qtVersion(t->kit()) : nullptr;
        });
    }
    // default kit
    qtVersionsToCheck << QtSupport::QtKitAspect::qtVersion(KitManager::defaultKit());
    // all kits
    qtVersionsToCheck += Utils::transform<QVector>(KitManager::kits(), QtSupport::QtKitAspect::qtVersion);
    qtVersionsToCheck = Utils::filteredUnique(qtVersionsToCheck); // can still contain nullptr
    data->binary = findFirstCommand(qtVersionsToCheck, commandForQtVersion);
    // fallback
    if (data->binary.isEmpty()) {
        const QString path = qtcEnvironmentVariable("PATH");
        data->binary = locateBinary(path, commandForQtVersion(nullptr));
    }

    if (data->binary.isEmpty()) {
        *errorMessage = Tr::tr("The application \"%1\" could not be found.")
            .arg(filePath.toUserOutput());
        return false;
    }

    // Setup binary + arguments, use Mac Open if appropriate
    data->arguments.push_back(filePath.toString());
    if (HostOsInfo::isMacHost())
        *data = createMacOpenCommand(*data);
    if (debug)
        qDebug() << Q_FUNC_INFO << '\n' << data->binary << data->arguments;
    return true;
}

static bool startEditorProcess(const LaunchData &data, QString *errorMessage)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << '\n' << data.binary << data.arguments << data.workingDirectory;
    qint64 pid = 0;
    if (!Process::startDetached({FilePath::fromString(data.binary), data.arguments}, data.workingDirectory, &pid)) {
        *errorMessage = msgStartFailed(data.binary, data.arguments);
        return false;
    }
    return true;
}

// DesignerExternalEditor with Designer Tcp remote control.

// A per-binary entry containing the socket
using ProcessCache = QMap<QString, QTcpSocket*>;

static ProcessCache m_processCache;

static void processTerminated(const QString &binary)
{
    const ProcessCache::iterator it = m_processCache.find(binary);
    if (it == m_processCache.end())
        return;
    // Make sure socket is closed and cleaned, remove from cache
    QTcpSocket *socket = it.value();
    m_processCache.erase(it); // Note that closing will cause the slot to be retriggered
    if (debug)
        qDebug() << Q_FUNC_INFO << '\n' << binary << socket->state();
    if (socket->state() == QAbstractSocket::ConnectedState)
        socket->close();
    socket->deleteLater();
}

DesignerExternalEditor::DesignerExternalEditor()
{
    setId("Qt.Designer");
    setDisplayName(::Core::Tr::tr("Qt Designer"));
    setMimeTypes({ProjectExplorer::Constants::FORM_MIMETYPE});

    setEditorStarter([this](const FilePath &filePath, QString *errorMessage) {
        LaunchData data;

        // Find the editor binary
        if (!getEditorLaunchData(designerBinary, filePath, &data, errorMessage))
            return false;

        if (HostOsInfo::isMacHost())
            return startEditorProcess(data, errorMessage);

        /* Qt Designer on the remaining platforms: Uses Designer's own
            * Tcp-based communication mechanism to ensure all files are opened
            * in one instance (per version). */

        // Known one?
        const ProcessCache::iterator it = m_processCache.find(data.binary);
        if (it != m_processCache.end()) {
            // Process is known, write to its socket to cause it to open the file
            if (debug)
                qDebug() << Q_FUNC_INFO << "\nWriting to socket:" << data.binary << filePath;
            QTcpSocket *socket = it.value();
            if (!socket->write(filePath.toString().toUtf8() + '\n')) {
                *errorMessage = Tr::tr("Qt Designer is not responding (%1).").arg(socket->errorString());
                return false;
            }
            return true;
        }
        // No process yet. Create socket & launch the process
        QTcpServer server;
        if (!server.listen(QHostAddress::LocalHost)) {
            *errorMessage = Tr::tr("Unable to create server socket: %1").arg(server.errorString());
            return false;
        }
        const quint16 port = server.serverPort();
        if (debug)
            qDebug() << Q_FUNC_INFO << "\nLaunching server:" << port << data.binary << filePath;
        // Start first one with file and socket as '-client port file'
        // Wait for the socket listening
        data.arguments.push_front(QString::number(port));
        data.arguments.push_front(QLatin1String("-client"));

        if (!startEditorProcess(data, errorMessage))
            return false;
        // Insert into cache if socket is created, else try again next time
        if (server.waitForNewConnection(3000)) {
            QTcpSocket *socket = server.nextPendingConnection();
            socket->setParent(&m_guard);
            const QString binary = data.binary;
            m_processCache.insert(binary, socket);
            auto mapSlot = [binary] { processTerminated(binary); };
            QObject::connect(socket, &QAbstractSocket::disconnected, &m_guard, mapSlot);
            QObject::connect(socket, &QAbstractSocket::errorOccurred, &m_guard, mapSlot);
        }
        return true;
    });
}

// Linguist

static QString linguistBinary(const QtSupport::QtVersion *qtVersion)
{
    if (qtVersion)
        return qtVersion->linguistFilePath().toString();
    return QLatin1String(HostOsInfo::isMacHost() ? "Linguist" : "linguist");
}

LinguistEditor::LinguistEditor()
{
    setId("Qt.Linguist");
    setDisplayName(::Core::Tr::tr("Qt Linguist"));
    setMimeTypes({ProjectExplorer::Constants::LINGUIST_MIMETYPE});
    setEditorStarter([](const FilePath &filePath, QString *errorMessage) {
        LaunchData data;
        return getEditorLaunchData(linguistBinary, filePath, &data, errorMessage)
               && startEditorProcess(data, errorMessage);
    });
}

} // QtSupport::Internal
