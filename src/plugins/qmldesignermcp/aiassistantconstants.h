// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qstringfwd.h>

#include <QSet>

namespace QmlDesigner::Constants {

using namespace Qt::StringLiterals;
inline constexpr QByteArrayView aiAssistantProviderKey = "AiAssistant/Provider"_L1;
inline constexpr QByteArrayView aiAssistantTermsAcceptedKey = "AiAssistant/TermsAccepted"_L1;
inline constexpr QByteArrayView aiAssistantSelectedModelKey = "AiAssistant/SelectedModel"_L1;

inline constexpr char aiAssistantSettingsPageCategory[] = "ZW.AiAssistant";
inline constexpr char aiAssistantSettingsPageId[] = "AiAssistant.A.ProviderSettings";
inline constexpr char structureResourceUri[] = "qmlproject://structure";
inline constexpr char qmlServerName[] = "qml_server"; // qml MCP server name

// Tools provided by qml_server that change files.
inline const QSet<QString> qmlStructureMutatingTools = {
    "create_qml",
    "modify_qml",
    "delete_qml",
    "move_qml",
};

// Tools provided by qml_server that require confirmation.
inline const QSet<QString> toolsRequiringConfirmation = {
    "delete_qml"
};

} // namespace QmlDesigner::Constants
