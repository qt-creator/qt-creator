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

#include "handler.h"

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

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/qplugin.h>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSettings>

#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>


using namespace FakeVim::Internal;
//using namespace FakeVim::Constants;
using namespace TextEditor;
using namespace Core;
using namespace ProjectExplorer;


namespace FakeVim {
namespace Constants {

const char * const INSTALL_HANDLER        = "FakeVim.InstallHandler";
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
    m_pm = 0;
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

    m_pm = ExtensionSystem::PluginManager::instance();

    ICore *core = m_pm->getObject<Core::ICore>();
    QTC_ASSERT(core, return false);

    Core::ActionManagerInterface *actionManager = core->actionManager();
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
    return true;
}

void FakeVimPlugin::extensionsInitialized()
{
}

void FakeVimPlugin::installHandler()
{
    ICore *core = m_pm->getObject<Core::ICore>();
    if (!core || !core->editorManager())
        return;
    Core::IEditor *editor = core->editorManager()->currentEditor();
    ITextEditor *textEditor = qobject_cast<ITextEditor*>(editor);
    if (!textEditor)
        return;

    QWidget *widget = textEditor->widget();
    QPlainTextEdit *plainTextEdit = qobject_cast<QPlainTextEdit *>(widget);
    if (!plainTextEdit)
        return;
    plainTextEdit->removeEventFilter(m_handler);
    plainTextEdit->installEventFilter(m_handler);
    plainTextEdit->setOverwriteMode(true);

    //QMainWindow mw;
    connect(m_handler, SIGNAL(commandBufferChanged(QString)),
        this, SLOT(showCommandBuffer(QString)));
    connect(m_handler, SIGNAL(quitRequested(QObject *)),
        this, SLOT(removeHandler(QObject *)));
}

void FakeVimPlugin::removeHandler(QObject *ob)
{
    ob->removeEventFilter(m_handler);
    QPlainTextEdit *plainTextEdit = qobject_cast<QPlainTextEdit *>(ob);
    if (!plainTextEdit)
        return;
    plainTextEdit->setOverwriteMode(false);
}

void FakeVimPlugin::showCommandBuffer(const QString &contents)
{
    //qDebug() << "CMD: " << contents;
}


//#include "fakevimplugin.moc"

Q_EXPORT_PLUGIN(FakeVimPlugin)
