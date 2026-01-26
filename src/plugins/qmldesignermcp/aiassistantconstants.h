// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qstringfwd.h>

namespace QmlDesigner::Constants {

using namespace Qt::StringLiterals;
inline constexpr QByteArrayView aiAssistantProviderKey = "AiAssistant/Provider"_L1;
inline constexpr QByteArrayView aiAssistantTermsAcceptedKey = "AiAssistant/TermsAccepted"_L1;
inline constexpr QByteArrayView aiAssistantSelectedModelKey = "AiAssistant/SelectedModel"_L1;

inline constexpr char aiAssistantSettingsPageCategory[] = "ZW.AiAssistant";
inline constexpr char aiAssistantSettingsPageId[] = "AiAssistant.A.ProviderSettings";

} // namespace QmlDesigner::Constants
