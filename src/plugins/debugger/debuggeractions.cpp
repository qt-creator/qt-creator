/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
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
    : QObject(parent)
{}

DebuggerSettings::~DebuggerSettings()
{
    qDeleteAll(m_items);
}
    
void DebuggerSettings::insertItem(int code, SavedAction *item)
{
    QTC_ASSERT(!m_items.contains(code), qDebug() << code << item->toString(); return);
    m_items[code] = item;
}

void DebuggerSettings::readSettings(QSettings *settings)
{
    foreach (SavedAction *item, m_items)
        item->readSettings(settings);
}

void DebuggerSettings::writeSettings(QSettings *settings)
{
    foreach (SavedAction *item, m_items)
        item->writeSettings(settings);
}
   
SavedAction *DebuggerSettings::item(int code)
{
    QTC_ASSERT(m_items.value(code, 0), return 0);
    return m_items.value(code, 0);
}

QString DebuggerSettings::dump()
{
    QString out;
    QTextStream ts(&out);
    ts  << "Debugger settings: ";
    foreach (SavedAction *item, m_items)
        ts << "\n" << item->value().toString();
    return out;
}



//////////////////////////////////////////////////////////////////////////
//
// Debugger specific actions and settings
//
//////////////////////////////////////////////////////////////////////////


