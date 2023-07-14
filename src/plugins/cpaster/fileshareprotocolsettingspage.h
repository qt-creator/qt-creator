// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace CodePaster {

class FileShareProtocolSettings final : public Utils::AspectContainer
{
public:
    FileShareProtocolSettings();

    Utils::FilePathAspect path{this};
    Utils::IntegerAspect displayCount{this};
};

FileShareProtocolSettings &fileShareSettings();

Core::IOptionsPage &fileShareSettingsPage();

} // CodePaster
