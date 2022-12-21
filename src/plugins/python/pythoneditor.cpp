// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythoneditor.h"

#include "pyside.h"
#include "pythonconstants.h"
#include "pythonhighlighter.h"
#include "pythonindenter.h"
#include "pythonlanguageclient.h"
#include "pythonsettings.h"
#include "pythontr.h"
#include "pythonutils.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/commandbutton.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditoractionhandler.h>

#include <QAction>
#include <QMenu>

using namespace Utils;

namespace Python::Internal {

static QAction *createAction(QObject *parent, ReplType type)
{
    QAction *action = new QAction(parent);
    switch (type) {
    case ReplType::Unmodified:
        action->setText(Tr::tr("REPL"));
        action->setToolTip(Tr::tr("Open interactive Python."));
        break;
    case ReplType::Import:
        action->setText(Tr::tr("REPL Import File"));
        action->setToolTip(Tr::tr("Open interactive Python and import file."));
        break;
    case ReplType::ImportToplevel:
        action->setText(Tr::tr("REPL Import *"));
        action->setToolTip(Tr::tr("Open interactive Python and import * from file."));
        break;
    }

    QObject::connect(action, &QAction::triggered, parent, [type, parent] {
        Core::IDocument *doc = Core::EditorManager::currentDocument();
        openPythonRepl(parent, doc ? doc->filePath() : FilePath(), type);
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
    replButton->setText(Tr::tr("REPL"));
    replButton->setPopupMode(QToolButton::InstantPopup);
    replButton->setToolTip(Tr::tr("Open interactive Python. Either importing nothing, "
          "importing the current file, or importing everything (*) from the current file."));
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

class PythonDocument : public TextEditor::TextDocument
{
public:
    PythonDocument() : TextEditor::TextDocument(Constants::C_PYTHONEDITOR_ID)
    {
        connect(PythonSettings::instance(),
                &PythonSettings::pylsEnabledChanged,
                this,
                [this](const bool enabled) {
                    if (!enabled)
                        return;
                    const FilePath &python = detectPython(filePath());
                    if (python.exists())
                        PyLSConfigureAssistant::openDocumentWithPython(python, this);
                });
        connect(this, &PythonDocument::openFinishedSuccessfully,
                this, &PythonDocument::checkForPyls);
    }

    void checkForPyls()
    {
        const FilePath &python = detectPython(filePath());
        if (!python.exists())
            return;

        PyLSConfigureAssistant::openDocumentWithPython(python, this);
        PySideInstaller::checkPySideInstallation(python, this);
    }
};

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

    setDocumentCreator([]() { return new PythonDocument; });
    setEditorWidgetCreator(createEditorWidget);
    setIndenterCreator([](QTextDocument *doc) { return new PythonIndenter(doc); });
    setSyntaxHighlighterCreator([] { return new PythonHighlighter; });
    setCommentDefinition(CommentDefinition::HashStyle);
    setParenthesesMatchingEnabled(true);
    setCodeFoldingSupported(true);
}

} // Python::Internal
