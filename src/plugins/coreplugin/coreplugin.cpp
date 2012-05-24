/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "coreplugin.h"
#include "actionmanager.h"
#include "designmode.h"
#include "editmode.h"
#include "editormanager.h"
#include "fileiconprovider.h"
#include "helpmanager.h"
#include "mainwindow.h"
#include "mimedatabase.h"
#include "modemanager.h"

#include <extensionsystem/pluginmanager.h>

#include <QtPlugin>
#include <QDebug>

using namespace Core;
using namespace Core::Internal;

CorePlugin::CorePlugin() :
    m_mainWindow(new MainWindow), m_editMode(0)
{
}

CorePlugin::~CorePlugin()
{
    if (m_editMode) {
        removeObject(m_editMode);
        delete m_editMode;
    }

    if (m_designMode) {
        if (m_designMode->designModeIsRequired())
            removeObject(m_designMode);
        delete m_designMode;
    }

    // delete FileIconProvider singleton
    delete FileIconProvider::instance();

    delete m_mainWindow;
}

void CorePlugin::parseArguments(const QStringList &arguments)
{
    for (int i = 0; i < arguments.size(); ++i) {
        if (arguments.at(i) == QLatin1String("-color")) {
            const QString colorcode(arguments.at(i + 1));
            m_mainWindow->setOverrideColor(QColor(colorcode));
            i++; // skip the argument
        }
        if (arguments.at(i) == QLatin1String("-presentationMode"))
            ActionManager::setPresentationModeEnabled(true);
    }
}

bool CorePlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    parseArguments(arguments);
    const bool success = m_mainWindow->init(errorMessage);
    if (success) {
        m_editMode = new EditMode;
        addObject(m_editMode);
        ModeManager::activateMode(m_editMode->id());
        m_designMode = new DesignMode;
    }
    return success;
}

void CorePlugin::extensionsInitialized()
{
    m_mainWindow->mimeDatabase()->syncUserModifiedMimeTypes();
    if (m_designMode->designModeIsRequired())
        addObject(m_designMode);
    m_mainWindow->extensionsInitialized();
}

bool CorePlugin::delayedInitialize()
{
    HelpManager::instance()->setupHelpManager();
    return true;
}

void CorePlugin::remoteCommand(const QStringList & /* options */, const QStringList &args)
{
    m_mainWindow->openFiles(args, ICore::SwitchMode);
    m_mainWindow->activateWindow();
}

void CorePlugin::fileOpenRequest(const QString &f)
{
    remoteCommand(QStringList(), QStringList(f));
}

ExtensionSystem::IPlugin::ShutdownFlag CorePlugin::aboutToShutdown()
{
    m_mainWindow->aboutToShutdown();
    return SynchronousShutdown;
}

Q_EXPORT_PLUGIN(CorePlugin)
