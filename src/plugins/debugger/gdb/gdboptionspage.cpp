// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <debugger/debuggeractions.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggertr.h>

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/layoutbuilder.h>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {

/////////////////////////////////////////////////////////////////////////
//
// GdbOptionsPage - harmless options
//
/////////////////////////////////////////////////////////////////////////

class GdbOptionsPage : public Core::IOptionsPage
{
public:
    GdbOptionsPage()
    {
        setId("M.Gdb");
        setDisplayName(Tr::tr("GDB"));
        setCategory(Constants::DEBUGGER_SETTINGS_CATEGORY);
        setSettings(&debuggerSettings()->page2);

        setLayouter([](QWidget *w) {
            using namespace Layouting;
            DebuggerSettings &s = *debuggerSettings();

            auto labelDangerous = new QLabel("<html><head/><body><i>" +
                Tr::tr("The options below give access to advanced<br>"
                       "or experimental functions of GDB.<p>"
                       "Enabling them may negatively impact<br>"
                       "your debugging experience.") + "</i></body></html>");

            Group general {
                title(Tr::tr("General")),
                Column {
                    Row { s.gdbWatchdogTimeout, st },
                    s.skipKnownFrames,
                    s.useMessageBoxForSignals,
                    s.adjustBreakpointLocations,
                    s.useDynamicType,
                    s.loadGdbInit,
                    s.loadGdbDumpers,
                    s.intelFlavor,
                    s.usePseudoTracepoints,
                    s.useIndexCache,
                    st
                }
            };

            Group extended {
                title(Tr::tr("Extended")),
                Column {
                    labelDangerous,
                    s.targetAsync,
                    s.autoEnrichParameters,
                    s.breakOnWarning,
                    s.breakOnFatal,
                    s.breakOnAbort,
                    s.enableReverseDebugging,
                    s.multiInferior,
                    st
                }
            };

            Group startup {
                title(Tr::tr("Additional Startup Commands")),
                Column { s.gdbStartupCommands }
            };

            Group attach {
                title(Tr::tr("Additional Attach Commands")),
                Column { s.gdbPostAttachCommands },
            };

            Grid { general, extended, br, startup, attach }.attachTo(w);
        });
    }
};

// Registration

void addGdbOptionPages(QList<IOptionsPage *> *opts)
{
    opts->push_back(new GdbOptionsPage);
}

} // Debugger::Internal
