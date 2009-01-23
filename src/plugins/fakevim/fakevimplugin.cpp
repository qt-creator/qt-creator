/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
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
#include <QtCore/QtPlugin>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSettings>

#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QTextEdit>


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
// FakeVimPluginPrivate
//
///////////////////////////////////////////////////////////////////////

namespace FakeVim {
namespace Internal {

class FakeVimPluginPrivate : public QObject
{
    Q_OBJECT

public:
    FakeVimPluginPrivate(FakeVimPlugin *);
    ~FakeVimPluginPrivate();
    friend class FakeVimPlugin;

    bool initialize();
    void shutdown();

private slots:
    void editorOpened(Core::IEditor *);
    void editorAboutToClose(Core::IEditor *);

    void installHandler();
    void installHandler(Core::IEditor *editor);
    void removeHandler();

    void showCommandBuffer(const QString &contents);
    void showExtraInformation(const QString &msg);
    void changeSelection(const QList<QTextEdit::ExtraSelection> &selections);
    void writeFile(bool *handled, const QString &fileName, const QString &contents);

private:
    FakeVimPlugin *q;
    QAction *m_installHandlerAction; 
    Core::ICore *m_core;
};

} // namespace Internal
} // namespace FakeVim

FakeVimPluginPrivate::FakeVimPluginPrivate(FakeVimPlugin *plugin)
{       
    q = plugin;
    m_installHandlerAction = 0;
    m_core = 0;
}

FakeVimPluginPrivate::~FakeVimPluginPrivate()
{
}

void FakeVimPluginPrivate::shutdown()
{
}

bool FakeVimPluginPrivate::initialize()
{
    m_core = Core::ICore::instance();
    QTC_ASSERT(m_core, return false);

    Core::ActionManager *actionManager = m_core->actionManager();
    QTC_ASSERT(actionManager, return false);

    QList<int> globalcontext;
    globalcontext << Core::Constants::C_GLOBAL_ID;

    m_installHandlerAction = new QAction(this);
    m_installHandlerAction->setText(tr("Set vi-Style Keyboard Action Handler"));
    
    Core::Command *cmd = 0;
    cmd = actionManager->registerAction(m_installHandlerAction,
        Constants::INSTALL_HANDLER, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::INSTALL_KEY));

    ActionContainer *advancedMenu =
        actionManager->actionContainer(Core::Constants::M_EDIT_ADVANCED);
    advancedMenu->addAction(cmd, Core::Constants::G_EDIT_EDITOR);

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

void FakeVimPluginPrivate::installHandler()
{
    if (Core::IEditor *editor = m_core->editorManager()->currentEditor())
        installHandler(editor);
}

void FakeVimPluginPrivate::installHandler(Core::IEditor *editor)
{
    QWidget *widget = editor->widget();
    
    FakeVimHandler *handler = new FakeVimHandler(widget, this);

    connect(handler, SIGNAL(extraInformationChanged(QString)),
        this, SLOT(showExtraInformation(QString)));
    connect(handler, SIGNAL(commandBufferChanged(QString)),
        this, SLOT(showCommandBuffer(QString)));
    connect(handler, SIGNAL(quitRequested()),
        this, SLOT(removeHandler()), Qt::QueuedConnection);
    connect(handler, SIGNAL(writeFileRequested(bool*,QString,QString)),
        this, SLOT(writeFile(bool*,QString,QString)));
    connect(handler, SIGNAL(selectionChanged(QList<QTextEdit::ExtraSelection>)),
        this, SLOT(changeSelection(QList<QTextEdit::ExtraSelection>)));

    handler->setupWidget();
    handler->setExtraData(editor);

    if (BaseTextEditor *bt = qobject_cast<BaseTextEditor *>(widget)) {
        using namespace TextEditor;
        using namespace FakeVim::Constants;
        handler->setCurrentFileName(editor->file()->fileName());
        TabSettings settings = bt->tabSettings();
        handler->setConfigValue(ConfigTabStop,
            QString::number(settings.m_tabSize));
        handler->setConfigValue(ConfigShiftWidth,
            QString::number(settings.m_indentSize));
        handler->setConfigValue(ConfigExpandTab,
            settings.m_spacesForTabs ? ConfigOn : ConfigOff);
        handler->setConfigValue(ConfigSmartTab,
            settings.m_smartBackspace ? ConfigOn : ConfigOff); 
        handler->setConfigValue(ConfigAutoIndent,
            settings.m_autoIndent ? ConfigOn : ConfigOff); 
    }
}

void FakeVimPluginPrivate::writeFile(bool *handled,
    const QString &fileName, const QString &contents)
{
    //qDebug() << "HANDLING WRITE FILE" << fileName << sender();
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;

    Core::IEditor *editor = qobject_cast<Core::IEditor *>(handler->extraData());
    if (editor && editor->file()->fileName() == fileName) {
        //qDebug() << "HANDLING CORE SAVE";
        // Handle that as a special case for nicer interaction with core
        Core::IFile *file = editor->file();
        m_core->fileManager()->blockFileChange(file);
        file->save(fileName);
        m_core->fileManager()->unblockFileChange(file);
        *handled = true;
    } 
}

void FakeVimPluginPrivate::removeHandler()
{
    if (FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender())) {
        handler->restoreWidget();
        handler->deleteLater();
    }
    Core::EditorManager::instance()->hideEditorInfoBar(
        QLatin1String(Constants::MINI_BUFFER));
}

void FakeVimPluginPrivate::editorOpened(Core::IEditor *editor)
{
    Q_UNUSED(editor);
    //qDebug() << "OPENING: " << editor << editor->widget();
    //installHandler(editor);
}

void FakeVimPluginPrivate::editorAboutToClose(Core::IEditor *editor)
{
    Q_UNUSED(editor);
    //qDebug() << "CLOSING: " << editor;
}

void FakeVimPluginPrivate::showCommandBuffer(const QString &contents)
{
    //qDebug() << "SHOW COMMAND BUFFER" << contents;
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (handler) {
        Core::EditorManager::instance()->showEditorInfoBar( 
            QLatin1String(Constants::MINI_BUFFER), contents,
            tr("Quit FakeVim"), handler, SLOT(quit()));
    }
}

void FakeVimPluginPrivate::showExtraInformation(const QString &text)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (handler)
        QMessageBox::information(handler->widget(), tr("FakeVim Information"), text);
}

void FakeVimPluginPrivate::changeSelection
    (const QList<QTextEdit::ExtraSelection> &selection)
{
    if (FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender()))
        if (BaseTextEditor *bt = qobject_cast<BaseTextEditor *>(handler->widget()))
            bt->setExtraSelections(BaseTextEditor::FakeVimSelection, selection);
}


///////////////////////////////////////////////////////////////////////
//
// FakeVimPlugin
//
///////////////////////////////////////////////////////////////////////

FakeVimPlugin::FakeVimPlugin()
    : d(new FakeVimPluginPrivate(this))
{}

FakeVimPlugin::~FakeVimPlugin()
{
    delete d;
}

bool FakeVimPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);
    return d->initialize();
}

void FakeVimPlugin::shutdown()
{
    d->shutdown();
}

void FakeVimPlugin::extensionsInitialized()
{
}

#include "fakevimplugin.moc"

Q_EXPORT_PLUGIN(FakeVimPlugin)
