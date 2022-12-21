// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Debugger::Internal {

class CommonOptionsPage final : public Core::IOptionsPage
{
public:
    CommonOptionsPage();

    static QString msgSetBreakpointAtFunction(const char *function);
    static QString msgSetBreakpointAtFunctionToolTip(const char *function,
                                                     const QString &hint = {});
};

class LocalsAndExpressionsOptionsPage final : public Core::IOptionsPage
{
public:
    LocalsAndExpressionsOptionsPage();
};

} // Debugger::Internal
