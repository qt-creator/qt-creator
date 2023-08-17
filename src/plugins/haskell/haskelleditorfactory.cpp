// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "haskelleditorfactory.h"

#include "haskellconstants.h"
#include "haskellhighlighter.h"
#include "haskellmanager.h"
#include "haskelltr.h"

#include <coreplugin/actionmanager/commandbutton.h>
#include <coreplugin/coreplugintr.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/textindenter.h>

namespace Haskell::Internal {

static QWidget *createEditorWidget()
{
    auto widget = new TextEditor::TextEditorWidget;
    auto ghciButton = new Core::CommandButton(Constants::A_RUN_GHCI, widget);
    ghciButton->setText(Tr::tr("GHCi"));
    QObject::connect(ghciButton, &QToolButton::clicked, widget, [widget] {
        HaskellManager::openGhci(widget->textDocument()->filePath());
    });
    widget->insertExtraToolBarWidget(TextEditor::TextEditorWidget::Left, ghciButton);
    return widget;
}

HaskellEditorFactory::HaskellEditorFactory()
{
    setId(Constants::C_HASKELLEDITOR_ID);
    setDisplayName(::Core::Tr::tr("Haskell Editor"));
    addMimeType("text/x-haskell");
    setEditorActionHandlers(TextEditor::TextEditorActionHandler::UnCommentSelection
                            | TextEditor::TextEditorActionHandler::FollowSymbolUnderCursor);
    setDocumentCreator([] { return new TextEditor::TextDocument(Constants::C_HASKELLEDITOR_ID); });
    setIndenterCreator([](QTextDocument *doc) { return new TextEditor::TextIndenter(doc); });
    setEditorWidgetCreator(&createEditorWidget);
    setCommentDefinition(Utils::CommentDefinition("--", "{-", "-}"));
    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setSyntaxHighlighterCreator([] { return new HaskellHighlighter(); });
}

} // Haskell::Internal
