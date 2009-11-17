/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "texteditoractionhandler.h"
#include "texteditorconstants.h"
#include "basetexteditor.h"
#include "texteditorplugin.h"
#include "linenumberfilter.h"

#include <locator/locatormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QSet>
#include <QtCore/QtDebug>
#include <QtGui/QAction>
#include <QtGui/QTextCursor>

using namespace TextEditor;
using namespace TextEditor::Internal;

TextEditorActionHandler::TextEditorActionHandler(const QString &context,
                                                 uint optionalActions)
  : QObject(Core::ICore::instance()),
    m_undoAction(0),
    m_redoAction(0),
    m_copyAction(0),
    m_cutAction(0),
    m_pasteAction(0),
    m_selectAllAction(0),
    m_gotoAction(0),
    m_printAction(0),
    m_formatAction(0),
    m_visualizeWhitespaceAction(0),
    m_cleanWhitespaceAction(0),
    m_textWrappingAction(0),
    m_unCommentSelectionAction(0),
    m_unCollapseAllAction(0),
    m_collapseAction(0),
    m_expandAction(0),
    m_cutLineAction(0),
    m_deleteLineAction(0),
    m_selectEncodingAction(0),
    m_increaseFontSizeAction(0),
    m_decreaseFontSizeAction(0),
    m_gotoBlockStartAction(0),
    m_gotoBlockEndAction(0),
    m_gotoBlockStartWithSelectionAction(0),
    m_gotoBlockEndWithSelectionAction(0),
    m_selectBlockUpAction(0),
    m_selectBlockDownAction(0),
    m_moveLineUpAction(0),
    m_moveLineDownAction(0),
    m_copyLineUpAction(0),
    m_copyLineDownAction(0),
    m_optionalActions(optionalActions),
    m_currentEditor(0),
    m_initialized(false)
{
    m_contextId << Core::UniqueIDManager::instance()->uniqueIdentifier(context);

    connect(Core::ICore::instance()->editorManager(), SIGNAL(currentEditorChanged(Core::IEditor*)),
        this, SLOT(updateCurrentEditor(Core::IEditor*)));
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

    Core::ActionManager *am = Core::ICore::instance()->actionManager();

    Core::ActionContainer *medit = am->actionContainer(Core::Constants::M_EDIT);
    Core::ActionContainer *advancedMenu = am->actionContainer(Core::Constants::M_EDIT_ADVANCED);

    m_selectEncodingAction = new QAction(tr("Select Encoding..."), this);
    Core::Command *command = am->registerAction(m_selectEncodingAction, Constants::SELECT_ENCODING, m_contextId);
    connect(m_selectEncodingAction, SIGNAL(triggered()), this, SLOT(selectEncoding()));
    medit->addAction(command, Core::Constants::G_EDIT_OTHER);


    m_formatAction = new QAction(tr("Auto-&indent Selection"), this);
    command = am->registerAction(m_formatAction, TextEditor::Constants::AUTO_INDENT_SELECTION, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+I")));
    advancedMenu->addAction(command, Core::Constants::G_EDIT_FORMAT);
    connect(m_formatAction, SIGNAL(triggered(bool)), this, SLOT(formatAction()));

#ifdef Q_WS_MAC
    QString modifier = tr("Meta");
#else
    QString modifier = tr("Ctrl");
#endif

    m_rewrapParagraphAction = new QAction(tr("&Rewrap Paragraph"), this);
    command = am->registerAction(m_rewrapParagraphAction, TextEditor::Constants::REWRAP_PARAGRAPH, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("%1+E, R").arg(modifier)));
    advancedMenu->addAction(command, Core::Constants::G_EDIT_FORMAT);
    connect(m_rewrapParagraphAction, SIGNAL(triggered(bool)), this, SLOT(rewrapParagraphAction()));


    m_visualizeWhitespaceAction = new QAction(tr("&Visualize Whitespace"), this);
    m_visualizeWhitespaceAction->setCheckable(true);
    command = am->registerAction(m_visualizeWhitespaceAction,
                                 TextEditor::Constants::VISUALIZE_WHITESPACE, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("%1+E, %2+V").arg(modifier, modifier)));
    advancedMenu->addAction(command, Core::Constants::G_EDIT_FORMAT);
    connect(m_visualizeWhitespaceAction, SIGNAL(triggered(bool)), this, SLOT(setVisualizeWhitespace(bool)));

    m_cleanWhitespaceAction = new QAction(tr("Clean Whitespace"), this);
    command = am->registerAction(m_cleanWhitespaceAction,
                                 TextEditor::Constants::CLEAN_WHITESPACE, m_contextId);

    advancedMenu->addAction(command, Core::Constants::G_EDIT_FORMAT);
    connect(m_cleanWhitespaceAction, SIGNAL(triggered()), this, SLOT(cleanWhitespace()));

    m_textWrappingAction = new QAction(tr("Enable Text &Wrapping"), this);
    m_textWrappingAction->setCheckable(true);
    command = am->registerAction(m_textWrappingAction, TextEditor::Constants::TEXT_WRAPPING, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("%1+E, %2+W").arg(modifier, modifier)));
    advancedMenu->addAction(command, Core::Constants::G_EDIT_FORMAT);
    connect(m_textWrappingAction, SIGNAL(triggered(bool)), this, SLOT(setTextWrapping(bool)));


    m_unCommentSelectionAction = new QAction(tr("(Un)Comment &Selection"), this);
    command = am->registerAction(m_unCommentSelectionAction, Constants::UN_COMMENT_SELECTION, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+/")));
    connect(m_unCommentSelectionAction, SIGNAL(triggered()), this, SLOT(unCommentSelection()));
    advancedMenu->addAction(command, Core::Constants::G_EDIT_FORMAT);

    m_cutLineAction = new QAction(tr("Cut &Line"), this);
    command = am->registerAction(m_cutLineAction, Constants::CUT_LINE, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Shift+Del")));
    connect(m_cutLineAction, SIGNAL(triggered()), this, SLOT(cutLine()));

    m_deleteLineAction = new QAction(tr("Delete &Line"), this);
    command = am->registerAction(m_deleteLineAction, Constants::DELETE_LINE, m_contextId);
    connect(m_deleteLineAction, SIGNAL(triggered()), this, SLOT(deleteLine()));

    m_collapseAction = new QAction(tr("Collapse"), this);
    command = am->registerAction(m_collapseAction, Constants::COLLAPSE, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+<")));
    connect(m_collapseAction, SIGNAL(triggered()), this, SLOT(collapse()));
    advancedMenu->addAction(command, Core::Constants::G_EDIT_COLLAPSING);

    m_expandAction = new QAction(tr("Expand"), this);
    command = am->registerAction(m_expandAction, Constants::EXPAND, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+>")));
    connect(m_expandAction, SIGNAL(triggered()), this, SLOT(expand()));
    advancedMenu->addAction(command, Core::Constants::G_EDIT_COLLAPSING);

    m_unCollapseAllAction = new QAction(tr("(Un)&Collapse All"), this);
    command = am->registerAction(m_unCollapseAllAction, Constants::UN_COLLAPSE_ALL, m_contextId);
    connect(m_unCollapseAllAction, SIGNAL(triggered()), this, SLOT(unCollapseAll()));
    advancedMenu->addAction(command, Core::Constants::G_EDIT_COLLAPSING);

    m_increaseFontSizeAction = new QAction(tr("Increase Font Size"), this);
    command = am->registerAction(m_increaseFontSizeAction, Constants::INCREASE_FONT_SIZE, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl++")));
    connect(m_increaseFontSizeAction, SIGNAL(triggered()), this, SLOT(increaseFontSize()));
    advancedMenu->addAction(command, Core::Constants::G_EDIT_FONT);

    m_decreaseFontSizeAction = new QAction(tr("Decrease Font Size"), this);
    command = am->registerAction(m_decreaseFontSizeAction, Constants::DECREASE_FONT_SIZE, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+-")));
    connect(m_decreaseFontSizeAction, SIGNAL(triggered()), this, SLOT(decreaseFontSize()));
    advancedMenu->addAction(command, Core::Constants::G_EDIT_FONT);

    m_gotoBlockStartAction = new QAction(tr("Goto Block Start"), this);
    command = am->registerAction(m_gotoBlockStartAction, Constants::GOTO_BLOCK_START, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+[")));
    connect(m_gotoBlockStartAction, SIGNAL(triggered()), this, SLOT(gotoBlockStart()));
    advancedMenu->addAction(command, Core::Constants::G_EDIT_BLOCKS);

    m_gotoBlockEndAction = new QAction(tr("Goto Block End"), this);
    command = am->registerAction(m_gotoBlockEndAction, Constants::GOTO_BLOCK_END, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+]")));
    connect(m_gotoBlockEndAction, SIGNAL(triggered()), this, SLOT(gotoBlockEnd()));
    advancedMenu->addAction(command, Core::Constants::G_EDIT_BLOCKS);

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
    advancedMenu->addAction(command, Core::Constants::G_EDIT_BLOCKS);

    m_selectBlockDownAction= new QAction(tr("Select Block Down"), this);
    command = am->registerAction(m_selectBlockDownAction, Constants::SELECT_BLOCK_DOWN, m_contextId);
    connect(m_selectBlockDownAction, SIGNAL(triggered()), this, SLOT(selectBlockDown()));
    advancedMenu->addAction(command, Core::Constants::G_EDIT_BLOCKS);

    m_moveLineUpAction= new QAction(tr("Move Line Up"), this);
    command = am->registerAction(m_moveLineUpAction, Constants::MOVE_LINE_UP, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+Up")));
    connect(m_moveLineUpAction, SIGNAL(triggered()), this, SLOT(moveLineUp()));

    m_moveLineDownAction= new QAction(tr("Move Line Down"), this);
    command = am->registerAction(m_moveLineDownAction, Constants::MOVE_LINE_DOWN, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+Down")));
    connect(m_moveLineDownAction, SIGNAL(triggered()), this, SLOT(moveLineDown()));

    m_copyLineUpAction= new QAction(tr("Copy Line Up"), this);
    command = am->registerAction(m_copyLineUpAction, Constants::COPY_LINE_UP, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Up")));
    connect(m_copyLineUpAction, SIGNAL(triggered()), this, SLOT(copyLineUp()));

    m_copyLineDownAction= new QAction(tr("Copy Line Down"), this);
    command = am->registerAction(m_copyLineDownAction, Constants::COPY_LINE_DOWN, m_contextId);
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Down")));
    connect(m_copyLineDownAction, SIGNAL(triggered()), this, SLOT(copyLineDown()));
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
    Core::ICore::instance()->actionManager()->registerAction(result, id, m_contextId);
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
    Q_ASSERT(m_currentEditor != 0);
    return m_currentEditor->file()->isReadOnly() ? ReadOnlyMode : WriteMode;
}

void TextEditorActionHandler::updateActions()
{
    if (!m_currentEditor || !m_initialized)
        return;
    updateActions(updateMode());
}

void TextEditorActionHandler::updateActions(UpdateMode um)
{
    m_pasteAction->setEnabled(um != ReadOnlyMode);
    m_formatAction->setEnabled((m_optionalActions & Format) && um != ReadOnlyMode);
    m_unCommentSelectionAction->setEnabled((m_optionalActions & UnCommentSelection) && um != ReadOnlyMode);
    m_moveLineUpAction->setEnabled(um != ReadOnlyMode);
    m_moveLineDownAction->setEnabled(um != ReadOnlyMode);

    m_formatAction->setEnabled((m_optionalActions & Format));
    m_unCollapseAllAction->setEnabled((m_optionalActions & UnCollapseAll));
    m_visualizeWhitespaceAction->setChecked(m_currentEditor->displaySettings().m_visualizeWhitespace);
    if (m_textWrappingAction) {
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
    const bool hasCopyableText = m_currentEditor && m_currentEditor->textCursor().hasSelection();
    if (m_cutAction)
        m_cutAction->setEnabled(hasCopyableText && updateMode() == WriteMode);
    if (m_copyAction) {
        m_copyAction->setEnabled(hasCopyableText);
    }
}

void TextEditorActionHandler::gotoAction()
{
    Locator::LocatorManager *locatorManager = Locator::LocatorManager::instance();
    QTC_ASSERT(locatorManager, return);
    QString locatorString = TextEditorPlugin::instance()->lineNumberFilter()->shortcutString();
    locatorString += QLatin1Char(' ');
    const int selectionStart = locatorString.size();
    locatorString += tr("<line number>");
    locatorManager->show(locatorString, selectionStart, locatorString.size() - selectionStart);
}

void TextEditorActionHandler::printAction()
{
    if (m_currentEditor)
        m_currentEditor->print(Core::ICore::instance()->printer());
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


FUNCTION2(undoAction, undo)
FUNCTION2(redoAction, redo)
FUNCTION2(copyAction, copy)
FUNCTION2(cutAction, cut)
FUNCTION2(pasteAction, paste)
FUNCTION2(formatAction, format)
FUNCTION2(rewrapParagraphAction, rewrapParagraph)
FUNCTION2(selectAllAction, selectAll)
FUNCTION(cleanWhitespace)
FUNCTION(unCommentSelection)
FUNCTION(cutLine)
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
FUNCTION(moveLineUp)
FUNCTION(moveLineDown)
FUNCTION(copyLineUp)
FUNCTION(copyLineDown)

void TextEditorActionHandler::updateCurrentEditor(Core::IEditor *editor)
{
    m_currentEditor = 0;

    if (!editor)
        return;

    BaseTextEditor *baseEditor = qobject_cast<BaseTextEditor *>(editor->widget());

    if (baseEditor && baseEditor->actionHack() == this) {
        m_currentEditor = baseEditor;
        updateActions();
    }
}

const QPointer<BaseTextEditor> &TextEditorActionHandler::currentEditor() const
{
    return m_currentEditor;
}
