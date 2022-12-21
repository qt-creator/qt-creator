// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace QmlDesigner {
class ExternalDependencies;

namespace Internal {

class SettingsPage final : public Core::IOptionsPage
{
public:
    SettingsPage(ExternalDependencies &externalDependencies);
};

} // namespace Internal
} // namespace QmlDesigner
