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

#include "qmljsinspectorconstants.h"
#include "qmljsinspectorplugin.h"
#include "qmljsinspector.h"
#include "qmljsclientproxy.h"
#include "qmlinspectortoolbar.h"

#include <debugger/debuggeruiswitcher.h>
#include <debugger/debuggerconstants.h>
#include <debugger/qml/qmladapter.h>
#include <debugger/qml/qmlengine.h>

#include <qmlprojectmanager/qmlproject.h>
#include <qmljseditor/qmljseditorconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/coreconstants.h>

#include <QtCore/QStringList>
#include <QtCore/QtPlugin>
#include <QtCore/QTimer>

#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>

#include <QtCore/QDebug>

using namespace QmlJSInspector::Internal;
using namespace QmlJSInspector::Constants;

namespace {

InspectorPlugin *g_instance = 0; // the global QML/JS inspector instance

} // end of anonymous namespace

InspectorPlugin::InspectorPlugin()
    : IPlugin()
    , m_clientProxy(0)
{
    Q_ASSERT(! g_instance);
    g_instance = this;

    m_inspectorUi = new InspectorUi(this);
}

InspectorPlugin::~InspectorPlugin()
{
}

QmlJS::ModelManagerInterface *InspectorPlugin::modelManager() const
{
    return ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();
}

InspectorPlugin *InspectorPlugin::instance()
{
    return g_instance;
}

InspectorUi *InspectorPlugin::inspector() const
{
    return m_inspectorUi;
}

ExtensionSystem::IPlugin::ShutdownFlag InspectorPlugin::aboutToShutdown()
{
    m_inspectorUi->saveSettings();
    return SynchronousShutdown;
}

bool InspectorPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    Debugger::DebuggerUISwitcher *uiSwitcher = pluginManager->getObject<Debugger::DebuggerUISwitcher>();

    uiSwitcher->addLanguage(Debugger::QmlLanguage, tr("QML"), Core::Context(C_INSPECTOR));

    return true;
}

void InspectorPlugin::extensionsInitialized()
{
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();

    connect(pluginManager, SIGNAL(objectAdded(QObject*)), SLOT(objectAdded(QObject*)));
    connect(pluginManager, SIGNAL(aboutToRemoveObject(QObject*)), SLOT(aboutToRemoveObject(QObject*)));

    m_inspectorUi->setupUi();
}

// The adapter object is only added to the pool with a succesful connection,
// so we can immediately init our stuff.
void InspectorPlugin::objectAdded(QObject *object)
{
    Debugger::QmlAdapter *adapter = qobject_cast<Debugger::QmlAdapter *>(object);
    if (adapter) {
        m_clientProxy = new ClientProxy(adapter);
        m_inspectorUi->connected(m_clientProxy);
        return;
    }

    Debugger::QmlEngine *engine = qobject_cast<Debugger::QmlEngine*>(object);
    if (engine) {
        m_inspectorUi->setDebuggerEngine(engine);
    }
}

void InspectorPlugin::aboutToRemoveObject(QObject *obj)
{
    if (m_clientProxy && m_clientProxy->qmlAdapter() == obj) {
        m_inspectorUi->disconnected();
        delete m_clientProxy;
        m_clientProxy = 0;
    }

    if (m_inspectorUi->debuggerEngine() == obj) {
        m_inspectorUi->setDebuggerEngine(0);
    }
}

Q_EXPORT_PLUGIN(InspectorPlugin)
