// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jdwpimpl.h"

#include "../breakpoint.h"
#include "../debuggerengine.h"
#include "../genericdebuggerengine.h"

#include <utils/algorithm.h>

using namespace Utils;

namespace Debugger::Internal {

DebuggerEngine *createJdwpEngine(const DebuggerRunParameters &rp)
{
    JdwpImplStartData startData;
    if (!rp.javaDebugChannel().isEmpty())
        startData.inferiorStartData = AttachToRemoteServerData{rp.javaDebugChannel(), {}};
    else if (rp.startMode() == AttachToRemoteServer)
        startData.inferiorStartData = AttachToRemoteServerData{rp.remoteChannel(), {}};
    else
        startData.inferiorStartData = rp.inferior();
    if (!rp.projectSourceDirectory().isEmpty())
        startData.sourceSearchPaths.append(rp.projectSourceDirectory());
    // The Java a library brings is built into the application and stepped into
    // like the Java of the project itself, while it lies where the library is:
    // what the run was set up with knows about such a directory, and the files
    // of the project do not.
    startData.sourceSearchPaths += rp.additionalSearchDirectories();
    startData.sourceFiles = Utils::filtered(rp.projectSourceFiles(), isJvmSource);
    return new GenericDebuggerEngine("JDWP", new JdwpImpl(startData));
}

} // namespace Debugger::Internal
