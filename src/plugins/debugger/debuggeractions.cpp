/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "debuggeractions.h"

#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtGui/QAbstractButton>
#include <QtGui/QRadioButton>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>

using namespace Core::Utils;

namespace Debugger {
namespace Internal {

//////////////////////////////////////////////////////////////////////////
//
// DebuggerSettings
//
//////////////////////////////////////////////////////////////////////////

DebuggerSettings::DebuggerSettings(QObject *parent)
    : QObject(parent), m_registerFormatGroup(0)
{}

DebuggerSettings::~DebuggerSettings()
{
    qDeleteAll(m_items);
}
    
void DebuggerSettings::insertItem(int code, SavedAction *item)
{
    QTC_ASSERT(!m_items.contains(code),
        qDebug() << code << item->toString(); return);
    QTC_ASSERT(item->settingsKey().isEmpty() || item->defaultValue().isValid(),
        qDebug() << "NO DEFAULT VALUE FOR " << item->settingsKey());
    m_items[code] = item;
}

void DebuggerSettings::readSettings(QSettings *settings)
{
    foreach (SavedAction *item, m_items)
        item->readSettings(settings);
}

void DebuggerSettings::writeSettings(QSettings *settings) const
{
    foreach (SavedAction *item, m_items)
        item->writeSettings(settings);
}
   
SavedAction *DebuggerSettings::item(int code) const
{
    QTC_ASSERT(m_items.value(code, 0), return 0);
    return m_items.value(code, 0);
}

QString DebuggerSettings::dump() const
{
    QString out;
    QTextStream ts(&out);
    ts << "Debugger settings: ";
    foreach (SavedAction *item, m_items) {
        QString key = item->settingsKey();
        if (!key.isEmpty())
            ts << '\n' << key << ": " << item->value().toString()
               << "  (default: " << item->defaultValue().toString() << ")";
    }
    return out;
}

DebuggerSettings *DebuggerSettings::instance()
{
    static DebuggerSettings *instance = 0;
    if (instance)
        return instance;

    const QString debugModeGroup = QLatin1String("DebugMode");
    instance = new DebuggerSettings;

    SavedAction *item = 0;

    item = new SavedAction(instance);
    instance->insertItem(SettingsDialog, item);
    item->setText(tr("Debugger properties..."));

    //
    // View
    //
    item = new SavedAction(instance);
    instance->insertItem(AdjustColumnWidths, item);
    item->setText(tr("Adjust column widths to contents"));

    item = new SavedAction(instance);
    instance->insertItem(AlwaysAdjustColumnWidths, item);
    item->setText(tr("Always adjust column widths to contents"));
    item->setCheckable(true);

    item = new SavedAction(instance);
    item->setText(tr("Use alternating row colors"));
    item->setSettingsKey(debugModeGroup, QLatin1String("UseAlternatingRowColours"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    instance->insertItem(UseAlternatingRowColors, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("LogTimeStamps"));
    item->setText(tr("Log time stamps"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    instance->insertItem(LogTimeStamps, item);

    //
    // Locals & Watchers
    //
    item = new SavedAction(instance);
    item->setTextPattern(tr("Watch expression \"%1\""));
    instance->insertItem(WatchExpression, item);

    item = new SavedAction(instance);
    item->setTextPattern(tr("Remove watch expression \"%1\""));
    instance->insertItem(RemoveWatchExpression, item);

    item = new SavedAction(instance);
    item->setTextPattern(tr("Watch expression \"%1\" in separate window"));
    instance->insertItem(WatchExpressionInWindow, item);

    item = new SavedAction(instance);
    instance->insertItem(AssignValue, item);

    item = new SavedAction(instance);
    instance->insertItem(AssignType, item);

    //
    // DebuggingHelper
    //
    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseDebuggingHelper"));
    item->setText(tr("Use debugging helper"));
    item->setCheckable(true);
    item->setDefaultValue(true);
    instance->insertItem(UseDebuggingHelpers, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseCustomDebuggingHelperLocation"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    instance->insertItem(UseCustomDebuggingHelperLocation, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("CustomDebuggingHelperLocation"));
    item->setCheckable(true);
    item->setDefaultValue(QString());
    instance->insertItem(CustomDebuggingHelperLocation, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("DebugDebuggingHelpers"));
    item->setText(tr("Debug debugging helper"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    instance->insertItem(DebugDebuggingHelpers, item);


    item = new SavedAction(instance);
    item->setText(tr("Recheck debugging helper availability"));
    instance->insertItem(RecheckDebuggingHelpers, item);

    //
    // Breakpoints
    //
    item = new SavedAction(instance);
    item->setText(tr("Synchronize breakpoints"));
    instance->insertItem(SynchronizeBreakpoints, item);


    //
    // Registers
    //

    instance->m_registerFormatGroup = new QActionGroup(instance);
    instance->m_registerFormatGroup->setExclusive(true);

    item = new SavedAction(instance);
    item->setText(tr("Hexadecimal"));
    item->setCheckable(true);
    item->setSettingsKey(debugModeGroup, QLatin1String("FormatHexadecimal"));
    item->setChecked(true);
    item->setDefaultValue(false);
    item->setData(FormatHexadecimal);
    instance->insertItem(FormatHexadecimal, item);
    instance->m_registerFormatGroup->addAction(item);

    item = new SavedAction(instance);
    item->setText(tr("Decimal"));
    item->setCheckable(true);
    item->setSettingsKey(debugModeGroup, QLatin1String("FormatDecimal"));
    item->setDefaultValue(false);
    item->setData(FormatDecimal);
    instance->insertItem(FormatDecimal, item);
    instance->m_registerFormatGroup->addAction(item);

    item = new SavedAction(instance);
    item->setText(tr("Octal"));
    item->setCheckable(true);
    item->setSettingsKey(debugModeGroup, QLatin1String("FormatOctal"));
    item->setDefaultValue(false);
    item->setData(FormatOctal);
    instance->insertItem(FormatOctal, item);
    instance->m_registerFormatGroup->addAction(item);

    item = new SavedAction(instance);
    item->setText(tr("Binary"));
    item->setCheckable(true);
    item->setSettingsKey(debugModeGroup, QLatin1String("FormatBinary"));
    item->setDefaultValue(false);
    item->setData(FormatBinary);
    instance->insertItem(FormatBinary, item);
    instance->m_registerFormatGroup->addAction(item);

    item = new SavedAction(instance);
    item->setText(tr("Raw"));
    item->setCheckable(true);
    item->setSettingsKey(debugModeGroup, QLatin1String("FormatRaw"));
    item->setDefaultValue(false);
    item->setData(FormatRaw);
    instance->insertItem(FormatRaw, item);
    instance->m_registerFormatGroup->addAction(item);

    item = new SavedAction(instance);
    item->setText(tr("Natural"));
    item->setCheckable(true);
    item->setSettingsKey(debugModeGroup, QLatin1String("FormatNatural"));
    item->setDefaultValue(true);
    item->setData(FormatNatural);
    instance->insertItem(FormatNatural, item);
    instance->m_registerFormatGroup->addAction(item);

    //
    // Settings
    //
    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("Location"));
    item->setDefaultValue("gdb");
    instance->insertItem(GdbLocation, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("Environment"));
    item->setDefaultValue(QString());
    instance->insertItem(GdbEnvironment, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("ScriptFile"));
    item->setDefaultValue(QString());
    instance->insertItem(GdbScriptFile, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("AutoQuit"));
    item->setText(tr("Automatically quit debugger"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    instance->insertItem(AutoQuit, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseToolTips"));
    item->setText(tr("Use tooltips when debugging"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    instance->insertItem(UseToolTips, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("ListSourceFiles"));
    item->setText(tr("List source files"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    instance->insertItem(ListSourceFiles, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("SkipKnownFrames"));
    item->setText(tr("Skip known frames"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    instance->insertItem(SkipKnownFrames, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("EnableReverseDebugging"));
    item->setText(tr("Enable reverse debugging"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    instance->insertItem(EnableReverseDebugging, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("AllPluginBreakpoints"));
    item->setDefaultValue(true);
    instance->insertItem(AllPluginBreakpoints, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("SelectedPluginBreakpoints"));
    item->setDefaultValue(false);
    instance->insertItem(SelectedPluginBreakpoints, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("NoPluginBreakpoints"));
    item->setDefaultValue(false);
    instance->insertItem(NoPluginBreakpoints, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("SelectedPluginBreakpointsPattern"));
    item->setDefaultValue(QString(".*"));
    instance->insertItem(SelectedPluginBreakpointsPattern, item);

    item = new SavedAction(instance);
    item->setSettingsKey(debugModeGroup, QLatin1String("MaximalStackDepth"));
    item->setDefaultValue(20);
    instance->insertItem(MaximalStackDepth, item);

    item = new SavedAction(instance);
    item->setText(tr("Reload full stack"));
    instance->insertItem(ExpandStack, item);

    item = new SavedAction(instance);
    item->setText(tr("Execute line"));
    instance->insertItem(ExecuteCommand, item);

    return instance;
}

int DebuggerSettings::checkedRegisterFormatAction() const
{
    return m_registerFormatGroup->checkedAction()->data().toInt();
}

//////////////////////////////////////////////////////////////////////////
//
// DebuggerActions
//
//////////////////////////////////////////////////////////////////////////

SavedAction *theDebuggerAction(int code)
{
    return DebuggerSettings::instance()->item(code);
}

int checkedRegisterFormatAction()
{
    return DebuggerSettings::instance()->checkedRegisterFormatAction();
}

bool theDebuggerBoolSetting(int code)
{
    return DebuggerSettings::instance()->item(code)->value().toBool();
}

QString theDebuggerStringSetting(int code)
{
    return DebuggerSettings::instance()->item(code)->value().toString();
}

} // namespace Internal
} // namespace Debugger

