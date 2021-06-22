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

#include "clangdquickfixfactory.h"

#include "clangdclient.h"
#include "clangmodelmanagersupport.h"

#include <languageclient/languageclientquickfix.h>

using namespace LanguageServerProtocol;

namespace ClangCodeModel {
namespace Internal {

ClangdQuickFixFactory::ClangdQuickFixFactory() = default;

void ClangdQuickFixFactory::match(const CppEditor::Internal::CppQuickFixInterface &interface,
                                  QuickFixOperations &result)
{
    const auto client = ClangModelManagerSupport::instance()->clientForFile(interface.filePath());
    if (!client)
        return;

    const auto uri = DocumentUri::fromFilePath(interface.filePath());
    QTextCursor cursor(interface.textDocument());
    cursor.setPosition(interface.position());
    cursor.select(QTextCursor::LineUnderCursor);
    const QList<Diagnostic> &diagnostics = client->diagnosticsAt(uri, cursor);
    for (const Diagnostic &diagnostic : diagnostics) {
        ClangdDiagnostic clangdDiagnostic(diagnostic);
        if (const auto actions = clangdDiagnostic.codeActions()) {
            for (const CodeAction &action : *actions)
                result << new LanguageClient::CodeActionQuickFixOperation(action, client);
        }
    }
}

} // namespace Internal
} // namespace ClangCodeModel
