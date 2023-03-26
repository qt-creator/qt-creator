// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace ProjectExplorer {
namespace Internal {

class SshSettingsPage final : public Core::IOptionsPage
{
public:
    SshSettingsPage();
};

} // namespace Internal
} // namespace ProjectExplorer
