/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "externaleditors.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qtversionmanager.h"
#include "qt4buildconfiguration.h"

#include <utils/synchronousprocess.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <designer/designerconstants.h>

#include <QtCore/QProcess>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QSignalMapper>

#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QTcpServer>

enum { debug = 0 };

namespace Qt4ProjectManager {
namespace Internal {

// Figure out the Qt4 project used by the file if any
static Qt4Project *qt4ProjectFor(const QString &fileName)
{
    ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();
    if (ProjectExplorer::Project *baseProject = pe->session()->projectForFile(fileName))
        if (Qt4Project *project = qobject_cast<Qt4Project*>(baseProject))
            return project;
    return 0;
}

// ------------ Messages
static inline QString msgStartFailed(const QString &binary, QStringList arguments)
{
    arguments.push_front(binary);
    return ExternalQtEditor::tr("Unable to start \"%1\"").arg(arguments.join(QString(QLatin1Char(' '))));
}

static inline QString msgAppNotFound(const QString &kind)
{
    return ExternalQtEditor::tr("The application \"%1\" could not be found.").arg(kind);
}

// -- Commands and helpers
#ifdef Q_OS_MAC
static const char *linguistBinaryC = "Linguist";
static const char *designerBinaryC = "Designer";

// Mac: Change the call 'Foo.app/Contents/MacOS/Foo <file>' to
// 'open Foo.app <file>'. Do this ONLY if you do not want to have
// command line arguments
static void createMacOpenCommand(QString *binary, QStringList *arguments)
{
    const int appFolderIndex = binary->lastIndexOf(QLatin1String("/Contents/MacOS/"));
    if (appFolderIndex != -1) {
        binary->truncate(appFolderIndex);
        arguments->push_front(*binary);
        *binary = QLatin1String("open");
    }
}

#else
static const char *linguistBinaryC = "linguist";
static const char *designerBinaryC = "designer";
#endif

static const char *designerKindC = "Qt Designer";

// -------------- ExternalQtEditor
ExternalQtEditor::ExternalQtEditor(const QString &kind,
                              const QString &mimetype,
                              QObject *parent) :
    Core::IExternalEditor(parent),
    m_mimeTypes(mimetype),
    m_kind(kind)
{
}

QStringList ExternalQtEditor::mimeTypes() const
{
    return m_mimeTypes;
}

QString ExternalQtEditor::kind() const
{
    return m_kind;
}

bool ExternalQtEditor::getEditorLaunchData(const QString &fileName,
                                           QtVersionCommandAccessor commandAccessor,
                                           const QString &fallbackBinary,
                                           const QStringList &additionalArguments,
                                           bool useMacOpenCommand,
                                           EditorLaunchData *data,
                                           QString *errorMessage) const
{
    const Qt4Project *project = qt4ProjectFor(fileName);
    // Get the binary either from the current Qt version of the project or Path
    if (project) {
        Qt4BuildConfiguration *qt4bc = project->activeQt4BuildConfiguration();
        const QtVersion *qtVersion= qt4bc->qtVersion();
        data->binary = (qtVersion->*commandAccessor)();
        data->workingDirectory = QFileInfo(project->file()->fileName()).absolutePath();
    } else {
        data->workingDirectory.clear();
        data->binary = Utils::SynchronousProcess::locateBinary(fallbackBinary);
    }
    if (data->binary.isEmpty()) {
        *errorMessage = msgAppNotFound(kind());
        return false;
    }
    // Setup binary + arguments, use Mac Open if appropriate
    data->arguments = additionalArguments;
    data->arguments.push_back(fileName);
#ifdef Q_OS_MAC
    if (useMacOpenCommand)
        createMacOpenCommand(&(data->binary), &(data->arguments));
#else
    Q_UNUSED(useMacOpenCommand)
#endif
    if (debug)
        qDebug() << Q_FUNC_INFO << '\n' << data->binary << data->arguments;
    return true;
}

bool ExternalQtEditor::startEditorProcess(const EditorLaunchData &data, QString *errorMessage)
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

// --------------- LinguistExternalEditor
LinguistExternalEditor::LinguistExternalEditor(QObject *parent) :
       ExternalQtEditor(QLatin1String("Qt Linguist"),
                        QLatin1String(Qt4ProjectManager::Constants::LINGUIST_MIMETYPE),
                        parent)
{
}

bool LinguistExternalEditor::startEditor(const QString &fileName, QString *errorMessage)
{
    EditorLaunchData data;
    return getEditorLaunchData(fileName, &QtVersion::linguistCommand,
                            QLatin1String(linguistBinaryC),
                            QStringList(), true, &data, errorMessage)
    && startEditorProcess(data, errorMessage);
}

// --------------- MacDesignerExternalEditor, using Mac 'open'
MacDesignerExternalEditor::MacDesignerExternalEditor(QObject *parent) :
       ExternalQtEditor(QLatin1String(designerKindC),
                        QLatin1String(Qt4ProjectManager::Constants::FORM_MIMETYPE),
                        parent)
{
}

bool MacDesignerExternalEditor::startEditor(const QString &fileName, QString *errorMessage)
{
    EditorLaunchData data;
    return getEditorLaunchData(fileName, &QtVersion::designerCommand,
                            QLatin1String(designerBinaryC),
                            QStringList(), true, &data, errorMessage)
        && startEditorProcess(data, errorMessage);
    return false;
}

// --------------- DesignerExternalEditor with Designer Tcp remote control.
DesignerExternalEditor::DesignerExternalEditor(QObject *parent) :
    ExternalQtEditor(QLatin1String(designerKindC),
                     QLatin1String(Designer::Constants::FORM_MIMETYPE),
                     parent),
    m_terminationMapper(0)
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
    EditorLaunchData data;
    // Find the editor binary
    if (!getEditorLaunchData(fileName, &QtVersion::designerCommand,
                            QLatin1String(designerBinaryC),
                            QStringList(), false, &data, errorMessage)) {
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
    // Set up signal mapper to track process termination via socket
    if (!m_terminationMapper) {
        m_terminationMapper = new QSignalMapper(this);
        connect(m_terminationMapper, SIGNAL(mapped(QString)), this, SLOT(processTerminated(QString)));
    }
    // Insert into cache if socket is created, else try again next time
    if (server.waitForNewConnection(3000)) {
        QTcpSocket *socket = server.nextPendingConnection();
        socket->setParent(this);
        m_processCache.insert(data.binary, socket);
        m_terminationMapper->setMapping(socket, data.binary);
        connect(socket, SIGNAL(disconnected()), m_terminationMapper, SLOT(map()));
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), m_terminationMapper, SLOT(map()));
    }
    return true;
}

} // namespace Internal
} // namespace Qt4ProjectManager
