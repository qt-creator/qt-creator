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

#include "externaleditors.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"

#include <utils/synchronousprocess.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <designer/designerconstants.h>

#include <QProcess>
#include <QFileInfo>
#include <QDebug>
#include <QSignalMapper>

#include <QTcpSocket>
#include <QTcpServer>

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

static inline QString msgAppNotFound(const QString &id)
{
    return ExternalQtEditor::tr("The application \"%1\" could not be found.").arg(id);
}

// -- Commands and helpers
#ifdef Q_OS_MAC
static const char linguistBinaryC[] = "Linguist";
static const char designerBinaryC[] = "Designer";

// Mac: Change the call 'Foo.app/Contents/MacOS/Foo <filelist>' to
// 'open -a Foo.app <filelist>'. doesn't support generic command line arguments
static void createMacOpenCommand(QString *binary, QStringList *arguments)
{
    const int appFolderIndex = binary->lastIndexOf(QLatin1String("/Contents/MacOS/"));
    if (appFolderIndex != -1) {
        binary->truncate(appFolderIndex);
        arguments->push_front(*binary);
        arguments->push_front(QLatin1String("-a"));
        *binary = QLatin1String("open");
    }
}
#else
static const char linguistBinaryC[] = "linguist";
static const char designerBinaryC[] = "designer";
#endif

static const char designerIdC[] = "Qt.Designer";
static const char linguistIdC[] = "Qt.Linguist";

static const char designerDisplayName[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Qt Designer");
static const char linguistDisplayName[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Qt Linguist");

// -------------- ExternalQtEditor
ExternalQtEditor::ExternalQtEditor(const QString &id,
                                   const QString &displayName,
                                   const QString &mimetype,
                                   QObject *parent) :
    Core::IExternalEditor(parent),
    m_mimeTypes(mimetype),
    m_id(id),
    m_displayName(displayName)
{
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

bool ExternalQtEditor::getEditorLaunchData(const QString &fileName,
                                           QtVersionCommandAccessor commandAccessor,
                                           const QString &fallbackBinary,
                                           const QStringList &additionalArguments,
                                           bool useMacOpenCommand,
                                           EditorLaunchData *data,
                                           QString *errorMessage) const
{
    // Get the binary either from the current Qt version of the project or Path
    if (const Qt4Project *project = qt4ProjectFor(fileName)) {
        if (const ProjectExplorer::Target *target = project->activeTarget()) {
            if (const QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(target->kit())) {
                data->binary = (qtVersion->*commandAccessor)();
                data->workingDirectory = project->projectDirectory();
            }
        }
    }
    if (data->binary.isEmpty()) {
        data->workingDirectory.clear();
        data->binary = Utils::SynchronousProcess::locateBinary(fallbackBinary);
    }
    if (data->binary.isEmpty()) {
        *errorMessage = msgAppNotFound(id().toString());
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
       ExternalQtEditor(QLatin1String(linguistIdC),
                        QLatin1String(linguistDisplayName),
                        QLatin1String(Qt4ProjectManager::Constants::LINGUIST_MIMETYPE),
                        parent)
{
}

bool LinguistExternalEditor::startEditor(const QString &fileName, QString *errorMessage)
{
    EditorLaunchData data;
    return getEditorLaunchData(fileName, &QtSupport::BaseQtVersion::linguistCommand,
                            QLatin1String(linguistBinaryC),
                            QStringList(), true, &data, errorMessage)
    && startEditorProcess(data, errorMessage);
}

// --------------- MacDesignerExternalEditor, using Mac 'open'
MacDesignerExternalEditor::MacDesignerExternalEditor(QObject *parent) :
       ExternalQtEditor(QLatin1String(designerIdC),
                        QLatin1String(designerDisplayName),
                        QLatin1String(Qt4ProjectManager::Constants::FORM_MIMETYPE),
                        parent)
{
}

bool MacDesignerExternalEditor::startEditor(const QString &fileName, QString *errorMessage)
{
    EditorLaunchData data;
    return getEditorLaunchData(fileName, &QtSupport::BaseQtVersion::designerCommand,
                            QLatin1String(designerBinaryC),
                            QStringList(), true, &data, errorMessage)
        && startEditorProcess(data, errorMessage);
    return false;
}

// --------------- DesignerExternalEditor with Designer Tcp remote control.
DesignerExternalEditor::DesignerExternalEditor(QObject *parent) :
    ExternalQtEditor(QLatin1String(designerIdC),
                     QLatin1String(designerDisplayName),
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
    if (!getEditorLaunchData(fileName, &QtSupport::BaseQtVersion::designerCommand,
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
