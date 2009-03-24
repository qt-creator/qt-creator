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
#include <QtGui/QAbstractButton>
#include <QtGui/QRadioButton>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>


namespace Debugger {
namespace Internal {


//////////////////////////////////////////////////////////////////////////
//
// DebuggerAction
//
//////////////////////////////////////////////////////////////////////////

DebuggerAction::DebuggerAction(QObject *parent)
  : QAction(parent)
{
    m_widget = 0;
    connect(this, SIGNAL(triggered(bool)), this, SLOT(actionTriggered(bool)));
}

QVariant DebuggerAction::value() const
{
    return m_value;
}

void DebuggerAction::setValue(const QVariant &value, bool doemit)
{
    if (value != m_value) {
        m_value = value;
        if (this->isCheckable())
            this->setChecked(m_value.toBool());
        if (doemit) {
            emit valueChanged(m_value);
            emit boolValueChanged(m_value.toBool());
            emit stringValueChanged(m_value.toString());
        }
    }
}

QVariant DebuggerAction::defaultValue() const
{
    return m_defaultValue;
}

void DebuggerAction::setDefaultValue(const QVariant &value)
{
    m_defaultValue = value;
}

QString DebuggerAction::settingsKey() const
{
    return m_settingsKey;
}

void DebuggerAction::setSettingsKey(const QString &key)
{
    m_settingsKey = key;
}

void DebuggerAction::setSettingsKey(const QString &group, const QString &key)
{
    m_settingsKey = key;
    m_settingsGroup = group;
}

QString DebuggerAction::settingsGroup() const
{
    return m_settingsGroup;
}

void DebuggerAction::setSettingsGroup(const QString &group)
{
    m_settingsGroup = group;
}

QString DebuggerAction::textPattern() const
{
    return m_textPattern;
}

void DebuggerAction::setTextPattern(const QString &value)
{
    m_textPattern = value;
}

QString DebuggerAction::toString() const
{
    return "value: " + m_value.toString()
        + "  defaultvalue: " + m_defaultValue.toString()
        + "  settingskey: " + m_settingsGroup + '/' + m_settingsKey;
}

QAction *DebuggerAction::updatedAction(const QString &text0)
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

void DebuggerAction::readSettings(QSettings *settings)
{
    if (m_settingsGroup.isEmpty() || m_settingsKey.isEmpty())
        return;
    settings->beginGroup(m_settingsGroup);
    setValue(settings->value(m_settingsKey, m_defaultValue), false);
    //qDebug() << "READING: " << m_settingsKey << " -> " << m_value;
    settings->endGroup();
}

void DebuggerAction::writeSettings(QSettings *settings)
{
    if (m_settingsGroup.isEmpty() || m_settingsKey.isEmpty())
        return;
    settings->beginGroup(m_settingsGroup);
    settings->setValue(m_settingsKey, m_value);
    //qDebug() << "WRITING: " << m_settingsKey << " -> " << toString();
    settings->endGroup();
}
   
void DebuggerAction::connectWidget(QWidget *widget, ApplyMode applyMode)
{
    using namespace Core::Utils;
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
        qDebug() << "CANNOT CONNECT WIDGET " << widget;
    }
}

void DebuggerAction::apply(QSettings *s)
{
    using namespace Core::Utils;
    if (QAbstractButton *button = qobject_cast<QAbstractButton *>(m_widget))
        setValue(button->isChecked());
    else if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(m_widget))
        setValue(lineEdit->text());
    else if (PathChooser *pathChooser = qobject_cast<PathChooser *>(m_widget))
        setValue(pathChooser->path());
    m_widget = 0;
    if (s)
       writeSettings(s);
}

void DebuggerAction::uncheckableButtonClicked()
{
    QAbstractButton *button = qobject_cast<QAbstractButton *>(sender());
    QTC_ASSERT(button, return);
    //qDebug() << "UNCHECKABLE BUTTON: " << sender();
    QAction::trigger();
}

