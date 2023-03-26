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
class ClangdClient;

class ClangdSwitchDeclDef : public QObject
{
    Q_OBJECT
public:
    ClangdSwitchDeclDef(ClangdClient *client, TextEditor::TextDocument *doc,
                        const QTextCursor &cursor, CppEditor::CppEditorWidget *editorWidget,
                        const Utils::LinkHandler &callback);
    ~ClangdSwitchDeclDef();

signals:
    void done();

private:
    void emitDone();
    class Private;
    Private * const d;
};

} // namespace ClangCodeModel::Internal
