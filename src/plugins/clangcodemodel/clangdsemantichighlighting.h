// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QLoggingCategory>
#include <QPointer>
#include <QVersionNumber>

QT_BEGIN_NAMESPACE
template <typename T>
class QPromise;
QT_END_NAMESPACE

namespace LanguageClient {
class Client;
class ExpandedSemanticToken;
}
namespace LanguageServerProtocol { class JsonRpcMessage; }
namespace TextEditor {
class HighlightingResult;
class TextDocument;
}
namespace Utils { class FilePath; }

namespace ClangCodeModel::Internal {
class ClangdAstNode;
class TaskTimer;
Q_DECLARE_LOGGING_CATEGORY(clangdLogHighlight);

void doSemanticHighlighting(
        QPromise<TextEditor::HighlightingResult> &promise,
        const Utils::FilePath &filePath,
        const QList<LanguageClient::ExpandedSemanticToken> &tokens,
        const QString &docContents,
        const ClangdAstNode &ast,
        const QPointer<TextEditor::TextDocument> &textDocument,
        int docRevision,
        const QVersionNumber &clangdVersion,
        const TaskTimer &taskTimer
        );


QString inactiveRegionsMethodName();
void handleInactiveRegions(LanguageClient::Client *client,
                           const LanguageServerProtocol::JsonRpcMessage &msg);

} // namespace ClangCodeModel::Internal
