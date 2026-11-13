// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <functional>

namespace AcpClient::Internal {

class AcpChatTab;
class AcpInspector;

void setupAcpChatEditor(AcpInspector *inspector,
                        const std::function<void(AcpChatTab *)> &moveToSidePanel);
bool openChatInEditor(AcpChatTab *tab);
void closeAllChatEditors();

} // namespace AcpClient::Internal
