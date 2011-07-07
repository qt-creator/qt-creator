/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmlprofilerengine.h"

#include "canvas/qdeclarativecanvas_p.h"
#include "canvas/qdeclarativecontext2d_p.h"
#include "canvas/qdeclarativetiledcanvas_p.h"
#include "codaqmlprofilerrunner.h"
#include "localqmlprofilerrunner.h"
#include "remotelinuxqmlprofilerrunner.h"
#include "qmlprofilerplugin.h"
#include "qmlprofilertool.h"

#include <analyzerbase/analyzermanager.h>
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <coreplugin/helpmanager.h>
#include <qmlprojectmanager/qmlprojectrunconfiguration.h>
#include <projectexplorer/localapplicationruncontrol.h>
#include <projectexplorer/applicationrunconfiguration.h>
#include <qt4projectmanager/qt-s60/s60devicedebugruncontrol.h>
#include <qt4projectmanager/qt-s60/s60devicerunconfiguration.h>

#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>

using namespace Analyzer;
using namespace ProjectExplorer;

namespace QmlProfiler {
namespace Internal {

//
// QmlProfilerEnginePrivate
//

class QmlProfilerEngine::QmlProfilerEnginePrivate
{
public:
    QmlProfilerEnginePrivate(QmlProfilerEngine *qq) : q(qq), m_runner(0) {}
    ~QmlProfilerEnginePrivate() {}

    bool attach(const QString &address, uint port);
    static AbstractQmlProfilerRunner *createRunner(ProjectExplorer::RunConfiguration *runConfiguration,
                                                   QObject *parent);

    QmlProfilerEngine *q;

