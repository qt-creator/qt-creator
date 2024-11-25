// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Gerrit::Internal {

class GerritOptionsPage : public Core::IOptionsPage
{
public:
    explicit GerritOptionsPage(const std::function<void()> &onChanged);
};

} // Gerrit::Internal
