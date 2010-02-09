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

#include "exception.h"
#include "qmldesignerplugin.h"
#include "designmode.h"
#include "qmldesignerconstants.h"
#include "pluginmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/dialogs/iwizard.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <utils/qtcassert.h>

#include <integrationcore.h>

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QtCore/qplugin.h>

namespace QmlDesigner {
namespace Internal {

BauhausPlugin *BauhausPlugin::m_pluginInstance = 0;

BauhausPlugin::BauhausPlugin() :
    m_designerCore(0)
{
    // Exceptions should never ever assert: they are handled in a number of
    // places where it is actually VALID AND EXPECTED BEHAVIOUR to get an
    // exception.
    // If you still want to see exactly where the exception originally
    // occurred, then you have various ways to do this:
    //  1. set a breakpoint on the constructor of the exception
    //  2. in gdb: "catch throw" or "catch throw Exception"
    //  3. set a breakpoint on __raise_exception()
    // And with gdb, you can even do this from your ~/.gdbinit file.
    Exception::setShouldAssert(false);
}

BauhausPlugin::~BauhausPlugin()
{
    delete m_designerCore;
}

////////////////////////////////////////////////////
//
// INHERITED FROM ExtensionSystem::Plugin
//
////////////////////////////////////////////////////
bool BauhausPlugin::initialize(const QStringList & /*arguments*/, QString *error_message/* = 0*/) // =0;
{
    Core::ICore *core = Core::ICore::instance();

    const int uid = core->uniqueIDManager()->uniqueIdentifier(QLatin1String(QmlDesigner::Constants::C_FORMEDITOR));
    const QList<int> context = QList<int>() << uid;

    m_designerCore = new QmlDesigner::IntegrationCore;

    m_pluginInstance = this;

#ifdef Q_OS_MAC
    const QString pluginPath = QCoreApplication::applicationDirPath() + "/../PlugIns/QmlDesigner";
#else
    const QString pluginPath = QCoreApplication::applicationDirPath() + "/../"
                               + QLatin1String(IDE_LIBRARY_BASENAME) + "/qmldesigner";
#endif

    m_designerCore->pluginManager()->setPluginPaths(QStringList() << pluginPath);

    addAutoReleasedObject(new DesignMode);

    error_message->clear();

    return true;
}

void BauhausPlugin::extensionsInitialized()
{
}

BauhausPlugin *BauhausPlugin::pluginInstance()
{
    return m_pluginInstance;
}

DesignerSettings BauhausPlugin::settings() const
{
    return m_settings;
}

void BauhausPlugin::setSettings(const DesignerSettings &s)
{
    if (s != m_settings) {
        m_settings = s;
        if (QSettings *settings = Core::ICore::instance()->settings())
            m_settings.toSettings(settings);
    }
}

}
}

Q_EXPORT_PLUGIN(QmlDesigner::Internal::BauhausPlugin)
