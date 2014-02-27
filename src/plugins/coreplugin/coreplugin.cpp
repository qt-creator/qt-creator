/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "coreplugin.h"
#include "designmode.h"
#include "editmode.h"
#include "helpmanager.h"
#include "mainwindow.h"
#include "mimedatabase.h"
#include "modemanager.h"
#include "infobar.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/locator/locator.h>

#include <utils/savefile.h>

#include <QtPlugin>
#include <QDebug>
#include <QDateTime>

using namespace Core;
using namespace Core::Internal;

CorePlugin::CorePlugin() : m_editMode(0), m_designMode(0)
{
    qRegisterMetaType<Core::Id>();
    m_mainWindow = new MainWindow;
    m_findPlugin = new FindPlugin;
    m_locator = new Locator;
}

CorePlugin::~CorePlugin()
{
    delete m_findPlugin;
    delete m_locator;

    if (m_editMode) {
        removeObject(m_editMode);
        delete m_editMode;
    }

    if (m_designMode) {
        if (m_designMode->designModeIsRequired())
            removeObject(m_designMode);
        delete m_designMode;
    }

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
    qsrand(QDateTime::currentDateTime().toTime_t());
    parseArguments(arguments);
    const bool success = m_mainWindow->init(errorMessage);
    if (success) {
        m_editMode = new EditMode;
        addObject(m_editMode);
        ModeManager::activateMode(m_editMode->id());
        m_designMode = new DesignMode;
        InfoBar::initializeGloballySuppressed();
    }

    // Make sure we respect the process's umask when creating new files
    Utils::SaveFile::initializeUmask();

    m_findPlugin->initialize(arguments, errorMessage);
    m_locator->initialize(this, arguments, errorMessage);

    return success;
}

void CorePlugin::extensionsInitialized()
{
    MimeDatabase::syncUserModifiedMimeTypes();
    if (m_designMode->designModeIsRequired())
        addObject(m_designMode);
    m_findPlugin->extensionsInitialized();
    m_locator->extensionsInitialized();
    m_mainWindow->extensionsInitialized();
}

bool CorePlugin::delayedInitialize()
{
    HelpManager::setupHelpManager();
    m_locator->delayedInitialize();
    return true;
}

QObject *CorePlugin::remoteCommand(const QStringList & /* options */, const QStringList &args)
{
    IDocument *res = m_mainWindow->openFiles(
                args, ICore::OpenFilesFlags(ICore::SwitchMode | ICore::CanContainLineNumbers));
    m_mainWindow->raiseWindow();
    return res;
}

void CorePlugin::fileOpenRequest(const QString &f)
{
    remoteCommand(QStringList(), QStringList(f));
}

ExtensionSystem::IPlugin::ShutdownFlag CorePlugin::aboutToShutdown()
{
    m_findPlugin->aboutToShutdown();
    m_mainWindow->aboutToShutdown();
    return SynchronousShutdown;
}

Q_EXPORT_PLUGIN(CorePlugin)
