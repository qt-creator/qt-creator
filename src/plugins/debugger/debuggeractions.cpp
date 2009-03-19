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
// QtcSettingsItem
//
//////////////////////////////////////////////////////////////////////////

QtcSettingsItem::QtcSettingsItem(QObject *parent)
  : QObject(parent)
{
    m_action = new QAction(this);
    connect(m_action, SIGNAL(triggered(bool)), this, SLOT(actionTriggered(bool)));
}

QVariant QtcSettingsItem::value() const
{
    return m_value;
}

void QtcSettingsItem::setValue(const QVariant &value, bool doemit)
{
    if (value != m_value) {
        m_value = value;
        if (m_action->isCheckable())
            m_action->setChecked(m_value.toBool());
        if (doemit) {
            emit valueChanged(m_value);
            emit boolValueChanged(m_value.toBool());
            emit stringValueChanged(m_value.toString());
        }
    }
}

QVariant QtcSettingsItem::defaultValue() const
{
    return m_defaultValue;
}

void QtcSettingsItem::setDefaultValue(const QVariant &value)
{
    m_defaultValue = value;
}

QString QtcSettingsItem::settingsKey() const
{
    return m_settingsKey;
}

void QtcSettingsItem::setSettingsKey(const QString &key)
{
    m_settingsKey = key;
}

void QtcSettingsItem::setSettingsKey(const QString &group, const QString &key)
{
    m_settingsKey = key;
    m_settingsGroup = group;
}

QString QtcSettingsItem::settingsGroup() const
{
    return m_settingsGroup;
}

void QtcSettingsItem::setSettingsGroup(const QString &group)
{
    m_settingsGroup = group;
}

QString QtcSettingsItem::text() const
{
    return m_action->text();
}

void QtcSettingsItem::setText(const QString &value)
{
    m_action->setText(value);
}

QString QtcSettingsItem::textPattern() const
{
    return m_textPattern;
}

void QtcSettingsItem::setTextPattern(const QString &value)
{
    m_textPattern = value;
}

QAction *QtcSettingsItem::updatedAction(const QString &text0)
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
    m_action->setEnabled(enabled);
    m_action->setData(text0);
    m_action->setText(text);
    return m_action;
}

void QtcSettingsItem::readSettings(QSettings *settings)
{
    if (m_settingsGroup.isEmpty() || m_settingsKey.isEmpty())
        return;
    settings->beginGroup(m_settingsGroup);
    setValue(settings->value(m_settingsKey, m_defaultValue), false);
    //qDebug() << "READING: " << m_settingsKey << " -> " << m_value;
    settings->endGroup();
}

void QtcSettingsItem::writeSettings(QSettings *settings)
{
    if (m_settingsGroup.isEmpty() || m_settingsKey.isEmpty())
        return;
    settings->beginGroup(m_settingsGroup);
    settings->setValue(m_settingsKey, m_value);
    //qDebug() << "WRITING: " << m_settingsKey << " -> " << m_value;
    settings->endGroup();
}
   
QAction *QtcSettingsItem::action()
{
    return m_action;
}
 
