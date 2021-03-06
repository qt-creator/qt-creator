/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggerconstants.h>

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

using namespace Core;
using namespace Utils;

namespace Debugger {
namespace Internal {

/////////////////////////////////////////////////////////////////////////
//
// GdbOptionsPageWidget - harmless options
//
/////////////////////////////////////////////////////////////////////////

class GdbOptionsPage : public Core::IOptionsPage
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::GdbOptionsPage)

public:
    GdbOptionsPage();
};

class GdbOptionsPageWidget : public IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::GdbOptionsPage)

public:
    GdbOptionsPageWidget();

    void apply() final { group.apply(); group.writeSettings(ICore::settings()); }
    void finish() final { group.finish(); }

    AspectContainer &group = debuggerSettings()->page2;
};

GdbOptionsPageWidget::GdbOptionsPageWidget()
{
    using namespace Layouting;
    DebuggerSettings &s = *debuggerSettings();

    Group general {
        Title { tr("General") },
        Row { s.gdbWatchdogTimeout, Stretch() },
        s.skipKnownFrames,
        s.useMessageBoxForSignals,
        s.adjustBreakpointLocations,
        s.useDynamicType,
        s.loadGdbInit,
        s.loadGdbDumpers,
        s.intelFlavor,
        s.usePseudoTracepoints,
        Stretch()
    };

    Column commands {
        Group { Title { tr("Additional Startup Commands") }, s.gdbStartupCommands },
        Group { Title { tr("Additional Attach Commands") }, s.gdbPostAttachCommands }
    };

    Row { general, commands }.attachTo(this);
}

GdbOptionsPage::GdbOptionsPage()
{
    setId("M.Gdb");
    setDisplayName(tr("GDB"));
    setCategory(Constants::DEBUGGER_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new GdbOptionsPageWidget; });
}

/////////////////////////////////////////////////////////////////////////
//
// GdbOptionsPageWidget2 - dangerous options
//
/////////////////////////////////////////////////////////////////////////

class GdbOptionsPageWidget2 : public IOptionsPageWidget
{
public:
    GdbOptionsPageWidget2();

    void apply() final { group.apply(); group.writeSettings(ICore::settings()); }
    void finish() final { group.finish(); }

    AspectContainer &group = debuggerSettings()->page3;
};

GdbOptionsPageWidget2::GdbOptionsPageWidget2()
{
    auto labelDangerous = new QLabel("<html><head/><body><i>" +
        GdbOptionsPage::tr("The options below give access to advanced "
        "or experimental functions of GDB.<br>Enabling them may negatively "
        "impact your debugging experience.") + "</i></body></html>");

    using namespace Layouting;
    DebuggerSettings &s = *debuggerSettings();

    Group {
        Title(GdbOptionsPage::tr("Extended")),
        labelDangerous,
        s.targetAsync,
        s.autoEnrichParameters,
        s.breakOnWarning,
        s.breakOnFatal,
        s.breakOnAbort,
        s.enableReverseDebugging,
        s.multiInferior,
    }.attachTo(this);
}

// The "Dangerous" options.
class GdbOptionsPage2 : public Core::IOptionsPage
{
public:
    GdbOptionsPage2()
    {
        setId("M.Gdb2");
        setDisplayName(GdbOptionsPage::tr("GDB Extended"));
        setCategory(Constants::DEBUGGER_SETTINGS_CATEGORY);
        setWidgetCreator([] { return new GdbOptionsPageWidget2; });
    }
};

// Registration

void addGdbOptionPages(QList<IOptionsPage *> *opts)
{
    opts->push_back(new GdbOptionsPage);
    opts->push_back(new GdbOptionsPage2);
}

} // namespace Internal
} // namespace Debugger
