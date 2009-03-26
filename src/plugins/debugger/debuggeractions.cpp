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


using namespace Debugger::Internal;
using namespace Core::Utils;


//////////////////////////////////////////////////////////////////////////
//
// SavedAction
//
//////////////////////////////////////////////////////////////////////////

SavedAction::SavedAction(QObject *parent)
  : QAction(parent)
{
    m_widget = 0;
    connect(this, SIGNAL(triggered(bool)), this, SLOT(actionTriggered(bool)));
}

QVariant SavedAction::value() const
{
    return m_value;
}

void SavedAction::setValue(const QVariant &value, bool doemit)
{
    if (value == m_value)
        return;
    m_value = value;
    if (this->isCheckable())
        this->setChecked(m_value.toBool());
    if (doemit)
        emit valueChanged(m_value);
}

QVariant SavedAction::defaultValue() const
{
    return m_defaultValue;
}

void SavedAction::setDefaultValue(const QVariant &value)
{
    m_defaultValue = value;
}

QString SavedAction::settingsKey() const
{
    return m_settingsKey;
}

void SavedAction::setSettingsKey(const QString &key)
{
    m_settingsKey = key;
}

void SavedAction::setSettingsKey(const QString &group, const QString &key)
{
    m_settingsKey = key;
    m_settingsGroup = group;
}

QString SavedAction::settingsGroup() const
{
    return m_settingsGroup;
}

void SavedAction::setSettingsGroup(const QString &group)
{
    m_settingsGroup = group;
}

QString SavedAction::textPattern() const
{
    return m_textPattern;
}

void SavedAction::setTextPattern(const QString &value)
{
    m_textPattern = value;
}

QString SavedAction::toString() const
{
    return "value: " + m_value.toString()
        + "  defaultvalue: " + m_defaultValue.toString()
        + "  settingskey: " + m_settingsGroup + '/' + m_settingsKey;
}

QAction *SavedAction::updatedAction(const QString &text0)
{
    QString text = text0;
    bool enabled = true;
    if (!m_textPattern.isEmpty()) {
        if (text.isEmpty()) {
            text = m_textPattern;
            text.remove("\"%1\"");
            text.remove("%1");
            enabled = false;
        } else {
            text = m_textPattern.arg(text0);
        }
    }
    this->setEnabled(enabled);
    this->setData(text0);
    this->setText(text);
    return this;
}

void SavedAction::readSettings(QSettings *settings)
{
    if (m_settingsGroup.isEmpty() || m_settingsKey.isEmpty())
        return;
    settings->beginGroup(m_settingsGroup);
    setValue(settings->value(m_settingsKey, m_defaultValue), false);
    //qDebug() << "READING: " << m_settingsKey << " -> " << m_value;
    settings->endGroup();
}

void SavedAction::writeSettings(QSettings *settings)
{
    if (m_settingsGroup.isEmpty() || m_settingsKey.isEmpty())
        return;
    settings->beginGroup(m_settingsGroup);
    settings->setValue(m_settingsKey, m_value);
    //qDebug() << "WRITING: " << m_settingsKey << " -> " << toString();
    settings->endGroup();
}
   
void SavedAction::connectWidget(QWidget *widget, ApplyMode applyMode)
{
    QTC_ASSERT(!m_widget,
        qDebug() << "ALREADY CONNECTED: " << widget << m_widget << toString(); return);
    m_widget = widget;
    m_applyMode = applyMode;
    
    if (QAbstractButton *button = qobject_cast<QAbstractButton *>(widget)) {
        if (button->isCheckable()) {
            button->setChecked(m_value.toBool());
            connect(button, SIGNAL(clicked(bool)),
                this, SLOT(checkableButtonClicked(bool)));
        } else {
            connect(button, SIGNAL(clicked()),
                this, SLOT(uncheckableButtonClicked()));
        }
    } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget)) {
        lineEdit->setText(m_value.toString());
        //qDebug() << "SETTING TEXT" << lineEdit->text(); 
        connect(lineEdit, SIGNAL(editingFinished()),
            this, SLOT(lineEditEditingFinished()));
    } else if (PathChooser *pathChooser = qobject_cast<PathChooser *>(widget)) {
        pathChooser->setPath(m_value.toString());
        connect(pathChooser, SIGNAL(editingFinished()),
            this, SLOT(pathChooserEditingFinished()));
        connect(pathChooser, SIGNAL(browsingFinished()),
            this, SLOT(pathChooserEditingFinished()));
    } else {
        qDebug() << "Cannot connect widget " << widget << toString();
    }
}

