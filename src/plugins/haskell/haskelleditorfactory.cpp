/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "haskelleditorfactory.h"

#include "haskellconstants.h"
#include "haskellhighlighter.h"
#include "haskellmanager.h"

#include <coreplugin/actionmanager/commandbutton.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/textindenter.h>

#include <QCoreApplication>

namespace Haskell {
namespace Internal {

static QWidget *createEditorWidget()
{
    auto widget = new TextEditor::TextEditorWidget;
    auto ghciButton = new Core::CommandButton(Constants::A_RUN_GHCI, widget);
    ghciButton->setText(HaskellManager::tr("GHCi"));
    QObject::connect(ghciButton, &QToolButton::clicked, HaskellManager::instance(), [widget] {
        HaskellManager::openGhci(widget->textDocument()->filePath());
    });
    widget->insertExtraToolBarWidget(TextEditor::TextEditorWidget::Left, ghciButton);
    return widget;
}

HaskellEditorFactory::HaskellEditorFactory()
{
    setId(Constants::C_HASKELLEDITOR_ID);
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", "Haskell Editor"));
    addMimeType("text/x-haskell");
    setEditorActionHandlers(TextEditor::TextEditorActionHandler::UnCommentSelection
                            | TextEditor::TextEditorActionHandler::FollowSymbolUnderCursor);
    setDocumentCreator([] { return new TextEditor::TextDocument(Constants::C_HASKELLEDITOR_ID); });
    setIndenterCreator([](QTextDocument *doc) { return new TextEditor::TextIndenter(doc); });
    setEditorWidgetCreator(createEditorWidget);
    setCommentDefinition(Utils::CommentDefinition("--", "{-", "-}"));
    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setSyntaxHighlighterCreator([] { return new HaskellHighlighter(); });
}

} // Internal
} // Haskell
