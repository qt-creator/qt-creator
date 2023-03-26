// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commonoptionspage.h"

#include "debuggeractions.h"
#include "debuggerinternalconstants.h"
#include "debuggertr.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

using namespace Core;
using namespace Debugger::Constants;
using namespace Utils;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// CommonOptionsPage
//
///////////////////////////////////////////////////////////////////////

class CommonOptionsPageWidget : public Core::IOptionsPageWidget
{
public:
    explicit CommonOptionsPageWidget()
    {
        DebuggerSettings &s = *debuggerSettings();
        using namespace Layouting;

        Column col1 {
            s.useAlternatingRowColors,
            s.useAnnotationsInMainEditor,
            s.useToolTipsInMainEditor,
            s.closeSourceBuffersOnExit,
            s.closeMemoryBuffersOnExit,
            s.raiseOnInterrupt,
            s.breakpointsFullPathByDefault,
            s.warnOnReleaseBuilds,
            Row { s.maximalStackDepth, st }
        };

        Column col2 {
            s.fontSizeFollowsEditor,
            s.switchModeOnExit,
            s.showQmlObjectTree,
            s.stationaryEditorWhileStepping,
            s.forceLoggingToConsole,
            s.registerForPostMortem,
            st
        };

        Column {
            Group { title("Behavior"), Row { col1, col2, st } },
            s.sourcePathMap,
            st
        }.attachTo(this);
    }

    void apply() final;
    void finish() final { m_group.finish(); }

private:
    AspectContainer &m_group = debuggerSettings()->page1;
};

void CommonOptionsPageWidget::apply()
{
    const DebuggerSettings *s = debuggerSettings();
    const bool originalPostMortem = s->registerForPostMortem->value();
    const bool currentPostMortem = s->registerForPostMortem->volatileValue().toBool();
    // explicitly trigger setValue() to override the setValueSilently() and trigger the registration
    if (originalPostMortem != currentPostMortem)
        s->registerForPostMortem->setValue(currentPostMortem);

    m_group.apply();
    m_group.writeSettings(ICore::settings());
}

CommonOptionsPage::CommonOptionsPage()
{
    setId(DEBUGGER_COMMON_SETTINGS_ID);
    setDisplayName(Tr::tr("General"));
    setCategory(DEBUGGER_SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr("Debugger"));
    setCategoryIconPath(":/debugger/images/settingscategory_debugger.png");
    setWidgetCreator([] { return new CommonOptionsPageWidget; });
}

QString CommonOptionsPage::msgSetBreakpointAtFunction(const char *function)
{
    return Tr::tr("Stop when %1() is called").arg(QLatin1String(function));
}

QString CommonOptionsPage::msgSetBreakpointAtFunctionToolTip(const char *function,
                                                             const QString &hint)
{
    QString result = "<html><head/><body>";
    result += Tr::tr("Always adds a breakpoint on the <i>%1()</i> function.")
            .arg(QLatin1String(function));
    if (!hint.isEmpty()) {
        result += "<br>";
        result += hint;
    }
    result += "</body></html>";
    return result;
}


///////////////////////////////////////////////////////////////////////
//
// LocalsAndExpressionsOptionsPage
//
///////////////////////////////////////////////////////////////////////

class LocalsAndExpressionsOptionsPageWidget : public IOptionsPageWidget
{
public:
    LocalsAndExpressionsOptionsPageWidget()
    {
        using namespace Layouting;
        DebuggerSettings &s = *debuggerSettings();

        auto label = new QLabel; //(useHelperGroup);
        label->setTextFormat(Qt::AutoText);
        label->setWordWrap(true);
        label->setText("<html><head/><body>\n<p>"
           + Tr::tr("The debugging helpers are used to produce a nice "
                "display of objects of certain types like QString or "
                "std::map in the &quot;Locals&quot; and &quot;Expressions&quot; views.")
            + "</p></body></html>");

        Column left {
            label,
            s.useCodeModel,
            s.showThreadNames,
            Group { title(Tr::tr("Extra Debugging Helper")), Column { s.extraDumperFile } }
        };

        Group useHelper {
            Row {
                left,
                Group {
                    title(Tr::tr("Debugging Helper Customization")),
                    Column { s.extraDumperCommands }
                }
            }
        };

        Grid limits {
            s.maximalStringLength, br,
            s.displayStringLimit
        };

        Column {
            s.useDebuggingHelpers,
            useHelper,
            Space(10),
            s.showStdNamespace,
            s.showQtNamespace,
            s.showQObjectNames,
            Space(10),
            Row { limits, st },
            st
        }.attachTo(this);
    }

    void apply() final { m_group.apply(); m_group.writeSettings(ICore::settings()); }
    void finish() final { m_group.finish(); }

private:
    AspectContainer &m_group = debuggerSettings()->page4;
};

LocalsAndExpressionsOptionsPage::LocalsAndExpressionsOptionsPage()
{
    setId("Z.Debugger.LocalsAndExpressions");
    //: '&&' will appear as one (one is marking keyboard shortcut)
    setDisplayName(Tr::tr("Locals && Expressions"));
    setCategory(DEBUGGER_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new LocalsAndExpressionsOptionsPageWidget; });
}

} // namespace Internal
} // namespace Debugger
