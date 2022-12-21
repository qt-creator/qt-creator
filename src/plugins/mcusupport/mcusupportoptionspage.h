// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "settingshandler.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace McuSupport {
namespace Internal {

class McuSupportOptions;

class McuSupportOptionsPage final : public Core::IOptionsPage
{
public:
    McuSupportOptionsPage(McuSupportOptions &, const SettingsHandler::Ptr &);
};

} // namespace Internal
} // namespace McuSupport
