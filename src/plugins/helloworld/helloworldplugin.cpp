/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "helloworldplugin.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/id.h>

#include <QDebug>
#include <QtPlugin>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>

namespace HelloWorld {
namespace Internal {

/*!  A mode with a push button based on BaseMode.  */

class HelloMode : public Core::IMode
{
public:
    HelloMode()
    {
        setWidget(new QPushButton(tr("Hello World PushButton!")));
        setContext(Core::Context("HelloWorld.MainView"));
        setDisplayName(tr("Hello world!"));
        setIcon(QIcon());
        setPriority(0);
        setId("HelloWorld.HelloWorldMode");
        setType("HelloWorld.HelloWorldMode");
        setContextHelpId(QString());
    }
};


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

    \a errorMessage can be used to pass an error message to the plugin system,
       if there was any.
*/
bool HelloWorldPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    // Create a unique context for our own view, that will be used for the
    // menu entry later.
    Core::Context context("HelloWorld.MainView");

    // Create an action to be triggered by a menu entry
    QAction *helloWorldAction = new QAction(tr("Say \"&Hello World!\""), this);
    connect(helloWorldAction, SIGNAL(triggered()), SLOT(sayHelloWorld()));

    // Register the action with the action manager
    Core::Command *command =
            Core::ActionManager::registerAction(
                    helloWorldAction, "HelloWorld.HelloWorldAction", context);

    // Create our own menu to place in the Tools menu
    Core::ActionContainer *helloWorldMenu =
            Core::ActionManager::createMenu("HelloWorld.HelloWorldMenu");
    QMenu *menu = helloWorldMenu->menu();
    menu->setTitle(tr("&Hello World"));
    menu->setEnabled(true);

    // Add the Hello World action command to the menu
    helloWorldMenu->addAction(command);

    // Request the Tools menu and add the Hello World menu to it
    Core::ActionContainer *toolsMenu =
            Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsMenu->addMenu(helloWorldMenu);

    // Add a mode with a push button based on BaseMode. Like the BaseView,
    // it will unregister itself from the plugin manager when it is deleted.
    Core::IMode *helloMode = new HelloMode;
    addAutoReleasedObject(helloMode);

    return true;
}

/*! Notification that all extensions that this plugin depends on have been
    initialized. The dependencies are defined in the plugins .pluginspec file.

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
            0, tr("Hello World!"), tr("Hello World! Beautiful day today, isn't it?"));
}

} // namespace Internal
} // namespace HelloWorld

Q_EXPORT_PLUGIN(HelloWorld::Internal::HelloWorldPlugin)