void DebuggerAction::checkableButtonClicked(bool)
{
    QAbstractButton *button = qobject_cast<QAbstractButton *>(sender());
    QTC_ASSERT(button, return);
    //qDebug() << "CHECKABLE BUTTON: " << sender();
    if (m_applyMode == ImmediateApply)
        setValue(button->isChecked());
}

void DebuggerAction::lineEditEditingFinished()
{
    QLineEdit *lineEdit = qobject_cast<QLineEdit *>(sender());
    QTC_ASSERT(lineEdit, return);
    //qDebug() << "LINEEDIT: " << sender() << lineEdit->text();
    if (m_applyMode == ImmediateApply)
        setValue(lineEdit->text());
}

void DebuggerAction::pathChooserEditingFinished()
{
    using namespace Core::Utils;
    PathChooser *pathChooser = qobject_cast<PathChooser *>(sender());
    QTC_ASSERT(pathChooser, return);
    //qDebug() << "PATHCHOOSER: " << sender() << pathChooser->path();
    if (m_applyMode == ImmediateApply)
        setValue(pathChooser->path());
}

void DebuggerAction::actionTriggered(bool)
{
    if (this->isCheckable())
        setValue(this->isChecked());
}

void DebuggerAction::trigger(const QVariant &data)
{
    setData(data);
    QAction::trigger();
}


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
    
void DebuggerSettings::insertItem(int code, DebuggerAction *item)
{
    m_items[code] = item;
}

void DebuggerSettings::readSettings(QSettings *settings)
{
    foreach (DebuggerAction *item, m_items)
        item->readSettings(settings);
}

void DebuggerSettings::writeSettings(QSettings *settings)
{
    foreach (DebuggerAction *item, m_items)
        item->writeSettings(settings);
}
   
DebuggerAction *DebuggerSettings::item(int code)
{
    QTC_ASSERT(m_items.value(code, 0), return 0);
    return m_items.value(code, 0);
}

