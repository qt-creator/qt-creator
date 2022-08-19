// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace CodePaster {

class FileShareProtocolSettings : public Utils::AspectContainer
{
    Q_DECLARE_TR_FUNCTIONS(CodePaster::FileShareProtocolSettings)

public:
    FileShareProtocolSettings();

    Utils::StringAspect path;
    Utils::IntegerAspect displayCount;
};

class FileShareProtocolSettingsPage final : public Core::IOptionsPage
{
public:
    explicit FileShareProtocolSettingsPage(FileShareProtocolSettings *settings);
};

} // namespace CodePaster
