// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppmodelmanagersupport.h"

#include <QScopedPointer>

namespace CppEditor { class FollowSymbolUnderCursor; }

namespace CppEditor::Internal {

class BuiltinModelManagerSupport: public ModelManagerSupport
{
    Q_DISABLE_COPY(BuiltinModelManagerSupport)

public:
    BuiltinModelManagerSupport();
    ~BuiltinModelManagerSupport() override;

    CppCompletionAssistProvider *completionAssistProvider();
    TextEditor::BaseHoverHandler *createHoverHandler();

    BaseEditorDocumentProcessor *createEditorDocumentProcessor(
            TextEditor::TextDocument *baseTextDocument) final;

    FollowSymbolUnderCursor &followSymbolInterface() { return *m_followSymbol; }

private:
    void followSymbol(const CursorInEditor &data, const Utils::LinkHandler &processLinkCallback,
                      bool resolveTarget, bool inNextSplit) override;
    void followSymbolToType(const CursorInEditor &data,
                            const Utils::LinkHandler &processLinkCallback,
                            bool inNextSplit) override;
    void switchDeclDef(const CursorInEditor &data,
                       const Utils::LinkHandler &processLinkCallback) override;
    void startLocalRenaming(const CursorInEditor &data,
                            const ProjectPart *projectPart,
                            RenameCallback &&renameSymbolsCallback) override;
    void globalRename(const CursorInEditor &data, const QString &replacement,
                      const std::function<void()> &callback) override;
    void findUsages(const CursorInEditor &data) const override;
    void switchHeaderSource(const Utils::FilePath &filePath, bool inNextSplit) override;
    void checkUnused(const Utils::Link &link, Core::SearchResult *search,
                     const Utils::LinkHandler &callback) override;

    QScopedPointer<CppCompletionAssistProvider> m_completionAssistProvider;
    QScopedPointer<FollowSymbolUnderCursor> m_followSymbol;
};

} // namespace CppEditor::Internal