QString DebuggerSettings::dump()
{
    QString out;
    QTextStream ts(&out);
    ts  << "Debugger settings: ";
    foreach (DebuggerAction *item, m_items)
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

    DebuggerAction *item = 0;

    item = new DebuggerAction(instance);
    instance->insertItem(AdjustColumnWidths, item);
    item->setText(QObject::tr("Adjust column widths to contents"));

    item = new DebuggerAction(instance);
    instance->insertItem(AlwaysAdjustColumnWidths, item);
    item->setText(QObject::tr("Always adjust column widths to contents"));
    item->setCheckable(true);

    item = new DebuggerAction(instance);
    instance->insertItem(WatchExpression, item);
    item->setTextPattern(QObject::tr("Watch expression \"%1\""));

    item = new DebuggerAction(instance);
    instance->insertItem(RemoveWatchExpression, item);
    item->setTextPattern(QObject::tr("Remove watch expression \"%1\""));

    item = new DebuggerAction(instance);
    instance->insertItem(WatchExpressionInWindow, item);
    item->setTextPattern(QObject::tr("Watch expression \"%1\" in separate window"));
    //item->setCheckable(true);

    item = new DebuggerAction(instance);
    instance->insertItem(AssignValue, item);

    //
    // Dumpers
    //
    item = new DebuggerAction(instance);
    instance->insertItem(SettingsDialog, item);
    item->setText(QObject::tr("Debugger properties..."));

    item = new DebuggerAction(instance);
    instance->insertItem(DebugDumpers, item);
    item->setText(QObject::tr("Debug custom dumpers"));
    item->setCheckable(true);

    item = new DebuggerAction(instance);
    instance->insertItem(RecheckDumpers, item);
    item->setText(QObject::tr("Recheck custom dumper availability"));

    //
    // Breakpoints
    //
    item = new DebuggerAction(instance);
    instance->insertItem(SynchronizeBreakpoints, item);
    item->setText(QObject::tr("Syncronize breakpoints"));

    //
    item = new DebuggerAction(instance);
    instance->insertItem(AutoQuit, item);
    item->setText(QObject::tr("Automatically quit debugger"));
    item->setCheckable(true);

    item = new DebuggerAction(instance);
    instance->insertItem(SkipKnownFrames, item);
    item->setText(QObject::tr("Skip known frames"));
    item->setCheckable(true);

    item = new DebuggerAction(instance);
    instance->insertItem(UseToolTips, item);
    item->setText(QObject::tr("Use tooltips when debugging"));
    item->setCheckable(true);

    item = new DebuggerAction(instance);
    instance->insertItem(ListSourceFiles, item);
    item->setText(QObject::tr("List source files"));
    item->setCheckable(true);


    //
    // Settings
    //
    item = new DebuggerAction(instance);
    instance->insertItem(GdbLocation, item);
    item->setSettingsKey("DebugMode", "Location");

    item = new DebuggerAction(instance);
    instance->insertItem(GdbEnvironment, item);
    item->setSettingsKey("DebugMode", "Environment");

    item = new DebuggerAction(instance);
    instance->insertItem(GdbScriptFile, item);
    item->setSettingsKey("DebugMode", "ScriptFile");

    item = new DebuggerAction(instance);
    instance->insertItem(GdbAutoQuit, item);
    item->setSettingsKey("DebugMode", "AutoQuit");

    item = new DebuggerAction(instance);
    instance->insertItem(GdbAutoRun, item);
    item->setSettingsKey("DebugMode", "AutoRun");

    item = new DebuggerAction(instance);
    instance->insertItem(UseToolTips, item);
    item->setSettingsKey("DebugMode", "UseToolTips");

    item = new DebuggerAction(instance);
    instance->insertItem(UseDumpers, item);
    item->setSettingsKey("DebugMode", "UseCustomDumpers");
    item->setText(QObject::tr("Use custom dumpers"));
    item->setCheckable(true);

    item = new DebuggerAction(instance);
    instance->insertItem(BuildDumpersOnTheFly, item);
    item->setDefaultValue(true);
    item->setSettingsKey("DebugMode", "BuildDumpersOnTheFly");
    item->setCheckable(true);

    item = new DebuggerAction(instance);
    instance->insertItem(UsePrebuiltDumpers, item);
    item->setSettingsKey("DebugMode", "UsePrebuiltDumpers");
    item->setCheckable(true);

    item = new DebuggerAction(instance);
    instance->insertItem(PrebuiltDumpersLocation, item);
    item->setSettingsKey("DebugMode", "PrebuiltDumpersLocation");

    item = new DebuggerAction(instance);
    instance->insertItem(Terminal, item);
    item->setDefaultValue("xterm");
    item->setSettingsKey("DebugMode", "Terminal");

    item = new DebuggerAction(instance);
    instance->insertItem(ListSourceFiles, item);
    item->setSettingsKey("DebugMode", "ListSourceFiles");

    item = new DebuggerAction(instance);
    instance->insertItem(SkipKnownFrames, item);
    item->setSettingsKey("DebugMode", "SkipKnownFrames");

    item = new DebuggerAction(instance);
    instance->insertItem(DebugDumpers, item);
    item->setSettingsKey("DebugMode", "DebugDumpers");

    item = new DebuggerAction(instance);
    instance->insertItem(AllPluginBreakpoints, item);
    item->setSettingsKey("DebugMode", "AllPluginBreakpoints");

    item = new DebuggerAction(instance);
    instance->insertItem(SelectedPluginBreakpoints, item);
    item->setSettingsKey("DebugMode", "SelectedPluginBreakpoints");

    item = new DebuggerAction(instance);
    instance->insertItem(NoPluginBreakpoints, item);
    item->setSettingsKey("DebugMode", "NoPluginBreakpoints");

    item = new DebuggerAction(instance);
    instance->insertItem(SelectedPluginBreakpointsPattern, item);
    item->setSettingsKey("DebugMode", "SelectedPluginBreakpointsPattern");

    return instance;
}

DebuggerAction *theDebuggerAction(int code)
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

