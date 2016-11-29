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

#include "externaleditors.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/synchronousprocess.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <qtsupport/qtkitinformation.h>
#include <designer/designerconstants.h>

#include <QProcess>
#include <QDebug>

#include <QTcpSocket>
#include <QTcpServer>

using namespace ProjectExplorer;

enum { debug = 0 };

namespace QmakeProjectManager {
namespace Internal {

// ------------ Messages
static inline QString msgStartFailed(const QString &binary, QStringList arguments)
{
    arguments.push_front(binary);
    return ExternalQtEditor::tr("Unable to start \"%1\"").arg(arguments.join(QLatin1Char(' ')));
}

static inline QString msgAppNotFound(const QString &id)
{
    return ExternalQtEditor::tr("The application \"%1\" could not be found.").arg(id);
}

// -- Commands and helpers
static QString linguistBinary(const QtSupport::BaseQtVersion *qtVersion)
{
    if (qtVersion)
        return qtVersion->linguistCommand();
    return QLatin1String(Utils::HostOsInfo::isMacHost() ? "Linguist" : "linguist");
}

static QString designerBinary(const QtSupport::BaseQtVersion *qtVersion)
{
    if (qtVersion)
        return qtVersion->designerCommand();
    return QLatin1String(Utils::HostOsInfo::isMacHost() ? "Designer" : "designer");
}

// Mac: Change the call 'Foo.app/Contents/MacOS/Foo <filelist>' to
// 'open -a Foo.app <filelist>'. doesn't support generic command line arguments
static ExternalQtEditor::LaunchData createMacOpenCommand(const ExternalQtEditor::LaunchData &data)
{
    ExternalQtEditor::LaunchData openData = data;
    const int appFolderIndex = data.binary.lastIndexOf(QLatin1String("/Contents/MacOS/"));
    if (appFolderIndex != -1) {
        openData.binary = "open";
        openData.arguments = QStringList({ QString("-a"), data.binary.left(appFolderIndex) })
                + data.arguments;
    }
    return openData;
}

static const char designerIdC[] = "Qt.Designer";
static const char linguistIdC[] = "Qt.Linguist";

static const char designerDisplayName[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Qt Designer");
static const char linguistDisplayName[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Qt Linguist");

// -------------- ExternalQtEditor
ExternalQtEditor::ExternalQtEditor(Core::Id id,
                                   const QString &displayName,
                                   const QString &mimetype,
                                   const CommandForQtVersion &commandForQtVersion) :
    m_mimeTypes(mimetype),
    m_id(id),
    m_displayName(displayName),
    m_commandForQtVersion(commandForQtVersion)
{
}

ExternalQtEditor *ExternalQtEditor::createLinguistEditor()
{
    return new ExternalQtEditor(linguistIdC,
                                QLatin1String(linguistDisplayName),
                                QLatin1String(ProjectExplorer::Constants::LINGUIST_MIMETYPE),
                                linguistBinary);
}

ExternalQtEditor *ExternalQtEditor::createDesignerEditor()
{
    if (Utils::HostOsInfo::isMacHost()) {
        return new ExternalQtEditor(designerIdC,
                                    QLatin1String(designerDisplayName),
                                    QLatin1String(ProjectExplorer::Constants::FORM_MIMETYPE),
                                    designerBinary);
    } else {
        return new DesignerExternalEditor;
    }
}

QStringList ExternalQtEditor::mimeTypes() const
{
    return m_mimeTypes;
}

Core::Id ExternalQtEditor::id() const
{
    return m_id;
}

QString ExternalQtEditor::displayName() const
{
    return m_displayName;
}

static QString findFirstCommand(QVector<QtSupport::BaseQtVersion *> qtVersions,
                                ExternalQtEditor::CommandForQtVersion command)
{
    foreach (QtSupport::BaseQtVersion *qt, qtVersions) {
        if (qt) {
            const QString binary = command(qt);
            if (!binary.isEmpty())
                return binary;
        }
    }
    return QString();
}

bool ExternalQtEditor::getEditorLaunchData(const QString &fileName,
                                           LaunchData *data,
                                           QString *errorMessage) const
{
    // Check in order for Qt version with the binary:
    // - active kit of project
    // - any other of the project
    // - default kit
    // - any other kit
    // As fallback check PATH
    data->workingDirectory.clear();
    QVector<QtSupport::BaseQtVersion *> qtVersionsToCheck; // deduplicated after being filled
    if (const Project *project = SessionManager::projectForFile(Utils::FileName::fromString(fileName))) {
        data->workingDirectory = project->projectDirectory().toString();
        // active kit
        if (const Target *target = project->activeTarget()) {
            qtVersionsToCheck << QtSupport::QtKitInformation::qtVersion(target->kit());
        }
        // all kits of project
        qtVersionsToCheck += Utils::transform<QVector>(project->targets(), [](Target *t) {
            return QTC_GUARD(t) ? QtSupport::QtKitInformation::qtVersion(t->kit()) : nullptr;
        });
    }
    // default kit
    qtVersionsToCheck << QtSupport::QtKitInformation::qtVersion(KitManager::defaultKit());
    // all kits
    qtVersionsToCheck += Utils::transform<QVector>(KitManager::kits(), QtSupport::QtKitInformation::qtVersion);
    qtVersionsToCheck = Utils::filteredUnique(qtVersionsToCheck); // can still contain nullptr
    data->binary = findFirstCommand(qtVersionsToCheck, m_commandForQtVersion);
    // fallback
    if (data->binary.isEmpty())
        data->binary = Utils::SynchronousProcess::locateBinary(m_commandForQtVersion(nullptr));
    if (data->binary.isEmpty()) {
        *errorMessage = msgAppNotFound(id().toString());
        return false;
    }
    // Setup binary + arguments, use Mac Open if appropriate
    data->arguments.push_back(fileName);
    if (Utils::HostOsInfo::isMacHost())
        *data = createMacOpenCommand(*data);
    if (debug)
        qDebug() << Q_FUNC_INFO << '\n' << data->binary << data->arguments;
    return true;
}

bool ExternalQtEditor::startEditor(const QString &fileName, QString *errorMessage)
{
    LaunchData data;
    return getEditorLaunchData(fileName, &data, errorMessage)
            && startEditorProcess(data, errorMessage);
}

bool ExternalQtEditor::startEditorProcess(const LaunchData &data, QString *errorMessage)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << '\n' << data.binary << data.arguments << data.workingDirectory;
    qint64 pid = 0;
    if (!QProcess::startDetached(data.binary, data.arguments, data.workingDirectory, &pid)) {
        *errorMessage = msgStartFailed(data.binary, data.arguments);
        return false;
    }
    return true;
}

// --------------- DesignerExternalEditor with Designer Tcp remote control.
DesignerExternalEditor::DesignerExternalEditor() :
    ExternalQtEditor(designerIdC,
                     QLatin1String(designerDisplayName),
                     QLatin1String(Designer::Constants::FORM_MIMETYPE),
                     designerBinary)
{
}

void DesignerExternalEditor::processTerminated(const QString &binary)
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

bool DesignerExternalEditor::startEditor(const QString &fileName, QString *errorMessage)
{
    LaunchData data;
    // Find the editor binary
    if (!getEditorLaunchData(fileName, &data, errorMessage)) {
        return false;
    }
    // Known one?
    const ProcessCache::iterator it = m_processCache.find(data.binary);
    if (it != m_processCache.end()) {
        // Process is known, write to its socket to cause it to open the file
        if (debug)
           qDebug() << Q_FUNC_INFO << "\nWriting to socket:" << data.binary << fileName;
        QTcpSocket *socket = it.value();
        if (!socket->write(fileName.toUtf8() + '\n')) {
            *errorMessage = tr("Qt Designer is not responding (%1).").arg(socket->errorString());
            return false;
        }
        return true;
    }
    // No process yet. Create socket & launch the process
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)) {
        *errorMessage = tr("Unable to create server socket: %1").arg(server.errorString());
        return false;
    }
    const quint16 port = server.serverPort();
    if (debug)
        qDebug() << Q_FUNC_INFO << "\nLaunching server:" << port << data.binary << fileName;
    // Start first one with file and socket as '-client port file'
    // Wait for the socket listening
    data.arguments.push_front(QString::number(port));
    data.arguments.push_front(QLatin1String("-client"));

    if (!startEditorProcess(data, errorMessage))
        return false;
    // Insert into cache if socket is created, else try again next time
    if (server.waitForNewConnection(3000)) {
        QTcpSocket *socket = server.nextPendingConnection();
        socket->setParent(this);
        const QString binary = data.binary;
        m_processCache.insert(binary, socket);
        auto mapSlot = [this, binary] { processTerminated(binary); };
        connect(socket, &QAbstractSocket::disconnected, this, mapSlot);
        connect(socket,
                static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
                this, mapSlot);
    }
    return true;
}

} // namespace Internal
} // namespace QmakeProjectManager
