// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "cursorineditor.h"

#include <utils/link.h>

#include <QSharedPointer>
#include <QString>

#include <functional>
#include <memory>

namespace Core { class SearchResult; }
namespace TextEditor {
class TextDocument;
class BaseHoverHandler;
} // namespace TextEditor

namespace CppEditor {

class BaseEditorDocumentProcessor;
class CppCompletionAssistProvider;
class ProjectPart;
class RefactoringEngineInterface;

class CPPEDITOR_EXPORT ModelManagerSupport
{
public:
    using Ptr = QSharedPointer<ModelManagerSupport>;

public:
    virtual ~ModelManagerSupport() = 0;

    virtual BaseEditorDocumentProcessor *createEditorDocumentProcessor(
                TextEditor::TextDocument *baseTextDocument) = 0;
    virtual bool usesClangd(const TextEditor::TextDocument *) const { return false; }

    virtual void followSymbol(const CursorInEditor &data,
                              const Utils::LinkHandler &processLinkCallback,
                              bool resolveTarget, bool inNextSplit) = 0;
    virtual void followSymbolToType(const CursorInEditor &data,
                                   const Utils::LinkHandler &processLinkCallback,
                                   bool inNextSplit) = 0;
    virtual void switchDeclDef(const CursorInEditor &data,
                               const Utils::LinkHandler &processLinkCallback) = 0;
    virtual void startLocalRenaming(const CursorInEditor &data,
                                    const ProjectPart *projectPart,
                                    RenameCallback &&renameSymbolsCallback) = 0;
    virtual void globalRename(const CursorInEditor &data, const QString &replacement,
                              const std::function<void()> &callback) = 0;
    virtual void findUsages(const CursorInEditor &data) const = 0;
    virtual void switchHeaderSource(const Utils::FilePath &filePath, bool inNextSplit) = 0;

    virtual void checkUnused(const Utils::Link &link, Core::SearchResult *search,
                             const Utils::LinkHandler &callback) = 0;
};

} // CppEditor namespace