void SavedAction::disconnectWidget()
{
    QTC_ASSERT(m_widget,
        qDebug() << "Widget already disconnected: " << m_widget << toString(); return);
    m_widget = 0;
}
void SavedAction::apply(QSettings *s)
{
    if (QAbstractButton *button = qobject_cast<QAbstractButton *>(m_widget))
        setValue(button->isChecked());
    else if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(m_widget))
        setValue(lineEdit->text());
    else if (PathChooser *pathChooser = qobject_cast<PathChooser *>(m_widget))
        setValue(pathChooser->path());
    if (s)
       writeSettings(s);
}

void SavedAction::uncheckableButtonClicked()
{
    QAbstractButton *button = qobject_cast<QAbstractButton *>(sender());
    QTC_ASSERT(button, return);
    //qDebug() << "UNCHECKABLE BUTTON: " << sender();
    QAction::trigger();
}

void SavedAction::checkableButtonClicked(bool)
{
    QAbstractButton *button = qobject_cast<QAbstractButton *>(sender());
    QTC_ASSERT(button, return);
    //qDebug() << "CHECKABLE BUTTON: " << sender();
    if (m_applyMode == ImmediateApply)
        setValue(button->isChecked());
}

void SavedAction::lineEditEditingFinished()
{
    QLineEdit *lineEdit = qobject_cast<QLineEdit *>(sender());
    QTC_ASSERT(lineEdit, return);
    if (m_applyMode == ImmediateApply)
        setValue(lineEdit->text());
}

void SavedAction::pathChooserEditingFinished()
{
    PathChooser *pathChooser = qobject_cast<PathChooser *>(sender());
    QTC_ASSERT(pathChooser, return);
    if (m_applyMode == ImmediateApply)
        setValue(pathChooser->path());
}

void SavedAction::actionTriggered(bool)
{
    if (isCheckable())
        setValue(isChecked());
    if (actionGroup() && actionGroup()->isExclusive()) {
        // FIXME: should be taken care of more directly
        foreach (QAction *act, actionGroup()->actions())
            if (SavedAction *dact = qobject_cast<SavedAction *>(act))
                dact->setValue(bool(act == this));
    }
}

void SavedAction::trigger(const QVariant &data)
{
    setData(data);
    QAction::trigger();
}


//////////////////////////////////////////////////////////////////////////
//
// SavedActionSet
//
//////////////////////////////////////////////////////////////////////////

void SavedActionSet::insert(SavedAction *action, QWidget *widget)
{
    m_list.append(action);
    action->connectWidget(widget);
}

void SavedActionSet::apply(QSettings *settings)
{
    foreach (SavedAction *action, m_list)
        action->apply(settings);
}

void SavedActionSet::finish()
{
    foreach (SavedAction *action, m_list)
        action->disconnectWidget();
}


//////////////////////////////////////////////////////////////////////////
//
// DebuggerSettings
//
//////////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

DebuggerSettings::DebuggerSettings(QObject *parent)
    : QObject(parent)
{}

DebuggerSettings::~DebuggerSettings()
{
    qDeleteAll(m_items);
}
    
