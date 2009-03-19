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

#ifndef DEBUGGER_ACTIONS_H
#define DEBUGGER_ACTIONS_H

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QAction;
class QSettings;
QT_END_NAMESPACE


namespace Debugger {
namespace Internal {

enum ApplyMode { ImmediateApply, DeferedApply };

class DebuggerAction : public QObject
{
    Q_OBJECT

public:
    DebuggerAction(QObject *parent = 0);

    virtual QVariant value() const;
    Q_SLOT virtual void setValue(const QVariant &value, bool doemit = true);

    virtual QVariant defaultValue() const;
    Q_SLOT virtual void setDefaultValue(const QVariant &value);

    virtual QAction *action();
    virtual QAction *updatedAction(const QString &newText);
    Q_SLOT virtual void trigger(const QVariant &data) const;

    // used for persistency
    virtual QString settingsKey() const;
    Q_SLOT virtual void setSettingsKey(const QString &key);
    Q_SLOT virtual void setSettingsKey(const QString &group, const QString &key);

    virtual QString settingsGroup() const;
    Q_SLOT virtual void setSettingsGroup(const QString &group);

    virtual void readSettings(QSettings *settings);
    Q_SLOT virtual void writeSettings(QSettings *settings);
    
    virtual void connectWidget(QWidget *widget, ApplyMode applyMode = DeferedApply);
    Q_SLOT virtual void apply(QSettings *settings);

    virtual QString text() const;
    Q_SLOT virtual void setText(const QString &value);

    virtual QString textPattern() const;
    Q_SLOT virtual void setTextPattern(const QString &value);

signals:
    void valueChanged(const QVariant &newValue);
    void boolValueChanged(bool newValue);
    void stringValueChanged(const QString &newValue);

private:
    Q_SLOT void uncheckableButtonClicked();
    Q_SLOT void checkableButtonClicked(bool);
    Q_SLOT void lineEditEditingFinished();
    Q_SLOT void pathChooserEditingFinished();
    Q_SLOT void actionTriggered(bool);

    QVariant m_value;
    QVariant m_defaultValue;
    QVariant m_deferedValue; // basically a temporary copy of m_value 
    QString m_settingsKey;
    QString m_settingsGroup;
    QString m_textPattern;
    QString m_textData;
    QAction *m_action;
    QHash<QObject *, ApplyMode> m_applyModes;
};

class DebuggerSettings : public QObject
{
    Q_OBJECT

public:
    DebuggerSettings(QObject *parent = 0);
    ~DebuggerSettings();
    
    void insertItem(int code, DebuggerAction *item);
    DebuggerAction *item(int code);

    QString dump();

public slots:
    void readSettings(QSettings *settings);
    void writeSettings(QSettings *settings);

private:
    QHash<int, DebuggerAction *> m_items; 
};


///////////////////////////////////////////////////////////

enum DebuggerActionCode
{
    // General
    SettingsDialog,
    AdjustColumnWidths,
    AlwaysAdjustColumnWidths,

    LockView,

    // Gdb
    GdbLocation,
    GdbEnvironment,
    GdbScriptFile,
    GdbAutoRun,
    GdbAutoQuit,

    // Watchers & Locals
    WatchExpression,
    WatchExpressionInWindow,
    RemoveWatchExpression,
    WatchModelUpdate,
    RecheckDumpers,
    UseDumpers,
    DebugDumpers,
    UseToolTips,
    AssignValue,

    // Source List
    ListSourceFiles,

    // Running
    SkipKnownFrames,

    // Breakpoints
    SynchronizeBreakpoints,
    AllPluginBreakpoints,
    SelectedPluginBreakpoints,
    NoPluginBreakpoints,
    SelectedPluginBreakpointsPattern,

    AutoQuit,
};

// singleton access
DebuggerSettings *theDebuggerSettings();
DebuggerAction *theDebuggerAction(int code);

// convienience
bool theDebuggerBoolSetting(int code);
QString theDebuggerStringSetting(int code);

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHWINDOW_H
