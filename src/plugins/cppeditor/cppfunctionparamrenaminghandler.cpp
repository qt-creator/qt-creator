// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppfunctionparamrenaminghandler.h"

#include "cppeditorwidget.h"
#include "cpplocalrenaming.h"
#include "cppfunctiondecldeflink.h"
#include "cppsemanticinfo.h"

#include <cplusplus/AST.h>
#include <cplusplus/ASTPath.h>
#include <texteditor/textdocument.h>

#include <memory>

using namespace CPlusPlus;

namespace CppEditor::Internal {

using DeclDefLinkPtr = std::shared_ptr<FunctionDeclDefLink>;

class CppFunctionParamRenamingHandler::Private
{
public:
    Private(CppEditorWidget &editorWidget, CppLocalRenaming &localRenaming);

    void handleRenamingStarted();
    void handleRenamingFinished();
    void handleLinkFound(const DeclDefLinkPtr &link);
    void findLink(FunctionDefinitionAST &func, const SemanticInfo &semanticInfo);

    CppEditorWidget &editorWidget;
    CppLocalRenaming &localRenaming;
    std::unique_ptr<FunctionDeclDefLinkFinder> linkFinder;
    DeclDefLinkPtr link;
};

CppFunctionParamRenamingHandler::CppFunctionParamRenamingHandler(
    CppEditorWidget &editorWidget, CppLocalRenaming &localRenaming)
    : d(new Private(editorWidget, localRenaming)) {}

CppFunctionParamRenamingHandler::~CppFunctionParamRenamingHandler() { delete d; }

CppFunctionParamRenamingHandler::Private::Private(
    CppEditorWidget &editorWidget, CppLocalRenaming &localRenaming)
    : editorWidget(editorWidget), localRenaming(localRenaming)
{
    QObject::connect(&localRenaming, &CppLocalRenaming::started,
                     &editorWidget, [this] { handleRenamingStarted(); });
    QObject::connect(&localRenaming, &CppLocalRenaming::finished,
                     &editorWidget, [this] { handleRenamingFinished(); });
}

void CppFunctionParamRenamingHandler::Private::handleRenamingStarted()
{
    linkFinder.reset();
    link.reset();

    // Are we currently on the function signature? In this case, the normal decl/def link
    // mechanism kicks in and we don't have to do anything.
    if (editorWidget.declDefLink())
        return;

    // If we find a surrounding function definition, start up the decl/def link finder.
    const SemanticInfo semanticInfo = editorWidget.semanticInfo();
    if (!semanticInfo.doc || !semanticInfo.doc->translationUnit())
        return;
    const QList<AST *> astPath = ASTPath(semanticInfo.doc)(editorWidget.textCursor());
    for (auto it = astPath.rbegin(); it != astPath.rend(); ++it) {
        if (const auto func = (*it)->asFunctionDefinition()) {
            findLink(*func, semanticInfo);
            return;
        }
    }
}

void CppFunctionParamRenamingHandler::Private::handleRenamingFinished()
{
    if (link) {
        link->apply(&editorWidget, false);
        link.reset();
    }
}

void CppFunctionParamRenamingHandler::Private::handleLinkFound(const DeclDefLinkPtr &link)
{
    if (localRenaming.isActive())
        this->link = link;
    linkFinder.reset();
}

void CppFunctionParamRenamingHandler::Private::findLink(FunctionDefinitionAST &func,
                                                        const SemanticInfo &semanticInfo)
{
    if (!func.declarator)
        return;

    // The finder needs a cursor that points to the signature, so provide one.
    QTextDocument * const doc = editorWidget.textDocument()->document();
    const int pos = semanticInfo.doc->translationUnit()->getTokenEndPositionInDocument(
        func.declarator->firstToken(), doc);
    QTextCursor cursor(doc);
    cursor.setPosition(pos);
    linkFinder.reset(new FunctionDeclDefLinkFinder);
    QObject::connect(linkFinder.get(), &FunctionDeclDefLinkFinder::foundLink,
            &editorWidget, [this](const DeclDefLinkPtr &link) {
        handleLinkFound(link);
    });
    linkFinder->startFindLinkAt(cursor, semanticInfo.doc, semanticInfo.snapshot);
}

} // namespace CppEditor::Internal
