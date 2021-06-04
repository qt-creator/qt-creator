/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#pragma once

#include <cpptools/refactoringengineinterface.h>
#include <languageclient/client.h>
#include <utils/link.h>
#include <utils/optional.h>

#include <QVersionNumber>

namespace Core { class SearchResultItem; }
namespace CppTools { class CppEditorWidgetInterface; }
namespace ProjectExplorer { class Project; }
namespace TextEditor { class TextDocument; }

namespace ClangCodeModel {
namespace Internal {

class ClangdClient : public LanguageClient::Client
{
    Q_OBJECT
public:
    ClangdClient(ProjectExplorer::Project *project, const Utils::FilePath &jsonDbDir);
    ~ClangdClient() override;

    bool isFullyIndexed() const;
    QVersionNumber versionNumber() const;

    void openExtraFile(const Utils::FilePath &filePath, const QString &content = {});
    void closeExtraFile(const Utils::FilePath &filePath);

    void findUsages(TextEditor::TextDocument *document, const QTextCursor &cursor,
                    const Utils::optional<QString> &replacement);
    void followSymbol(TextEditor::TextDocument *document,
            const QTextCursor &cursor,
            CppTools::CppEditorWidgetInterface *editorWidget,
            Utils::ProcessLinkCallback &&callback,
            bool resolveTarget,
            bool openInSplit);

    void switchDeclDef(TextEditor::TextDocument *document,
            const QTextCursor &cursor,
            CppTools::CppEditorWidgetInterface *editorWidget,
            Utils::ProcessLinkCallback &&callback);

    void findLocalUsages(TextEditor::TextDocument *document, const QTextCursor &cursor,
                         CppTools::RefactoringEngineInterface::RenameCallback &&callback);

    void gatherHelpItemForTooltip(
            const LanguageServerProtocol::HoverRequest::Response &hoverResponse,
            const LanguageServerProtocol::DocumentUri &uri);

    void enableTesting();

signals:
    void indexingFinished();
    void foundReferences(const QList<Core::SearchResultItem> &items);
    void findUsagesDone();
    void helpItemGathered(const Core::HelpItem &helpItem);

private:
    void handleDiagnostics(const LanguageServerProtocol::PublishDiagnosticsParams &params) override;

    class Private;
    class FollowSymbolData;
    class VirtualFunctionAssistProcessor;
    class VirtualFunctionAssistProvider;
    Private * const d;
};

class ClangdDiagnostic : public LanguageServerProtocol::Diagnostic
{
public:
    using Diagnostic::Diagnostic;
    Utils::optional<QList<LanguageServerProtocol::CodeAction>> codeActions() const;
    QString category() const;
};

} // namespace Internal
} // namespace ClangCodeModel
