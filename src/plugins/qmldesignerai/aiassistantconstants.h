// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qstringfwd.h>

namespace QmlDesigner::Constants {

using namespace Qt::StringLiterals;
inline constexpr QByteArrayView aiAssistantSettingsKey = "AiAssistant"_L1;
inline constexpr QByteArrayView aiAssistantProviderConfigKey = "ProviderConfig"_L1;
inline constexpr QByteArrayView aiAssistantTermsAcceptedKey = "QML/Designer/AiTermsAccepted"_L1;

inline constexpr char aiAssistantSettingsPageCategory[] = "ZW.AiAssistant";
inline constexpr char aiAssistantProviderSettingsPage[] = "AiAssistant.A.ProviderSettings";

} // namespace QmlDesigner::Constants
