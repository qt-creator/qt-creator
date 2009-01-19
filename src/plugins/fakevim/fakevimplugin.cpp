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

    bool initialize(const QStringList &arguments, QString *error_message);
    void shutdown();

private slots:
    void installHandler();
    void installHandler(QWidget *widget);
    void removeHandler(QWidget *widget);
    void showCommandBuffer(const QString &contents);
    void showExtraInformation(QWidget *, const QString &msg);
    void editorOpened(Core::IEditor *);
    void editorAboutToClose(Core::IEditor *);
    void changeSelection(QWidget *widget,
        const QList<QTextEdit::ExtraSelection> &selections);
    void writeFile(const QString &fileName, const QString &contents);

private:
    FakeVimPlugin *q;
    FakeVimHandler *m_handler;
    QAction *m_installHandlerAction; 
    Core::ICore *m_core;
    Core::IFile *m_currentFile;
};

} // namespace Internal
} // namespace FakeVim

FakeVimPluginPrivate::FakeVimPluginPrivate(FakeVimPlugin *plugin)
{       
    q = plugin;
    m_handler = 0;
    m_installHandlerAction = 0;
    m_core = 0;
    m_currentFile = 0;
}

FakeVimPluginPrivate::~FakeVimPluginPrivate()
{
}

void FakeVimPluginPrivate::shutdown()
{
    delete m_handler;
    m_handler = 0;
}

bool FakeVimPluginPrivate::initialize(const QStringList &arguments, QString *error_message)
{
    Q_UNUSED(arguments);
    Q_UNUSED(error_message);

    m_handler = new FakeVimHandler;

    m_core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
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

void FakeVimPluginPrivate::installHandler()
{
    if (Core::IEditor *editor = m_core->editorManager()->currentEditor())
        installHandler(editor->widget());
}

void FakeVimPluginPrivate::installHandler(QWidget *widget)
{
    connect(m_handler, SIGNAL(extraInformationChanged(QWidget *, QString)),
        this, SLOT(showExtraInformation(QWidget *, QString)));
    connect(m_handler, SIGNAL(commandBufferChanged(QString)),
        this, SLOT(showCommandBuffer(QString)));
    connect(m_handler, SIGNAL(quitRequested(QWidget *)),
        this, SLOT(removeHandler(QWidget *)));
    connect(m_handler,
        SIGNAL(selectionChanged(QWidget*,QList<QTextEdit::ExtraSelection>)),
        this, SLOT(changeSelection(QWidget*,QList<QTextEdit::ExtraSelection>)));

    m_handler->addWidget(widget);
    TextEditor::BaseTextEditor* editor =
        qobject_cast<TextEditor::BaseTextEditor*>(widget);
    if (editor) {
        m_currentFile = editor->file();
        m_handler->setCurrentFileName(editor->file()->fileName());
    }

    BaseTextEditor *bt = qobject_cast<BaseTextEditor *>(widget);
    if (bt) {
        using namespace TextEditor;
        using namespace FakeVim::Constants;
        TabSettings settings = bt->tabSettings();
        m_handler->setConfigValue(ConfigTabStop,
            QString::number(settings.m_tabSize));
        m_handler->setConfigValue(ConfigShiftWidth,
            QString::number(settings.m_indentSize));
        m_handler->setConfigValue(ConfigExpandTab,
            settings.m_spacesForTabs ? ConfigOn : ConfigOff);
        m_handler->setConfigValue(ConfigSmartTab,
            settings.m_smartBackspace ? ConfigOn : ConfigOff); 
        m_handler->setConfigValue(ConfigAutoIndent,
            settings.m_autoIndent ? ConfigOn : ConfigOff); 
    }
}

void FakeVimPluginPrivate::writeFile(const QString &fileName,
    const QString &contents)
{
    if (m_currentFile && fileName == m_currentFile->fileName()) {
        // Handle that as a special case for nicer interaction with core
        m_core->fileManager()->blockFileChange(m_currentFile);
        m_currentFile->save(fileName);
        m_core->fileManager()->unblockFileChange(m_currentFile);
    } else {
        QFile file(fileName);
        file.open(QIODevice::ReadWrite);
        { QTextStream ts(&file); ts << contents; }
        file.close();
    }
}

void FakeVimPluginPrivate::removeHandler(QWidget *widget)
{
    Q_UNUSED(widget);
    m_handler->removeWidget(widget);
    Core::EditorManager::instance()->hideEditorInfoBar(
        QLatin1String(Constants::MINI_BUFFER));
    m_currentFile = 0;
}

void FakeVimPluginPrivate::editorOpened(Core::IEditor *editor)
{
    Q_UNUSED(editor);
    //qDebug() << "OPENING: " << editor << editor->widget();
    //installHandler(editor->widget()); 
}

void FakeVimPluginPrivate::editorAboutToClose(Core::IEditor *editor)
{
    //qDebug() << "CLOSING: " << editor << editor->widget();
    removeHandler(editor->widget()); 
}

void FakeVimPluginPrivate::showCommandBuffer(const QString &contents)
{
    Core::EditorManager::instance()->showEditorInfoBar( 
        QLatin1String(Constants::MINI_BUFFER), contents,
        tr("Quit FakeVim"), m_handler, SLOT(quit()));
}

void FakeVimPluginPrivate::showExtraInformation(QWidget *widget, const QString &text)
{
    QMessageBox::information(widget, tr("FakeVim Information"), text);
}

void FakeVimPluginPrivate::changeSelection(QWidget *widget,
    const QList<QTextEdit::ExtraSelection> &selection)
{
    if (BaseTextEditor *bt = qobject_cast<BaseTextEditor *>(widget))
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

bool FakeVimPlugin::initialize(const QStringList &arguments, QString *error_message)
{
    return d->initialize(arguments, error_message);
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
