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
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "fakevimplugin.h"

#include "fakevimconstants.h"
#include "fakevimhandler.h"

#include <coreplugin/actionmanager/actionmanagerinterface.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/uniqueidmanager.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/qplugin.h>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSettings>

#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>


using namespace FakeVim::Internal;
using namespace TextEditor;
using namespace Core;
using namespace ProjectExplorer;


namespace FakeVim {
namespace Constants {

const char * const INSTALL_HANDLER        = "FakeVim.InstallHandler";
const char * const MINI_BUFFER            = "FakeVim.MiniBuffer";
const char * const INSTALL_KEY            = "Alt+V,Alt+V";

} // namespace Constants
} // namespace FakeVim


///////////////////////////////////////////////////////////////////////
//
// FakeVimPlugin
//
///////////////////////////////////////////////////////////////////////

FakeVimPlugin::FakeVimPlugin()
{
    m_core = 0;
    m_handler = 0;
}

FakeVimPlugin::~FakeVimPlugin()
{}

void FakeVimPlugin::shutdown()
{
    delete m_handler;
    m_handler = 0;
}

bool FakeVimPlugin::initialize(const QStringList &arguments, QString *error_message)
{
    Q_UNUSED(arguments);
    Q_UNUSED(error_message);

    m_handler = new FakeVimHandler;

    m_core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    QTC_ASSERT(m_core, return false);

    Core::ActionManagerInterface *actionManager = m_core->actionManager();
    QTC_ASSERT(actionManager, return false);

    QList<int> globalcontext;
    globalcontext << Core::Constants::C_GLOBAL_ID;

    m_installHandlerAction = new QAction(this);
    m_installHandlerAction->setText(tr("Set vi-Style Keyboard Action Handler"));
    
    Core::ICommand *cmd = 0;
    cmd = actionManager->registerAction(m_installHandlerAction,
        Constants::INSTALL_HANDLER, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::INSTALL_KEY));

    IActionContainer *advancedMenu =
        actionManager->actionContainer(Core::Constants::M_EDIT_ADVANCED);
    advancedMenu->addAction(cmd);

    connect(m_installHandlerAction, SIGNAL(triggered()),
        this, SLOT(installHandler()));

    // EditorManager
    QObject *editorManager = m_core->editorManager();
    connect(editorManager, SIGNAL(editorAboutToClose(Core::IEditor*)),
        this, SLOT(editorAboutToClose(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorOpened(Core::IEditor*)),
        this, SLOT(editorOpened(Core::IEditor*)));

    return true;
}

void FakeVimPlugin::extensionsInitialized()
{
}

void FakeVimPlugin::installHandler()
{
    if (Core::IEditor *editor = m_core->editorManager()->currentEditor())
        installHandler(editor->widget());
}

void FakeVimPlugin::installHandler(QWidget *widget)
{
    connect(m_handler, SIGNAL(extraInformationChanged(QString)),
        this, SLOT(showExtraInformation(QString)));
    connect(m_handler, SIGNAL(commandBufferChanged(QString)),
        this, SLOT(showCommandBuffer(QString)));
    connect(m_handler, SIGNAL(quitRequested(QWidget *)),
        this, SLOT(removeHandler(QWidget *)));

    m_handler->addWidget(widget);

    BaseTextEditor *baseTextEditor = qobject_cast<BaseTextEditor *>(widget);
    if (baseTextEditor) {
        using namespace TextEditor;
        using namespace FakeVim::Constants;
        TabSettings settings = baseTextEditor->tabSettings();
        m_handler->setConfigValue(ConfigTabStop,
            QString::number(settings.m_tabSize));
        m_handler->setConfigValue(ConfigShiftWidth,
            QString::number(settings.m_indentSize));
        m_handler->setConfigValue(ConfigExpandTab,
            settings.m_spacesForTabs ? ConfigOn : ConfigOff);
        m_handler->setConfigValue(ConfigSmartTab,
            settings.m_smartBackspace ? ConfigOn : ConfigOff); 
        //m_handler->setConfigValue(ConfigSmartTab,
        //    settings.m_autoIndent ? ConfigOn : ConfigOff); 
    }
}

void FakeVimPlugin::removeHandler(QWidget *widget)
{
    //m_handler->removeWidget(widget);
    Core::EditorManager::instance()->hideEditorInfoBar(
        QLatin1String(Constants::MINI_BUFFER));
}

void FakeVimPlugin::editorOpened(Core::IEditor *editor)
{
    //qDebug() << "OPENING: " << editor << editor->widget();
    //installHandler(editor->widget()); 
}

void FakeVimPlugin::editorAboutToClose(Core::IEditor *editor)
{
    //qDebug() << "CLOSING: " << editor << editor->widget();
    removeHandler(editor->widget()); 
}

void FakeVimPlugin::showCommandBuffer(const QString &contents)
{
    Core::EditorManager::instance()->showEditorInfoBar( 
        QLatin1String(Constants::MINI_BUFFER), contents,
        tr("Quit FakeVim"), m_handler, SLOT(quit()));
}

void FakeVimPlugin::showExtraInformation(const QString &text)
{
    QMessageBox::information(0, tr("FakeVim Information"), text);
}

//#include "fakevimplugin.moc"

Q_EXPORT_PLUGIN(FakeVimPlugin)
