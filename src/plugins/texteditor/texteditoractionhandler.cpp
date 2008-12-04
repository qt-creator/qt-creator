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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "texteditoractionhandler.h"
#include "texteditorconstants.h"
#include "basetexteditor.h"
#include "texteditorplugin.h"
#include "linenumberfilter.h"

#include <quickopen/quickopenmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanagerinterface.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QSet>
#include <QtCore/QtDebug>
#include <QtGui/QAction>
#include <QtGui/QTextCursor>

using namespace TextEditor;
using namespace TextEditor::Internal;

TextEditorActionHandler::TextEditorActionHandler(Core::ICore *core,
                                                 const QString &context,
                                                 uint optionalActions)  :
    QObject(core),
    m_optionalActions(optionalActions),
    m_currentEditor(0),
    m_core(core),
    m_initialized(false)
{
    m_undoAction = m_redoAction = m_copyAction = m_cutAction = m_pasteAction
                 = m_selectAllAction = m_gotoAction = m_printAction = m_formatAction
                 = m_visualizeWhitespaceAction = m_textWrappingAction
                 = m_unCommentSelectionAction = m_unCollapseAllAction
                 = m_collapseAction = m_expandAction
                 = m_deleteLineAction = m_selectEncodingAction
                 = m_increaseFontSizeAction = m_decreaseFontSizeAction
                 = m_gotoBlockStartAction = m_gotoBlockStartWithSelectionAction
                 = m_gotoBlockEndAction = m_gotoBlockEndWithSelectionAction
                 = m_selectBlockUpAction = m_selectBlockDownAction
                 = 0;

    m_contextId << m_core->uniqueIDManager()->uniqueIdentifier(context);

    connect(m_core, SIGNAL(contextAboutToChange(Core::IContext *)),
        this, SLOT(updateCurrentEditor(Core::IContext *)));
}

void TextEditorActionHandler::setupActions(BaseTextEditor *editor)
{
    initializeActions();
    editor->setActionHack(this);
    QObject::connect(editor, SIGNAL(undoAvailable(bool)), this, SLOT(updateUndoAction()));
    QObject::connect(editor, SIGNAL(redoAvailable(bool)), this, SLOT(updateRedoAction()));
    QObject::connect(editor, SIGNAL(copyAvailable(bool)), this, SLOT(updateCopyAction()));
}


void TextEditorActionHandler::initializeActions()
{
    if (!m_initialized) {
        createActions();
        m_initialized = true;
    }
}

