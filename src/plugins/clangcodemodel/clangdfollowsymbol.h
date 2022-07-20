/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

class ClangdFollowSymbol : public QObject
{
    Q_OBJECT
public:
    ClangdFollowSymbol(ClangdClient *client, const QTextCursor &cursor,
                       CppEditor::CppEditorWidget *editorWidget,
                       TextEditor::TextDocument *document, const Utils::LinkHandler &callback,
                       bool openInSplit);
    ~ClangdFollowSymbol();
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
