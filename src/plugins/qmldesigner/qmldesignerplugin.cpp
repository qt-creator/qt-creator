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

BauhausPlugin::BauhausPlugin() :
    m_designerCore(0)
{
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

}
}

Q_EXPORT_PLUGIN(QmlDesigner::Internal::BauhausPlugin)
