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

#include "helloworldplugin.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/basemode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/uniqueidmanager.h>

#include <QtCore/QDebug>
#include <QtCore/QtPlugin>
#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>

using namespace HelloWorld::Internal;

/*! Constructs the Hello World plugin. Normally plugins don't do anything in
    their constructor except for initializing their member variables. The
    actual work is done later, in the initialize() and extensionsInitialized()
    methods.
*/
HelloWorldPlugin::HelloWorldPlugin()
{
}

/*! Plugins are responsible for deleting objects they created on the heap, and
    to unregister objects from the plugin manager that they registered there.
*/
HelloWorldPlugin::~HelloWorldPlugin()
{
}

/*! Initializes the plugin. Returns true on success.
    Plugins want to register objects with the plugin manager here.

    \a error_message can be used to pass an error message to the plugin system,
       if there was any.
*/
bool HelloWorldPlugin::initialize(const QStringList &arguments, QString *error_message)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error_message)

    // Get the primary access point to the workbench.
    Core::ICore *core = Core::ICore::instance();

    // Create a unique context id for our own view, that will be used for the
    // menu entry later.
    QList<int> context = QList<int>()
        << core->uniqueIDManager()->uniqueIdentifier(
                QLatin1String("HelloWorld.MainView"));

    // Create an action to be triggered by a menu entry
    QAction *helloWorldAction = new QAction("Say \"&Hello World!\"", this);
    connect(helloWorldAction, SIGNAL(triggered()), SLOT(sayHelloWorld()));

    // Register the action with the action manager
    Core::ActionManager *actionManager = core->actionManager();
    Core::Command *command =
            actionManager->registerAction(
                    helloWorldAction, "HelloWorld.HelloWorldAction", context);

    // Create our own menu to place in the Tools menu
    Core::ActionContainer *helloWorldMenu =
            actionManager->createMenu("HelloWorld.HelloWorldMenu");
    QMenu *menu = helloWorldMenu->menu();
    menu->setTitle(tr("&Hello World"));
    menu->setEnabled(true);

    // Add the Hello World action command to the menu
    helloWorldMenu->addAction(command);

    // Request the Tools menu and add the Hello World menu to it
    Core::ActionContainer *toolsMenu =
            actionManager->actionContainer(Core::Constants::M_TOOLS);
    toolsMenu->addMenu(helloWorldMenu);

    // Add a mode with a push button based on BaseMode. Like the BaseView,
    // it will unregister itself from the plugin manager when it is deleted.
    Core::BaseMode *baseMode = new Core::BaseMode;
    baseMode->setUniqueModeName("HelloWorld.HelloWorldMode");
    baseMode->setName(tr("Hello world!"));
    baseMode->setIcon(QIcon());
    baseMode->setPriority(0);
    baseMode->setWidget(new QPushButton(tr("Hello World PushButton!")));
    addAutoReleasedObject(baseMode);

    // Add the Hello World action command to the mode manager (with 0 priority)
    Core::ModeManager *modeManager = core->modeManager();
    modeManager->addAction(command, 0);

    return true;
}

/*! Notification that all extensions that this plugin depends on have been
    initialized. The dependencies are defined in the plugins .qwp file.

    Normally this method is used for things that rely on other plugins to have
    added objects to the plugin manager, that implement interfaces that we're
    interested in. These objects can now be requested through the
    PluginManagerInterface.

    The HelloWorldPlugin doesn't need things from other plugins, so it does
    nothing here.
*/
void HelloWorldPlugin::extensionsInitialized()
{
}

void HelloWorldPlugin::sayHelloWorld()
{
    // When passing 0 for the parent, the message box becomes an
    // application-global modal dialog box
    QMessageBox::information(
            0, "Hello World!", "Hello World! Beautiful day today, isn't it?");
}

Q_EXPORT_PLUGIN(HelloWorldPlugin)
