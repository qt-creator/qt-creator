/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "pythoneditor.h"
#include "pythonconstants.h"
#include "pythonhighlighter.h"
#include "pythonindenter.h"
#include "pythonsettings.h"
#include "pythonutils.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/commandbutton.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditoractionhandler.h>

#include <QAction>
#include <QMenu>

namespace Python {
namespace Internal {

static QAction *createAction(QObject *parent, ReplType type)
{
    QAction *action = new QAction(parent);
    switch (type) {
    case ReplType::Unmodified:
        action->setText(QCoreApplication::translate("Python", "REPL"));
        action->setToolTip(QCoreApplication::translate("Python", "Open interactive Python."));
        break;
    case ReplType::Import:
        action->setText(QCoreApplication::translate("Python", "REPL Import File"));
        action->setToolTip(
            QCoreApplication::translate("Python", "Open interactive Python and import file."));
        break;
    case ReplType::ImportToplevel:
        action->setText(QCoreApplication::translate("Python", "REPL Import *"));
        action->setToolTip(
            QCoreApplication::translate("Python",
                                        "Open interactive Python and import * from file."));
        break;
    }

    QObject::connect(action, &QAction::triggered, parent, [type] {
        Core::IDocument *doc = Core::EditorManager::currentDocument();
        openPythonRepl(doc ? doc->filePath() : Utils::FilePath(), type);
    });
    return action;
}

static void registerReplAction(QObject *parent)
{
    Core::ActionManager::registerAction(createAction(parent, ReplType::Unmodified),
                                        Constants::PYTHON_OPEN_REPL);
    Core::ActionManager::registerAction(createAction(parent, ReplType::Import),
                                        Constants::PYTHON_OPEN_REPL_IMPORT);
    Core::ActionManager::registerAction(createAction(parent, ReplType::ImportToplevel),
                                        Constants::PYTHON_OPEN_REPL_IMPORT_TOPLEVEL);
}

static QWidget *createEditorWidget()
{
    auto widget = new TextEditor::TextEditorWidget;
    auto replButton = new QToolButton(widget);
    replButton->setProperty("noArrow", true);
    replButton->setText(QCoreApplication::translate("Python", "REPL"));
    replButton->setPopupMode(QToolButton::InstantPopup);
    auto menu = new QMenu(replButton);
    replButton->setMenu(menu);
    menu->addAction(Core::ActionManager::command(Constants::PYTHON_OPEN_REPL)->action());
    menu->addSeparator();
    menu->addAction(Core::ActionManager::command(Constants::PYTHON_OPEN_REPL_IMPORT)->action());
    menu->addAction(
        Core::ActionManager::command(Constants::PYTHON_OPEN_REPL_IMPORT_TOPLEVEL)->action());
    widget->insertExtraToolBarWidget(TextEditor::TextEditorWidget::Left, replButton);
    return widget;
}

PythonEditorFactory::PythonEditorFactory()
{
    registerReplAction(this);

    setId(Constants::C_PYTHONEDITOR_ID);
    setDisplayName(
        QCoreApplication::translate("OpenWith::Editors", Constants::C_EDITOR_DISPLAY_NAME));
    addMimeType(Constants::C_PY_MIMETYPE);

    setEditorActionHandlers(TextEditor::TextEditorActionHandler::Format
                            | TextEditor::TextEditorActionHandler::UnCommentSelection
                            | TextEditor::TextEditorActionHandler::UnCollapseAll
                            | TextEditor::TextEditorActionHandler::FollowSymbolUnderCursor);

    setDocumentCreator([] { return new TextEditor::TextDocument(Constants::C_PYTHONEDITOR_ID); });
    setEditorWidgetCreator(createEditorWidget);
    setIndenterCreator([](QTextDocument *doc) { return new PythonIndenter(doc); });
    setSyntaxHighlighterCreator([] { return new PythonHighlighter; });
    setCommentDefinition(Utils::CommentDefinition::HashStyle);
    setParenthesesMatchingEnabled(true);
    setCodeFoldingSupported(true);
}

} // namespace Internal
} // namespace Python
