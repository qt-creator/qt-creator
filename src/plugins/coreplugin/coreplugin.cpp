/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

#include "coreplugin.h"
#include "welcomemode.h"
#include "editmode.h"
#include "editormanager.h"
#include "mainwindow.h"
#include "modemanager.h"
#include "viewmanager.h"
#include "fileiconprovider.h"

#include <QtCore/qplugin.h>
#ifdef QT_WEBKIT
#include <QtGui/QApplication>
#include <QtWebKit/QWebSettings>
#endif

using namespace Core::Internal;

CorePlugin::CorePlugin() :
    m_mainWindow(new MainWindow), m_welcomeMode(0), m_editMode(0), m_pm(0)
{
}

CorePlugin::~CorePlugin()
{
    if (m_welcomeMode) {
        removeObject(m_welcomeMode);
        delete m_welcomeMode;
    }
    if (m_editMode) {
        removeObject(m_editMode);
        delete m_editMode;
    }

    // delete FileIconProvider singleton
    delete FileIconProvider::instance();

    delete m_mainWindow;
}

bool CorePlugin::initialize(const QStringList & /*arguments*/, QString *error_message)
{
    m_pm = ExtensionSystem::PluginManager::instance();
    const bool success = m_mainWindow->init(m_pm, error_message);
    if (success) {
#ifdef QT_WEBKIT
        QWebSettings *webSettings = QWebSettings::globalSettings();
        const QFont applicationFont = QApplication::font();
        webSettings->setFontFamily(QWebSettings::StandardFont, applicationFont.family());
        //webSettings->setFontSize(QWebSettings::DefaultFontSize, applicationFont.pointSize());
#endif
        m_welcomeMode = new WelcomeMode;
        addObject(m_welcomeMode);

        EditorManager *editorManager = qobject_cast<EditorManager*>(m_mainWindow->editorManager());
        m_editMode = new EditMode(editorManager);
        addObject(m_editMode);
    }
    return success;
}

void CorePlugin::extensionsInitialized()
{
    m_mainWindow->modeManager()->activateMode(m_welcomeMode->uniqueModeName());
    m_mainWindow->extensionsInitialized();
}

void CorePlugin::remoteArgument(const QString& arg)
{
    // An empty argument is sent to trigger activation
    // of the window via QtSingleApplication. It should be
    // the last of a sequence.
    if (arg.isEmpty()) {
        m_mainWindow->activateWindow();
    } else {
        m_mainWindow->openFiles(QStringList(arg));
    }
}

Q_EXPORT_PLUGIN(CorePlugin)
