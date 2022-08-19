// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

namespace Valgrind {
namespace Internal {

class ValgrindGlobalSettings;

class CallgrindTool final
{
public:
    CallgrindTool();
    ~CallgrindTool();
};

} // namespace Internal
} // namespace Valgrind