void TextEditorActionHandler::createActions()
{
    m_undoAction      = registerNewAction(QLatin1String(Core::Constants::UNDO),      this, SLOT(undoAction()),
                                          tr("&Undo"));
    m_redoAction      = registerNewAction(QLatin1String(Core::Constants::REDO),      this, SLOT(redoAction()),
                                          tr("&Redo"));
    m_copyAction      = registerNewAction(QLatin1String(Core::Constants::COPY),      this, SLOT(copyAction()));
    m_cutAction       = registerNewAction(QLatin1String(Core::Constants::CUT),       this, SLOT(cutAction()));
    m_pasteAction     = registerNewAction(QLatin1String(Core::Constants::PASTE),     this, SLOT(pasteAction()));
    m_selectAllAction = registerNewAction(QLatin1String(Core::Constants::SELECTALL), this, SLOT(selectAllAction()));
    m_gotoAction      = registerNewAction(QLatin1String(Core::Constants::GOTO),      this, SLOT(gotoAction()));
    m_printAction     = registerNewAction(QLatin1String(Core::Constants::PRINT),     this, SLOT(printAction()));

    Core::ActionManagerInterface *am = m_core->actionManager();

    Core::IActionContainer *medit = am->actionContainer(Core::Constants::M_EDIT);
    Core::IActionContainer *advancedMenu = am->actionContainer(Core::Constants::M_EDIT_ADVANCED);

    m_selectEncodingAction = new QAction(tr("Select Encoding..."), this);
    Core::ICommand *command = am->registerAction(m_selectEncodingAction, Constants::SELECT_ENCODING, m_contextId);
    connect(m_selectEncodingAction, SIGNAL(triggered()), this, SLOT(selectEncoding()));
    medit->addAction(command, Core::Constants::G_EDIT_OTHER);


    m_formatAction = new QAction(tr("Auto-&indent Selection"), this);
    command = am->registerAction(m_formatAction, TextEditor::Constants::AUTO_INDENT_SELECTION, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+I")));
    advancedMenu->addAction(command);
    connect(m_formatAction, SIGNAL(triggered(bool)), this, SLOT(formatAction()));


    m_visualizeWhitespaceAction = new QAction(tr("Visualize &Whitespace"), this);
    m_visualizeWhitespaceAction->setCheckable(true);
    command = am->registerAction(m_visualizeWhitespaceAction,
                                                 TextEditor::Constants::VISUALIZE_WHITESPACE, m_contextId);
#ifndef Q_OS_MAC
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+E, Ctrl+V")));
#endif

    advancedMenu->addAction(command);
    connect(m_visualizeWhitespaceAction, SIGNAL(triggered(bool)), this, SLOT(setVisualizeWhitespace(bool)));

    m_textWrappingAction = new QAction(tr("Enable Text &Wrapping"), this);
    m_textWrappingAction->setCheckable(true);
    command = am->registerAction(m_textWrappingAction,
                                                 TextEditor::Constants::TEXT_WRAPPING, m_contextId);
#ifndef Q_OS_MAC
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+E, Ctrl+W")));
#endif
    advancedMenu->addAction(command);
    connect(m_textWrappingAction, SIGNAL(triggered(bool)), this, SLOT(setTextWrapping(bool)));


    m_unCommentSelectionAction = new QAction(tr("(Un)Comment &Selection"), this);
    command = am->registerAction(m_unCommentSelectionAction, Constants::UN_COMMENT_SELECTION, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+/")));
    connect(m_unCommentSelectionAction, SIGNAL(triggered()), this, SLOT(unCommentSelection()));
    advancedMenu->addAction(command);

    m_deleteLineAction = new QAction(tr("Delete &Line"), this);
    command = am->registerAction(m_deleteLineAction, Constants::DELETE_LINE, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Shift+Del")));
    connect(m_deleteLineAction, SIGNAL(triggered()), this, SLOT(deleteLine()));

    m_collapseAction = new QAction(tr("Collapse"), this);
    command = am->registerAction(m_collapseAction, Constants::COLLAPSE, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+<")));
    connect(m_collapseAction, SIGNAL(triggered()), this, SLOT(collapse()));

    m_expandAction = new QAction(tr("Expand"), this);
    command = am->registerAction(m_expandAction, Constants::EXPAND, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+>")));
    connect(m_expandAction, SIGNAL(triggered()), this, SLOT(expand()));

    m_unCollapseAllAction = new QAction(tr("(Un)&Collapse All"), this);
    command = am->registerAction(m_unCollapseAllAction, Constants::UN_COLLAPSE_ALL, m_contextId);
    connect(m_unCollapseAllAction, SIGNAL(triggered()), this, SLOT(unCollapseAll()));
    advancedMenu->addAction(command);

    m_increaseFontSizeAction = new QAction(tr("Increase Font Size"), this);
    command = am->registerAction(m_increaseFontSizeAction, Constants::INCREASE_FONT_SIZE, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl++")));
    connect(m_increaseFontSizeAction, SIGNAL(triggered()), this, SLOT(increaseFontSize()));
    advancedMenu->addAction(command);
    
    m_decreaseFontSizeAction = new QAction(tr("Decrease Font Size"), this);
    command = am->registerAction(m_decreaseFontSizeAction, Constants::DECREASE_FONT_SIZE, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+-")));
    connect(m_decreaseFontSizeAction, SIGNAL(triggered()), this, SLOT(decreaseFontSize()));
    advancedMenu->addAction(command);

    m_gotoBlockStartAction = new QAction(tr("Goto Block Start"), this);
    command = am->registerAction(m_gotoBlockStartAction, Constants::GOTO_BLOCK_START, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+[")));
    connect(m_gotoBlockStartAction, SIGNAL(triggered()), this, SLOT(gotoBlockStart()));

    m_gotoBlockEndAction = new QAction(tr("Goto Block End"), this);
    command = am->registerAction(m_gotoBlockEndAction, Constants::GOTO_BLOCK_END, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+]")));
    connect(m_gotoBlockEndAction, SIGNAL(triggered()), this, SLOT(gotoBlockEnd()));

    m_gotoBlockStartWithSelectionAction = new QAction(tr("Goto Block Start With Selection"), this);
    command = am->registerAction(m_gotoBlockStartWithSelectionAction, Constants::GOTO_BLOCK_START_WITH_SELECTION, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+{")));
    connect(m_gotoBlockStartWithSelectionAction, SIGNAL(triggered()), this, SLOT(gotoBlockStartWithSelection()));

    m_gotoBlockEndWithSelectionAction = new QAction(tr("Goto Block End With Selection"), this);
    command = am->registerAction(m_gotoBlockEndWithSelectionAction, Constants::GOTO_BLOCK_END_WITH_SELECTION, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+}")));
    connect(m_gotoBlockEndWithSelectionAction, SIGNAL(triggered()), this, SLOT(gotoBlockEndWithSelection()));

    m_selectBlockUpAction= new QAction(tr("Select Block Up"), this);
    command = am->registerAction(m_selectBlockUpAction, Constants::SELECT_BLOCK_UP, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+U")));
    connect(m_selectBlockUpAction, SIGNAL(triggered()), this, SLOT(selectBlockUp()));

    m_selectBlockDownAction= new QAction(tr("Select Block Down"), this);
    command = am->registerAction(m_selectBlockDownAction, Constants::SELECT_BLOCK_DOWN, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+U")));
    connect(m_selectBlockDownAction, SIGNAL(triggered()), this, SLOT(selectBlockDown()));
}

bool TextEditorActionHandler::supportsAction(const QString & /*id */) const
{
    return true;
}

QAction *TextEditorActionHandler::registerNewAction(const QString &id, const QString &title)
{
    if (!supportsAction(id))
        return 0;

    QAction *result = new QAction(title, this);
    m_core->actionManager()->registerAction(result, id, m_contextId);
    return result;
}

QAction *TextEditorActionHandler::registerNewAction(const QString &id,
                                                    QObject *receiver,
                                                    const char *slot,
                                                    const QString &title)
{
    QAction *rc = registerNewAction(id, title);
    if (!rc)
        return 0;

    connect(rc, SIGNAL(triggered()), receiver, slot);
    return rc;
}

TextEditorActionHandler::UpdateMode TextEditorActionHandler::updateMode() const
{
    if (!m_currentEditor)
        return NoEditor;
    return m_currentEditor->file()->isReadOnly() ? ReadOnlyMode : WriteMode;
}

void TextEditorActionHandler::updateActions()
{
    updateActions(updateMode());
}

void TextEditorActionHandler::updateActions(UpdateMode um)
{
    if (!m_initialized)
        return;
    m_pasteAction->setEnabled(um != NoEditor);
    m_selectAllAction->setEnabled(um != NoEditor);
    m_gotoAction->setEnabled(um != NoEditor);
    m_selectEncodingAction->setEnabled(um != NoEditor);
    m_printAction->setEnabled(um != NoEditor);
    m_formatAction->setEnabled((m_optionalActions & Format) && um != NoEditor);
    m_unCommentSelectionAction->setEnabled((m_optionalActions & UnCommentSelection) && um != NoEditor);
    m_collapseAction->setEnabled(um != NoEditor);
    m_expandAction->setEnabled(um != NoEditor);
    m_unCollapseAllAction->setEnabled((m_optionalActions & UnCollapseAll) && um != NoEditor);
    m_decreaseFontSizeAction->setEnabled(um != NoEditor);
    m_increaseFontSizeAction->setEnabled(um != NoEditor);
    m_gotoBlockStartAction->setEnabled(um != NoEditor);
    m_gotoBlockStartWithSelectionAction->setEnabled(um != NoEditor);
    m_gotoBlockEndAction->setEnabled(um != NoEditor);
    m_gotoBlockEndWithSelectionAction->setEnabled(um != NoEditor);
    m_selectBlockUpAction->setEnabled(um != NoEditor);
    m_selectBlockDownAction->setEnabled(um != NoEditor);

    m_visualizeWhitespaceAction->setEnabled(um != NoEditor);
    if (m_currentEditor)
        m_visualizeWhitespaceAction->setChecked(m_currentEditor->displaySettings().m_visualizeWhitespace);
    if (m_textWrappingAction) {
        m_textWrappingAction->setEnabled(um != NoEditor);
        if (m_currentEditor)
            m_textWrappingAction->setChecked(m_currentEditor->displaySettings().m_textWrapping);
    }

    updateRedoAction();
    updateUndoAction();
    updateCopyAction();
}

void TextEditorActionHandler::updateRedoAction()
{
    if (m_redoAction)
        m_redoAction->setEnabled(m_currentEditor && m_currentEditor->document()->isRedoAvailable());
}

void TextEditorActionHandler::updateUndoAction()
{
    if (m_undoAction)
        m_undoAction->setEnabled(m_currentEditor && m_currentEditor->document()->isUndoAvailable());
}

void TextEditorActionHandler::updateCopyAction()
{
    const bool hasCopyableText = m_currentEditor &&  m_currentEditor->textCursor().hasSelection();
    if (m_cutAction)
        m_cutAction->setEnabled(hasCopyableText && updateMode() == WriteMode);
    if (m_copyAction)
        m_copyAction->setEnabled(hasCopyableText);
}

void TextEditorActionHandler::undoAction()
{
    if (m_currentEditor)
        m_currentEditor->undo();
}

void TextEditorActionHandler::redoAction()
{
    if (m_currentEditor)
        m_currentEditor->redo();
}

void TextEditorActionHandler::copyAction()
{
    if (m_currentEditor)
        m_currentEditor->copy();
}

void TextEditorActionHandler::cutAction()
{
    if (m_currentEditor)
        m_currentEditor->cut();
}

void TextEditorActionHandler::pasteAction()
{
    if (m_currentEditor)
        m_currentEditor->paste();
}

void TextEditorActionHandler::selectAllAction()
{
    if (m_currentEditor)
        m_currentEditor->selectAll();
}

void TextEditorActionHandler::gotoAction()
{
    QuickOpen::QuickOpenManager *quickopen = QuickOpen::QuickOpenManager::instance();
    Q_ASSERT(quickopen);
    QString shortcut = TextEditorPlugin::instance()->lineNumberFilter()->shortcutString();
    quickopen->show(shortcut + " <line number>", 2, 13);
}

void TextEditorActionHandler::printAction()
{
    if (m_currentEditor)
        m_currentEditor->print(m_core->printer());
}

void TextEditorActionHandler::formatAction()
{
    if (m_currentEditor)
        m_currentEditor->format();
}


void TextEditorActionHandler::setVisualizeWhitespace(bool checked)
{
    if (m_currentEditor) {
        DisplaySettings ds = m_currentEditor->displaySettings();
        ds.m_visualizeWhitespace = checked;
        m_currentEditor->setDisplaySettings(ds);
    }
}

void TextEditorActionHandler::setTextWrapping(bool checked)
{
    if (m_currentEditor) {
        DisplaySettings ds = m_currentEditor->displaySettings();
        ds.m_textWrapping = checked;
        m_currentEditor->setDisplaySettings(ds);
    }
}

#define FUNCTION(funcname) void TextEditorActionHandler::funcname ()\
{\
    if (m_currentEditor)\
        m_currentEditor->funcname ();\
}
#define FUNCTION2(funcname, funcname2) void TextEditorActionHandler::funcname ()\
{\
    if (m_currentEditor)\
        m_currentEditor->funcname2 ();\
}

FUNCTION(unCommentSelection)
FUNCTION(deleteLine)
FUNCTION(unCollapseAll)
FUNCTION(collapse)
FUNCTION(expand)
FUNCTION2(increaseFontSize, zoomIn)
FUNCTION2(decreaseFontSize, zoomOut)
FUNCTION(selectEncoding)
FUNCTION(gotoBlockStart)
FUNCTION(gotoBlockEnd)
FUNCTION(gotoBlockStartWithSelection)
FUNCTION(gotoBlockEndWithSelection)
FUNCTION(selectBlockUp)
FUNCTION(selectBlockDown)

void TextEditorActionHandler::updateCurrentEditor(Core::IContext *object)
{
    do {
        if (!object) {
            if (!m_currentEditor)
                return;

            m_currentEditor = 0;
            break;
        }
        BaseTextEditor *editor = qobject_cast<BaseTextEditor *>(object->widget());
        if (!editor) {
            if (!m_currentEditor)
                return;

            m_currentEditor = 0;
            break;
        }

        if (editor == m_currentEditor)
            return;

        if (editor->actionHack() != this) {
             m_currentEditor = 0;
             break;
         }

        m_currentEditor = editor;

    } while (false);
    updateActions();
}


const QPointer<BaseTextEditor> &TextEditorActionHandler::currentEditor() const
{
    return m_currentEditor;
}

Core::ICore *TextEditorActionHandler::core() const
{
    return m_core;
}

