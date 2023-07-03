// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/link.h>

#include <QObject>

namespace CppEditor { class CppEditorWidget; }
namespace TextEditor { class TextDocument; }

QT_BEGIN_NAMESPACE
class QTextCursor;
QT_END_NAMESPACE

namespace ClangCodeModel::Internal {
class ClangdAstNode;
class ClangdClient;
enum class FollowTo;

class ClangdFollowSymbol : public QObject
{
    Q_OBJECT
public:
    ClangdFollowSymbol(ClangdClient *client, const QTextCursor &cursor,
                       CppEditor::CppEditorWidget *editorWidget,
                       TextEditor::TextDocument *document, const Utils::LinkHandler &callback,
                       FollowTo followTo, bool openInSplit);
    ~ClangdFollowSymbol();
    void cancel();
    void clear();

signals:
    void done();

private:
    void emitDone(const Utils::Link &link = {});
    class VirtualFunctionAssistProcessor;
    class VirtualFunctionAssistProvider;

    class Private;
    Private * const d;
};

} // namespace ClangCodeModel::Internal
