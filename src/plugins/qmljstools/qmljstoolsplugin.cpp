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

#include "qmljstoolsplugin.h"
#include "qmljsmodelmanager.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>

#include <QtCore/QtPlugin>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QSettings>

using namespace QmlJSTools::Internal;

enum { debug = 0 };

QmlJSToolsPlugin *QmlJSToolsPlugin::m_instance = 0;

QmlJSToolsPlugin::QmlJSToolsPlugin()
    : m_modelManager(0)
{
    m_instance = this;
}

QmlJSToolsPlugin::~QmlJSToolsPlugin()
{
    m_instance = 0;
    m_modelManager = 0; // deleted automatically
}

bool QmlJSToolsPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error)
//    Core::ICore *core = Core::ICore::instance();

    // Objects
    m_modelManager = new ModelManager(this);
//    Core::VCSManager *vcsManager = core->vcsManager();
//    Core::FileManager *fileManager = core->fileManager();
//    connect(vcsManager, SIGNAL(repositoryChanged(QString)),
//            m_modelManager, SLOT(updateModifiedSourceFiles()));
//    connect(fileManager, SIGNAL(filesChangedInternally(QStringList)),
//            m_modelManager, SLOT(updateSourceFiles(QStringList)));
    addAutoReleasedObject(m_modelManager);

    return true;
}

void QmlJSToolsPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag QmlJSToolsPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

Q_EXPORT_PLUGIN(QmlJSToolsPlugin)