DebuggerSettings *theDebuggerSettings()
{
    static DebuggerSettings *instance = 0;
    if (instance)
        return instance;

    instance = new DebuggerSettings;

    SavedAction *item = 0;

    item = new SavedAction(instance);
    instance->insertItem(SettingsDialog, item);
    item->setText(QObject::tr("Debugger properties..."));

    //
    // View
    //
    item = new SavedAction(instance);
    instance->insertItem(AdjustColumnWidths, item);
    item->setText(QObject::tr("Adjust column widths to contents"));

    item = new SavedAction(instance);
    instance->insertItem(AlwaysAdjustColumnWidths, item);
    item->setText(QObject::tr("Always adjust column widths to contents"));
    item->setCheckable(true);

    //
    // Locals & Watchers
    //
    item = new SavedAction(instance);
    instance->insertItem(WatchExpression, item);
    item->setTextPattern(QObject::tr("Watch expression \"%1\""));

    item = new SavedAction(instance);
    instance->insertItem(RemoveWatchExpression, item);
    item->setTextPattern(QObject::tr("Remove watch expression \"%1\""));

    item = new SavedAction(instance);
    instance->insertItem(WatchExpressionInWindow, item);
    item->setTextPattern(QObject::tr("Watch expression \"%1\" in separate window"));

    item = new SavedAction(instance);
    instance->insertItem(AssignValue, item);

    item = new SavedAction(instance);
    instance->insertItem(AssignType, item);

    item = new SavedAction(instance);
    instance->insertItem(ExpandItem, item);
    item->setText(QObject::tr("Expand item"));

    item = new SavedAction(instance);
    instance->insertItem(CollapseItem, item);
    item->setText(QObject::tr("Collapse item"));

    //
    // Dumpers
    //
    item = new SavedAction(instance);
    instance->insertItem(UseDumpers, item);
    item->setDefaultValue(true);
    item->setSettingsKey("DebugMode", "UseDumpers");
    item->setText(QObject::tr("Use data dumpers"));
    item->setCheckable(true);
    item->setDefaultValue(true);

    item = new SavedAction(instance);
    instance->insertItem(UseCustomDumperLocation, item);
    item->setSettingsKey("DebugMode", "CustomDumperLocation");
    item->setCheckable(true);

    item = new SavedAction(instance);
    instance->insertItem(CustomDumperLocation, item);
    item->setSettingsKey("DebugMode", "CustomDumperLocation");

    item = new SavedAction(instance);
    instance->insertItem(DebugDumpers, item);
    item->setSettingsKey("DebugMode", "DebugDumpers");
    item->setText(QObject::tr("Debug data dumpers"));
    item->setCheckable(true);


    item = new SavedAction(instance);
    item->setText(QObject::tr("Recheck custom dumper availability"));
    instance->insertItem(RecheckDumpers, item);

    //
    // Breakpoints
    //
    item = new SavedAction(instance);
    item->setText(QObject::tr("Syncronize breakpoints"));
    instance->insertItem(SynchronizeBreakpoints, item);

    //
    // Registers
    //

    QActionGroup *registerFormatGroup = new QActionGroup(instance);
    registerFormatGroup->setExclusive(true);

    item = new SavedAction(instance);
    item->setText(QObject::tr("Hexadecimal"));
    item->setCheckable(true);
    item->setSettingsKey("DebugMode", "FormatHexadecimal");
    item->setChecked(true);
    instance->insertItem(FormatHexadecimal, item);
    registerFormatGroup->addAction(item);

    item = new SavedAction(instance);
    item->setText(QObject::tr("Decimal"));
    item->setCheckable(true);
    item->setSettingsKey("DebugMode", "FormatDecimal");
    instance->insertItem(FormatDecimal, item);
    registerFormatGroup->addAction(item);

    item = new SavedAction(instance);
    item->setText(QObject::tr("Octal"));
    item->setCheckable(true);
    item->setSettingsKey("DebugMode", "FormatOctal");
    instance->insertItem(FormatOctal, item);
    registerFormatGroup->addAction(item);

    item = new SavedAction(instance);
    item->setText(QObject::tr("Binary"));
    item->setCheckable(true);
    item->setSettingsKey("DebugMode", "FormatBinary");
    instance->insertItem(FormatBinary, item);
    registerFormatGroup->addAction(item);

    item = new SavedAction(instance);
    item->setText(QObject::tr("Raw"));
    item->setCheckable(true);
    item->setSettingsKey("DebugMode", "FormatRaw");
    instance->insertItem(FormatRaw, item);
    registerFormatGroup->addAction(item);

    item = new SavedAction(instance);
    item->setText(QObject::tr("Natural"));
    item->setCheckable(true);
    item->setSettingsKey("DebugMode", "FormatNatural");
    instance->insertItem(FormatNatural, item);
    registerFormatGroup->addAction(item);


    //
    // Settings
    //
    item = new SavedAction(instance);
    item->setSettingsKey("DebugMode", "Location");
    instance->insertItem(GdbLocation, item);

    item = new SavedAction(instance);
    item->setSettingsKey("DebugMode", "Environment");
    instance->insertItem(GdbEnvironment, item);

    item = new SavedAction(instance);
    item->setSettingsKey("DebugMode", "ScriptFile");
    instance->insertItem(GdbScriptFile, item);

    item = new SavedAction(instance);
    item->setSettingsKey("DebugMode", "AutoQuit");
    item->setText(QObject::tr("Automatically quit debugger"));
    item->setCheckable(true);
    instance->insertItem(AutoQuit, item);

    item = new SavedAction(instance);
    item->setSettingsKey("DebugMode", "UseToolTips");
    item->setText(QObject::tr("Use tooltips when debugging"));
    item->setCheckable(true);
    instance->insertItem(UseToolTips, item);

    item = new SavedAction(instance);
    item->setDefaultValue("xterm");
    item->setSettingsKey("DebugMode", "Terminal");
    instance->insertItem(TerminalApplication, item);

    item = new SavedAction(instance);
    item->setSettingsKey("DebugMode", "ListSourceFiles");
    item->setText(QObject::tr("List source files"));
    item->setCheckable(true);
    instance->insertItem(ListSourceFiles, item);

    item = new SavedAction(instance);
    item->setSettingsKey("DebugMode", "SkipKnownFrames");
    item->setText(QObject::tr("Skip known frames"));
    item->setCheckable(true);
    instance->insertItem(SkipKnownFrames, item);

    item = new SavedAction(instance);
    item->setSettingsKey("DebugMode", "AllPluginBreakpoints");
    instance->insertItem(AllPluginBreakpoints, item);

    item = new SavedAction(instance);
    item->setSettingsKey("DebugMode", "SelectedPluginBreakpoints");
    instance->insertItem(SelectedPluginBreakpoints, item);

    item = new SavedAction(instance);
    item->setSettingsKey("DebugMode", "NoPluginBreakpoints");
    instance->insertItem(NoPluginBreakpoints, item);

    item = new SavedAction(instance);
    item->setSettingsKey("DebugMode", "SelectedPluginBreakpointsPattern");
    instance->insertItem(SelectedPluginBreakpointsPattern, item);

    item = new SavedAction(instance);
    item->setSettingsKey("DebugMode", "MaximalStackDepth");
    item->setDefaultValue(20);
    instance->insertItem(MaximalStackDepth, item);

    item = new SavedAction(instance);
    item->setText(QObject::tr("Reload full stack"));
    instance->insertItem(ExpandStack, item);

    item = new SavedAction(instance);
    item->setText(QObject::tr("Execute line"));
    instance->insertItem(ExecuteCommand, item);

    return instance;
}

SavedAction *theDebuggerAction(int code)
{
    return theDebuggerSettings()->item(code);
}

bool theDebuggerBoolSetting(int code)
{
    return theDebuggerSettings()->item(code)->value().toBool();
}

QString theDebuggerStringSetting(int code)
{
    return theDebuggerSettings()->item(code)->value().toString();
}

} // namespace Internal
} // namespace Debugger

