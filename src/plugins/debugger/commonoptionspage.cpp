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

#include "commonoptionspage.h"

#include "debuggeractions.h"
#include "debuggerinternalconstants.h"

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
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::CommonOptionsPageWidget)

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
            Row { s.maximalStackDepth, Stretch() }
        };

        Column col2 {
            s.fontSizeFollowsEditor,
            s.switchModeOnExit,
            s.showQmlObjectTree,
            s.stationaryEditorWhileStepping,
            s.forceLoggingToConsole,
            s.registerForPostMortem,
            Stretch()
        };

        Column {
            Group { Title("Behavior"), Row { col1, col2, Stretch() } },
            s.sourcePathMap,
            Stretch()
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
    setDisplayName(QCoreApplication::translate("Debugger", "General"));
    setCategory(DEBUGGER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Debugger", "Debugger"));
    setCategoryIconPath(":/debugger/images/settingscategory_debugger.png");
    setWidgetCreator([] { return new CommonOptionsPageWidget; });
}

QString CommonOptionsPage::msgSetBreakpointAtFunction(const char *function)
{
    return CommonOptionsPageWidget::tr("Stop when %1() is called").arg(QLatin1String(function));
}

QString CommonOptionsPage::msgSetBreakpointAtFunctionToolTip(const char *function,
                                                             const QString &hint)
{
    QString result = "<html><head/><body>";
    result += CommonOptionsPageWidget::tr("Always adds a breakpoint on the <i>%1()</i> function.")
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
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::LocalsAndExpressionsOptionsPage)

public:
    LocalsAndExpressionsOptionsPageWidget()
    {
        using namespace Layouting;
        DebuggerSettings &s = *debuggerSettings();

        auto label = new QLabel; //(useHelperGroup);
        label->setTextFormat(Qt::AutoText);
        label->setWordWrap(true);
        label->setText("<html><head/><body>\n<p>"
           + tr("The debugging helpers are used to produce a nice "
                "display of objects of certain types like QString or "
                "std::map in the &quot;Locals&quot; and &quot;Expressions&quot; views.")
            + "</p></body></html>");

        Column left {
            label,
            s.useCodeModel,
            s.showThreadNames,
            Group { Title(tr("Extra Debugging Helper")), s.extraDumperFile }
        };

        Group useHelper {
            Row {
                left,
                Group { Title(tr("Debugging Helper Customization")), s.extraDumperCommands }
            }
        };

        Grid limits {
            s.maximalStringLength,
            Break(),
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
            Row { limits, Stretch() },
            Stretch()
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
    setDisplayName(QCoreApplication::translate("Debugger", "Locals && Expressions"));
    setCategory(DEBUGGER_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new LocalsAndExpressionsOptionsPageWidget; });
}

} // namespace Internal
} // namespace Debugger
