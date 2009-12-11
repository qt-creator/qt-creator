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
#include "runcontrol.h"
#include "qmlinspector.h"
#include "qmlinspectormode.h"
#include "inspectoroutputpane.h"
#include "qmlinspectorplugin.h"

#include <private/qmldebug_p.h>
#include <private/qmldebugclient_p.h>

#include <coreplugin/icore.h>

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QStringList>
#include <QtCore/QtPlugin>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE


QmlInspectorPlugin::QmlInspectorPlugin()
    : m_inspectMode(0),
      m_runControl(0)
{
}

QmlInspectorPlugin::~QmlInspectorPlugin()
{
}

void QmlInspectorPlugin::shutdown()
{
    removeObject(m_inspectMode);
    delete m_inspectMode;
    m_inspectMode = 0;

    removeObject(m_outputPane);
    delete m_outputPane;
    m_outputPane = 0;
}

bool QmlInspectorPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    Core::ICore *core = Core::ICore::instance();
    Core::UniqueIDManager *uidm = core->uniqueIDManager();

    QList<int> context;
    context.append(uidm->uniqueIdentifier(QmlInspector::Constants::C_INSPECTOR));
    context.append(uidm->uniqueIdentifier(Core::Constants::C_EDITORMANAGER));
    context.append(uidm->uniqueIdentifier(Core::Constants::C_NAVIGATION_PANE));

    m_inspectMode = new QmlInspectorMode(this);
    connect(m_inspectMode, SIGNAL(startViewer()), SLOT(startViewer()));
    connect(m_inspectMode, SIGNAL(stopViewer()), SLOT(stopViewer()));
    m_inspectMode->setContext(context);
    addObject(m_inspectMode);

    m_outputPane = new InspectorOutputPane;
    addObject(m_outputPane);

    connect(m_inspectMode, SIGNAL(statusMessage(QString)),
            m_outputPane, SLOT(addInspectorStatus(QString)));

    m_runControlFactory = new QmlInspectorRunControlFactory(this);
    addAutoReleasedObject(m_runControlFactory);
    
    return true;
}

void QmlInspectorPlugin::extensionsInitialized()
{
}

void QmlInspectorPlugin::startViewer()
{
    stopViewer();
    
    ProjectExplorer::Project *project = 0;
    ProjectExplorer::ProjectExplorerPlugin *plugin = ProjectExplorer::ProjectExplorerPlugin::instance();
    if (plugin)
        project = plugin->currentProject();
    if (!project) {
        qDebug() << "No project loaded"; // TODO should this just run the debugger without a viewer?
        return;
    }
        
    ProjectExplorer::RunConfiguration *rc = project->activeRunConfiguration();

    QmlInspector::StartParameters sp;
    sp.port = m_inspectMode->viewerPort();

    m_runControl = m_runControlFactory->create(rc, ProjectExplorer::Constants::RUNMODE, sp);

    if (m_runControl) {
        connect(m_runControl, SIGNAL(started()), m_inspectMode, SLOT(connectToViewer()));
        connect(m_runControl, SIGNAL(finished()), m_inspectMode, SLOT(disconnectFromViewer()));

        connect(m_runControl, SIGNAL(addToOutputWindow(RunControl*,QString)),
                m_outputPane, SLOT(addOutput(RunControl*,QString)));
        connect(m_runControl, SIGNAL(addToOutputWindowInline(RunControl*,QString)),
                m_outputPane, SLOT(addOutputInline(RunControl*,QString)));
        connect(m_runControl, SIGNAL(error(RunControl*,QString)),
                m_outputPane, SLOT(addErrorOutput(RunControl*,QString)));
 
        m_runControl->start();
        m_outputPane->popup(false);
    }
    
}

void QmlInspectorPlugin::stopViewer()
{
    if (m_runControl) {
        m_runControl->stop();
        m_runControl->deleteLater();
        m_runControl = 0;
    }
}


Q_EXPORT_PLUGIN(QmlInspectorPlugin)

QT_END_NAMESPACE

