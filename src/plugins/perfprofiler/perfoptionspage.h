// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace PerfProfiler {

class PerfSettings;

namespace Internal {

class PerfOptionsPage final : public Core::IOptionsPage
{
public:
    explicit PerfOptionsPage(PerfSettings *settings);
};

} // namespace Internal
} // namespace PerfProfiler
