// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "terminalpane.h"

#include <utils/terminalinterface.h>

namespace Terminal {

class TerminalProcessImpl : public Utils::TerminalInterface
{
public:
    TerminalProcessImpl(TerminalPane *terminalPane);
};

} // namespace Terminal