void DebuggerSettings::insertItem(int code, SavedAction *item)
{
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
    //item->setCheckable(true);

    item = new SavedAction(instance);
    instance->insertItem(AssignValue, item);

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
    instance->insertItem(SettingsDialog, item);
    item->setText(QObject::tr("Debugger properties..."));

    item = new SavedAction(instance);
    instance->insertItem(DebugDumpers, item);
    item->setText(QObject::tr("Debug custom dumpers"));
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
    // Misc
    //

    item = new SavedAction(instance);
    instance->insertItem(SkipKnownFrames, item);
    item->setText(QObject::tr("Skip known frames"));
    item->setCheckable(true);

    item = new SavedAction(instance);
    instance->insertItem(UseToolTips, item);
    item->setText(QObject::tr("Use tooltips when debugging"));
    item->setCheckable(true);

    item = new SavedAction(instance);
    instance->insertItem(ListSourceFiles, item);
    item->setText(QObject::tr("List source files"));
    item->setCheckable(true);


    //
    // Settings
    //
    item = new SavedAction(instance);
    instance->insertItem(GdbLocation, item);
    item->setSettingsKey("DebugMode", "Location");

    item = new SavedAction(instance);
    instance->insertItem(GdbEnvironment, item);
    item->setSettingsKey("DebugMode", "Environment");

    item = new SavedAction(instance);
    instance->insertItem(GdbScriptFile, item);
    item->setSettingsKey("DebugMode", "ScriptFile");

    item = new SavedAction(instance);
    item->setSettingsKey("DebugMode", "AutoQuit");
    item->setText(QObject::tr("Automatically quit debugger"));
    item->setCheckable(true);
    instance->insertItem(AutoQuit, item);

    item = new SavedAction(instance);
    instance->insertItem(UseToolTips, item);
    item->setSettingsKey("DebugMode", "UseToolTips");

    item = new SavedAction(instance);
    instance->insertItem(UseDumpers, item);
    item->setSettingsKey("DebugMode", "UseCustomDumpers");
    item->setText(QObject::tr("Use custom dumpers"));
    item->setCheckable(true);

    item = new SavedAction(instance);
    instance->insertItem(BuildDumpersOnTheFly, item);
    item->setDefaultValue(true);
    item->setSettingsKey("DebugMode", "BuildDumpersOnTheFly");
    item->setCheckable(true);

    item = new SavedAction(instance);
    instance->insertItem(UseQtDumpers, item);
    item->setSettingsKey("DebugMode", "UseQtDumpers");
    item->setCheckable(true);

    item = new SavedAction(instance);
    instance->insertItem(UsePrebuiltDumpers, item);
    item->setSettingsKey("DebugMode", "UsePrebuiltDumpers");
    item->setCheckable(true);

    item = new SavedAction(instance);
    instance->insertItem(PrebuiltDumpersLocation, item);
    item->setSettingsKey("DebugMode", "PrebuiltDumpersLocation");

    item = new SavedAction(instance);
    instance->insertItem(TerminalApplication, item);
    item->setDefaultValue("xterm");
    item->setSettingsKey("DebugMode", "Terminal");

    item = new SavedAction(instance);
    instance->insertItem(ListSourceFiles, item);
    item->setSettingsKey("DebugMode", "ListSourceFiles");

    item = new SavedAction(instance);
    instance->insertItem(SkipKnownFrames, item);
    item->setSettingsKey("DebugMode", "SkipKnownFrames");

    item = new SavedAction(instance);
    instance->insertItem(DebugDumpers, item);
    item->setSettingsKey("DebugMode", "DebugDumpers");

    item = new SavedAction(instance);
    instance->insertItem(AllPluginBreakpoints, item);
    item->setSettingsKey("DebugMode", "AllPluginBreakpoints");

    item = new SavedAction(instance);
    instance->insertItem(SelectedPluginBreakpoints, item);
    item->setSettingsKey("DebugMode", "SelectedPluginBreakpoints");

    item = new SavedAction(instance);
    instance->insertItem(NoPluginBreakpoints, item);
    item->setSettingsKey("DebugMode", "NoPluginBreakpoints");

    item = new SavedAction(instance);
    instance->insertItem(SelectedPluginBreakpointsPattern, item);
    item->setSettingsKey("DebugMode", "SelectedPluginBreakpointsPattern");

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

