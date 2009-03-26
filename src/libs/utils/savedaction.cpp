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

#include <utils/savedaction.h>

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

