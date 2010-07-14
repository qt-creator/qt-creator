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
#include "qmljsdebuggerclient.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

using namespace QmlJSInspector::Internal;

DebuggerClient::DebuggerClient(QDeclarativeDebugConnection* client)
    : QDeclarativeDebugClient(QLatin1String("JSDebugger"), client)
    , connection(client)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    const QList<Debugger::DebuggerRunControlFactory *> factories = pm->getObjects<Debugger::DebuggerRunControlFactory>();
    ProjectExplorer::RunControl *runControl = 0;

    Debugger::DebuggerStartParameters sp;
    sp.startMode = Debugger::StartExternal;
    sp.executable = "qmlviewer"; //FIXME
    runControl = factories.first()->create(sp);
    Debugger::DebuggerRunControl* debuggerRunControl = qobject_cast<Debugger::DebuggerRunControl *>(runControl);

    QTC_ASSERT(debuggerRunControl, return );
    engine = qobject_cast<Debugger::Internal::QmlEngine *>(debuggerRunControl->engine());
    QTC_ASSERT(engine, return );

    engine->Debugger::Internal::DebuggerEngine::startDebugger(debuggerRunControl);
    //engine->startSuccessful();  // FIXME: AAA: port to new debugger states

    connect(engine, SIGNAL(sendMessage(QByteArray)), this, SLOT(slotSendMessage(QByteArray)));
    setEnabled(true);
}

DebuggerClient::~DebuggerClient()
{
}

void DebuggerClient::messageReceived(const QByteArray &data)
{
    engine->messageReceived(data);
}

void DebuggerClient::slotSendMessage(const QByteArray &message)
{
    QDeclarativeDebugClient::sendMessage(message);
}
