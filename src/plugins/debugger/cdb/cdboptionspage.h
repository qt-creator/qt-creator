// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Debugger {
namespace Internal {

class CdbOptionsPage final : public Core::IOptionsPage
{
public:
    CdbOptionsPage();
};

class CdbPathsPage final : public Core::IOptionsPage
{
public:
    CdbPathsPage();
};

} // namespace Internal
} // namespace Debugger