    //AnalyzerStartParameters m_params;
    AbstractQmlProfilerRunner *m_runner;
    bool m_running;
    bool m_fetchingData;
    bool m_delayedDelete;
};

AbstractQmlProfilerRunner *
QmlProfilerEngine::QmlProfilerEnginePrivate::createRunner(ProjectExplorer::RunConfiguration *runConfiguration,
                                                          QObject *parent)
{
    AbstractQmlProfilerRunner *runner = 0;
    if (QmlProjectManager::QmlProjectRunConfiguration *rc1 =
            qobject_cast<QmlProjectManager::QmlProjectRunConfiguration *>(runConfiguration)) {
        // This is a "plain" .qmlproject.
        LocalQmlProfilerRunner::Configuration conf;
        conf.executable = rc1->observerPath();
        conf.executableArguments = rc1->viewerArguments();
        conf.workingDirectory = rc1->workingDirectory();
        conf.environment = rc1->environment();
        conf.port = rc1->qmlDebugServerPort();
        runner = new LocalQmlProfilerRunner(conf, parent);
    } else if (LocalApplicationRunConfiguration *rc2 =
            qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration)) {
        // FIXME: Check.
        LocalQmlProfilerRunner::Configuration conf;
        conf.executable = rc2->executable();
        conf.executableArguments = rc2->commandLineArguments();
        conf.workingDirectory = rc2->workingDirectory();
        conf.environment = rc2->environment();
        conf.port = rc2->qmlDebugServerPort();
        runner = new LocalQmlProfilerRunner(conf, parent);
    } else if (Qt4ProjectManager::S60DeviceRunConfiguration *s60Config =
            qobject_cast<Qt4ProjectManager::S60DeviceRunConfiguration*>(runConfiguration)) {
        runner = new CodaQmlProfilerRunner(s60Config, parent);
    } else if (RemoteLinux::RemoteLinuxRunConfiguration *rmConfig =
            qobject_cast<RemoteLinux::RemoteLinuxRunConfiguration *>(runConfiguration)) {
        runner = new RemoteLinuxQmlProfilerRunner(rmConfig, parent);
    } else {
        QTC_ASSERT(false, /**/);
    }
    return runner;
}

//
// QmlProfilerEngine
//

QmlProfilerEngine::QmlProfilerEngine(IAnalyzerTool *tool,
         ProjectExplorer::RunConfiguration *runConfiguration)
    : IAnalyzerEngine(tool, runConfiguration)
    , d(new QmlProfilerEnginePrivate(this))
{
    d->m_running = false;
    d->m_fetchingData = false;
    d->m_delayedDelete = false;
}

QmlProfilerEngine::~QmlProfilerEngine()
{
    if (d->m_running)
        stop();
    delete d;
}

void QmlProfilerEngine::start()
{
    QTC_ASSERT(!d->m_runner, return);
    d->m_runner = QmlProfilerEnginePrivate::createRunner(runConfiguration(), this);


    connect(d->m_runner, SIGNAL(stopped()), this, SLOT(stopped()));
    connect(d->m_runner, SIGNAL(appendMessage(QString,Utils::OutputFormat)),
            this, SLOT(logApplicationMessage(QString,Utils::OutputFormat)));

    d->m_runner->start();

    d->m_running = true;
    d->m_delayedDelete = false;

    AnalyzerManager::handleToolStarted();
}

void QmlProfilerEngine::stop()
{
    if (d->m_fetchingData) {
        if (d->m_running)
            d->m_delayedDelete = true;
        // will result in dataReceived() call
        emit stopRecording();
    } else {
        finishProcess();
    }
}

void QmlProfilerEngine::stopped()
{
    d->m_running = false;
    AnalyzerManager::stopTool(); // FIXME: Needed?
    emit finished();
}

void QmlProfilerEngine::setFetchingData(bool b)
{
    d->m_fetchingData = b;
}

void QmlProfilerEngine::dataReceived()
{
    if (d->m_delayedDelete)
        finishProcess();
    d->m_delayedDelete = false;
}

void QmlProfilerEngine::finishProcess()
{
    // user stop?
    if (d->m_running) {
        d->m_running = false;
        d->m_runner->stop();
        emit finished();
    }
}

void QmlProfilerEngine::filterApplicationMessage(const QString &msg)
{
    static const QString qddserver = QLatin1String("QDeclarativeDebugServer: ");
    static const QString cannotRetrieveDebuggingOutput = ProjectExplorer::ApplicationLauncher::msgWinCannotRetrieveDebuggingOutput();

    const int index = msg.indexOf(qddserver);
    if (index != -1) {
        QString status = msg;
        status.remove(0, index + qddserver.length()); // chop of 'QDeclarativeDebugServer: '

        static QString waitingForConnection = QLatin1String("Waiting for connection ");
        static QString unableToListen = QLatin1String("Unable to listen ");
        static QString debuggingNotEnabled = QLatin1String("Ignoring \"-qmljsdebugger=");
        static QString debuggingNotEnabled2 = QLatin1String("Ignoring\"-qmljsdebugger="); // There is (was?) a bug in one of the error strings - safest to handle both
        static QString connectionEstablished = QLatin1String("Connection established");

        QString errorMessage;
        if (status.startsWith(waitingForConnection)) {
            emit processRunning(d->m_runner->debugPort());
        } else if (status.startsWith(unableToListen)) {
            //: Error message shown after 'Could not connect ... debugger:"
            errorMessage = tr("The port seems to be in use.");
        } else if (status.startsWith(debuggingNotEnabled) || status.startsWith(debuggingNotEnabled2)) {
            //: Error message shown after 'Could not connect ... debugger:"
            errorMessage = tr("The application is not set up for QML/JS debugging.");
        } else if (status.startsWith(connectionEstablished)) {
            // nothing to do
        } else {
            qWarning() << "Unknown QDeclarativeDebugServer status message: " << status;
        }

        if (!errorMessage.isEmpty()) {
            Core::ICore * const core = Core::ICore::instance();
            QMessageBox *infoBox = new QMessageBox(core->mainWindow());
            infoBox->setIcon(QMessageBox::Critical);
            infoBox->setWindowTitle(tr("Qt Creator"));
            //: %1 is detailed error message
            infoBox->setText(tr("Could not connect to the in-process QML debugger:\n%1")
                             .arg(errorMessage));
            infoBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Help);
            infoBox->setDefaultButton(QMessageBox::Ok);
            infoBox->setModal(true);

            connect(infoBox, SIGNAL(finished(int)),
                    this, SLOT(wrongSetupMessageBoxFinished(int)));

            infoBox->show();
        }
    } else if (msg.contains(cannotRetrieveDebuggingOutput)) {
        // we won't get debugging output, so just try to connect ...
        emit processRunning(d->m_runner->debugPort());
    }
}

void QmlProfilerEngine::logApplicationMessage(const QString &msg, Utils::OutputFormat format)
{
    emit outputReceived(msg, format);

    filterApplicationMessage(msg);
}

void QmlProfilerEngine::wrongSetupMessageBoxFinished(int button)
{
    if (button == QMessageBox::Help) {
        Core::HelpManager *helpManager = Core::HelpManager::instance();
        helpManager->handleHelpRequest("creator-qml-performance-monitor.html");
    }
}

} // namespace Internal
} // namespace QmlProfiler
