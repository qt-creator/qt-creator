/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "coreplugin.h"
#include "welcomemode.h"
#include "editmode.h"
#include "editormanager.h"
#include "mainwindow.h"
#include "modemanager.h"
#include "fileiconprovider.h"

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QtPlugin>
#if !defined(QT_NO_WEBKIT)
#include <QtGui/QApplication>
#include <QtWebKit/QWebSettings>
#endif

using namespace Core::Internal;

CorePlugin::CorePlugin() :
    m_mainWindow(new MainWindow), m_welcomeMode(0), m_editMode(0)
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

bool CorePlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    const bool success = m_mainWindow->init(errorMessage);
    if (success) {
#if !defined(QT_NO_WEBKIT)
        QWebSettings *webSettings = QWebSettings::globalSettings();
        const QFont applicationFont = QApplication::font();
        webSettings->setFontFamily(QWebSettings::StandardFont, applicationFont.family());
        //webSettings->setFontSize(QWebSettings::DefaultFontSize, applicationFont.pointSize());
#endif
        m_welcomeMode = new WelcomeMode;
        addObject(m_welcomeMode);

        EditorManager *editorManager = m_mainWindow->editorManager();
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
