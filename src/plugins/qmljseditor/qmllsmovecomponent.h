// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <languageserverprotocol/lsptypes.h>

#include <texteditor/quickfix.h>

namespace LanguageClient { class Client; }
namespace Utils { class FilePath; }

namespace QmlJSEditor::Internal {

bool isMoveComponentAction(const LanguageServerProtocol::CodeAction &action);

TextEditor::QuickFixOperation *createMoveComponentOperation(
    const LanguageServerProtocol::CodeAction &action,
    LanguageClient::Client *client,
    const Utils::FilePath &filePath);

#ifdef WITH_TESTS
QObject *createQmllsMoveComponentTest();
#endif

} // namespace QmlJSEditor::Internal
