// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Debugger::Internal {

class GdbSettings : public Utils::AspectContainer
{
public:
    GdbSettings();

    Utils::IntegerAspect gdbWatchdogTimeout{this};
    Utils::BoolAspect skipKnownFrames{this};
    Utils::BoolAspect useMessageBoxForSignals{this};
    Utils::BoolAspect adjustBreakpointLocations{this};
    Utils::BoolAspect useDynamicType{this};
    Utils::BoolAspect loadGdbInit{this};
    Utils::BoolAspect loadGdbDumpers{this};
    Utils::BoolAspect intelFlavor{this};
    Utils::BoolAspect usePseudoTracepoints{this};
    Utils::BoolAspect useIndexCache{this};
    Utils::StringAspect gdbStartupCommands{this};
    Utils::StringAspect gdbPostAttachCommands{this};

    Utils::BoolAspect targetAsync{this};
    Utils::BoolAspect autoEnrichParameters{this};
    Utils::BoolAspect breakOnThrow{this};
    Utils::BoolAspect breakOnCatch{this};
    Utils::BoolAspect breakOnWarning{this};
    Utils::BoolAspect breakOnFatal{this};
    Utils::BoolAspect breakOnAbort{this};
    Utils::BoolAspect enableReverseDebugging{this};
    Utils::BoolAspect multiInferior{this};
};

GdbSettings &gdbSettings();

} // Debugger::Internal