void QtcSettingsItem::connectWidget(QWidget *widget, ApplyMode applyMode)
{
    using namespace Core::Utils;
    //qDebug() << "CONNECT WIDGET " << widget << " TO " << m_settingsKey;
    m_applyModes[widget] = applyMode;
    m_deferedValue = m_value;
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

void QtcSettingsItem::apply(QSettings *s)
{
    setValue(m_deferedValue);
    if (s)
        writeSettings(s);
}

void QtcSettingsItem::uncheckableButtonClicked()
{
    QAbstractButton *button = qobject_cast<QAbstractButton *>(sender());
    QTC_ASSERT(button, return);
    //qDebug() << "UNCHECKABLE BUTTON: " << sender();
    m_action->trigger();
}

void QtcSettingsItem::checkableButtonClicked(bool)
{
    QAbstractButton *button = qobject_cast<QAbstractButton *>(sender());
    QTC_ASSERT(button, return);
    //qDebug() << "CHECKABLE BUTTON: " << sender();
    if (m_applyModes[sender()] == DeferedApply)
        m_deferedValue = button->isChecked();
    else
        setValue(button->isChecked());
}

void QtcSettingsItem::lineEditEditingFinished()
{
    QLineEdit *lineEdit = qobject_cast<QLineEdit *>(sender());
    QTC_ASSERT(lineEdit, return);
    //qDebug() << "LINEEDIT: " << sender() << lineEdit->text();
    if (m_applyModes[sender()] == DeferedApply)
        m_deferedValue = lineEdit->text();
    else
        setValue(lineEdit->text());
}

void QtcSettingsItem::pathChooserEditingFinished()
{
    using namespace Core::Utils;
    PathChooser *pathChooser = qobject_cast<PathChooser *>(sender());
    QTC_ASSERT(pathChooser, return);
    //qDebug() << "PATHCHOOSER: " << sender() << pathChooser->path();
    if (m_applyModes[sender()] == DeferedApply)
        m_deferedValue = pathChooser->path();
    else
        setValue(pathChooser->path());
}

void QtcSettingsItem::actionTriggered(bool on)
{
    Q_UNUSED(on);
    if (QAction *action = qobject_cast<QAction *>(sender())) {
        if (action->isCheckable())
            setValue(action->isChecked());
    }
}


//////////////////////////////////////////////////////////////////////////
//
// QtcSettingsPool
//
//////////////////////////////////////////////////////////////////////////


QtcSettingsPool::QtcSettingsPool(QObject *parent)
    : QObject(parent)
{}

QtcSettingsPool::~QtcSettingsPool()
{
    qDeleteAll(m_items);
}
    
void QtcSettingsPool::insertItem(int code, QtcSettingsItem *item)
{
    m_items[code] = item;
}

void QtcSettingsPool::readSettings(QSettings *settings)
{
    foreach (QtcSettingsItem *item, m_items)
        item->readSettings(settings);
}

void QtcSettingsPool::writeSettings(QSettings *settings)
{
    foreach (QtcSettingsItem *item, m_items)
        item->writeSettings(settings);
}
   
QtcSettingsItem *QtcSettingsPool::item(int code)
{
    QTC_ASSERT(m_items.value(code, 0), return 0);
    return m_items.value(code, 0);
}

QString QtcSettingsPool::dump()
{
    QString out;
    QTextStream ts(&out);
    ts  << "Debugger settings: ";
    foreach (QtcSettingsItem *item, m_items)
        ts << "\n" << item->value().toString();
    return out;
}



//////////////////////////////////////////////////////////////////////////
//
// Debugger specific stuff
//
//////////////////////////////////////////////////////////////////////////

#if 0
    QString dump();

    QString m_gdbCmd;
    QString m_gdbEnv;
    bool m_autoRun;
    bool m_autoQuit;

    bool m_useDumpers;
    bool m_skipKnownFrames;
    bool m_debugDumpers;
    bool m_useToolTips;
    bool m_listSourceFiles;

    QString m_scriptFile;

    bool m_pluginAllBreakpoints;
    bool m_pluginSelectedBreakpoints;
    bool m_pluginNoBreakpoints;
    QString m_pluginSelectedBreakpointsPattern;
#endif


QtcSettingsPool *theDebuggerSettings()
{
    static QtcSettingsPool *instance = 0;
    if (instance)
        return instance;

    instance = new QtcSettingsPool;

    QtcSettingsItem *item = 0;

    item = new QtcSettingsItem(instance);
    instance->insertItem(AdjustColumnWidths, item);
    item->setText(QObject::tr("Adjust column widths to contents"));

    item = new QtcSettingsItem(instance);
    instance->insertItem(AlwaysAdjustColumnWidths, item);
    item->setText(QObject::tr("Always adjust column widths to contents"));
    item->action()->setCheckable(true);

    item = new QtcSettingsItem(instance);
    instance->insertItem(WatchExpression, item);
    item->setTextPattern(QObject::tr("Watch expression \"%1\""));

    item = new QtcSettingsItem(instance);
    instance->insertItem(RemoveWatchExpression, item);
    item->setTextPattern(QObject::tr("Remove watch expression \"%1\""));

    item = new QtcSettingsItem(instance);
    instance->insertItem(WatchExpressionInWindow, item);
    item->setTextPattern(QObject::tr("Watch expression \"%1\" in separate window"));

    //
    // Dumpers
    //
    item = new QtcSettingsItem(instance);
    instance->insertItem(SettingsDialog, item);
    item->setText(QObject::tr("Debugger properties..."));

    item = new QtcSettingsItem(instance);
    instance->insertItem(DebugDumpers, item);
    item->setText(QObject::tr("Debug custom dumpers"));
    item->action()->setCheckable(true);

    item = new QtcSettingsItem(instance);
    instance->insertItem(RecheckDumpers, item);
    item->setText(QObject::tr("Recheck custom dumper availability"));

    //
    // Breakpoints
    //
    item = new QtcSettingsItem(instance);
    instance->insertItem(SynchronizeBreakpoints, item);
    item->setText(QObject::tr("Syncronize breakpoints"));

    //
    item = new QtcSettingsItem(instance);
    instance->insertItem(AutoQuit, item);
    item->setText(QObject::tr("Automatically quit debugger"));
    item->action()->setCheckable(true);

    item = new QtcSettingsItem(instance);
    instance->insertItem(SkipKnownFrames, item);
    item->setText(QObject::tr("Skip known frames"));
    item->action()->setCheckable(true);

    item = new QtcSettingsItem(instance);
    instance->insertItem(UseToolTips, item);
    item->setText(QObject::tr("Use tooltips when debugging"));
    item->action()->setCheckable(true);

    item = new QtcSettingsItem(instance);
    instance->insertItem(ListSourceFiles, item);
    item->setText(QObject::tr("List source files"));
    item->action()->setCheckable(true);


    //
    // Settings
    //
    item = new QtcSettingsItem(instance);
    instance->insertItem(GdbLocation, item);
    item->setSettingsKey("DebugMode", "Location");

    item = new QtcSettingsItem(instance);
    instance->insertItem(GdbEnvironment, item);
    item->setSettingsKey("DebugMode", "Environment");

    item = new QtcSettingsItem(instance);
    instance->insertItem(GdbScriptFile, item);
    item->setSettingsKey("DebugMode", "ScriptFile");

    item = new QtcSettingsItem(instance);
    instance->insertItem(GdbAutoQuit, item);
    item->setSettingsKey("DebugMode", "AutoQuit");

    item = new QtcSettingsItem(instance);
    instance->insertItem(GdbAutoRun, item);
    item->setSettingsKey("DebugMode", "AutoRun");

    item = new QtcSettingsItem(instance);
    instance->insertItem(UseToolTips, item);
    item->setSettingsKey("DebugMode", "UseToolTips");

    item = new QtcSettingsItem(instance);
    instance->insertItem(UseDumpers, item);
    item->setSettingsKey("DebugMode", "UseCustomDumpers");
    item->setText(QObject::tr("Use custom dumpers"));
    item->action()->setCheckable(true);


    item = new QtcSettingsItem(instance);
    instance->insertItem(ListSourceFiles, item);
    item->setSettingsKey("DebugMode", "ListSourceFiles");

    item = new QtcSettingsItem(instance);
    instance->insertItem(SkipKnownFrames, item);
    item->setSettingsKey("DebugMode", "SkipKnownFrames");

    item = new QtcSettingsItem(instance);
    instance->insertItem(DebugDumpers, item);
    item->setSettingsKey("DebugMode", "DebugDumpers");

    item = new QtcSettingsItem(instance);
    instance->insertItem(AllPluginBreakpoints, item);
    item->setSettingsKey("DebugMode", "AllPluginBreakpoints");

    item = new QtcSettingsItem(instance);
    instance->insertItem(SelectedPluginBreakpoints, item);
    item->setSettingsKey("DebugMode", "SelectedPluginBreakpoints");

    item = new QtcSettingsItem(instance);
    instance->insertItem(NoPluginBreakpoints, item);
    item->setSettingsKey("DebugMode", "NoPluginBreakpoints");

    item = new QtcSettingsItem(instance);
    instance->insertItem(SelectedPluginBreakpointsPattern, item);
    item->setSettingsKey("DebugMode", "SelectedPluginBreakpointsPattern");

    return instance;
}

QtcSettingsItem *theDebuggerSetting(int code)
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

