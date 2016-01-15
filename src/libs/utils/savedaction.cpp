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

#include <utils/savedaction.h>

#include <utils/qtcassert.h>
#include <utils/pathchooser.h>
#include <utils/pathlisteditor.h>

#include <QDebug>
#include <QSettings>

#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>

namespace Utils {

//////////////////////////////////////////////////////////////////////////
//
// SavedAction
//
//////////////////////////////////////////////////////////////////////////

/*!
    \class Utils::SavedAction

    \brief The SavedAction class is a helper class for actions with persistent
    state.
*/

SavedAction::SavedAction(QObject *parent)
  : QAction(parent)
{
    m_widget = 0;
    connect(this, &QAction::triggered, this, &SavedAction::actionTriggered);
}


/*!
    Returns the current value of the object.

    \sa setValue()
*/
QVariant SavedAction::value() const
{
    return m_value;
}


/*!
    Sets the current value of the object. If the value changed and
    \a doemit is true, the \c valueChanged() signal will be emitted.

    \sa value()
*/
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


/*!
    Returns the default value to be used when the item does not exist yet
    in the settings.

    \sa setDefaultValue()
*/
QVariant SavedAction::defaultValue() const
{
    return m_defaultValue;
}


/*!
    Sets the default value to be used when the item does not exist yet
    in the settings.

    \sa defaultValue()
*/
void SavedAction::setDefaultValue(const QVariant &value)
{
    m_defaultValue = value;
}


/*!
    Returns the key to be used when accessing the settings.

    \sa settingsKey()
*/
QString SavedAction::settingsKey() const
{
    return m_settingsKey;
}


/*!
    Sets the key to be used when accessing the settings.

    \sa settingsKey()
*/
void SavedAction::setSettingsKey(const QString &key)
{
    m_settingsKey = key;
}


/*!
    Sets the key and group to be used when accessing the settings.

    \sa settingsKey()
*/
void SavedAction::setSettingsKey(const QString &group, const QString &key)
{
    m_settingsKey = key;
    m_settingsGroup = group;
}


/*!
    Sets the key to be used when accessing the settings.

    \sa settingsKey()
*/
QString SavedAction::settingsGroup() const
{
    return m_settingsGroup;
}

/*!
    Sets the group to be used when accessing the settings.

    \sa settingsGroup()
*/
void SavedAction::setSettingsGroup(const QString &group)
{
    m_settingsGroup = group;
}

QString SavedAction::toString() const
{
    return QLatin1String("value: ") + m_value.toString()
        + QLatin1String("  defaultvalue: ") + m_defaultValue.toString()
        + QLatin1String("  settingskey: ") + m_settingsGroup
        + QLatin1Char('/') + m_settingsKey;
}

/*
    Uses \c settingsGroup() and \c settingsKey() to restore the
    item from \a settings,

    \sa settingsKey(), settingsGroup(), writeSettings()
*/
void SavedAction::readSettings(const QSettings *settings)
{
    if (m_settingsGroup.isEmpty() || m_settingsKey.isEmpty())
        return;
    QVariant var = settings->value(m_settingsGroup + QLatin1Char('/') + m_settingsKey, m_defaultValue);
    // work around old ini files containing @Invalid() entries
    if (isCheckable() && !var.isValid())
        var = false;
    setValue(var);
}

/*
    Uses \c settingsGroup() and \c settingsKey() to write the
    item to \a settings,

    \sa settingsKey(), settingsGroup(), readSettings()
*/
void SavedAction::writeSettings(QSettings *settings)
{
    if (m_settingsGroup.isEmpty() || m_settingsKey.isEmpty())
        return;
    settings->beginGroup(m_settingsGroup);
    settings->setValue(m_settingsKey, m_value);
    settings->endGroup();
}

/*
    A \c SavedAction can be connected to a widget, typically a
    checkbox, radiobutton, or a lineedit in some configuration dialog.

    The widget will retrieve its contents from the SavedAction's
    value, and - depending on the \a ApplyMode - either write
    changes back immediately, or when \s SavedAction::apply()
    is called explicitly.

    \sa apply(), disconnectWidget()
*/
void SavedAction::connectWidget(QWidget *widget, ApplyMode applyMode)
{
    QTC_ASSERT(!m_widget,
        qDebug() << "ALREADY CONNECTED: " << widget << m_widget << toString(); return);
    m_widget = widget;

    if (auto button = qobject_cast<QCheckBox *>(widget)) {
        if (!m_dialogText.isEmpty())
            button->setText(m_dialogText);
        button->setChecked(m_value.toBool());
        if (applyMode == ImmediateApply) {
            connect(button, &QCheckBox::clicked,
                    this, [this, button] { setValue(button->isChecked()); });
        }
    } else if (auto spinBox = qobject_cast<QSpinBox *>(widget)) {
        spinBox->setValue(m_value.toInt());
        if (applyMode == ImmediateApply) {
            connect(spinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, [this, spinBox]() { setValue(spinBox->value()); });
        }
    } else if (auto lineEdit = qobject_cast<QLineEdit *>(widget)) {
        lineEdit->setText(m_value.toString());
        if (applyMode == ImmediateApply) {
            connect(lineEdit, &QLineEdit::editingFinished,
                    this, [this, lineEdit] { setValue(lineEdit->text()); });
        }

    } else if (auto pathChooser = qobject_cast<PathChooser *>(widget)) {
        pathChooser->setPath(m_value.toString());
        if (applyMode == ImmediateApply) {
            auto finished = [this, pathChooser] { setValue(pathChooser->path()); };
            connect(pathChooser, &PathChooser::editingFinished, this, finished);
            connect(pathChooser, &PathChooser::browsingFinished, this, finished);
        }
    } else if (auto groupBox = qobject_cast<QGroupBox *>(widget)) {
        if (!groupBox->isCheckable())
            qDebug() << "connectWidget to non-checkable group box" << widget << toString();
        groupBox->setChecked(m_value.toBool());
        if (applyMode == ImmediateApply) {
            connect(groupBox, &QGroupBox::toggled,
                    this, [this, groupBox] { setValue(QVariant(groupBox->isChecked())); });
        }
    } else if (auto textEdit = qobject_cast<QTextEdit *>(widget)) {
        textEdit->setPlainText(m_value.toString());
        if (applyMode == ImmediateApply) {
            connect(textEdit, &QTextEdit::textChanged,
                    this, [this, textEdit] { setValue(textEdit->toPlainText()); });
        }
    } else if (auto editor = qobject_cast<PathListEditor *>(widget)) {
        editor->setPathList(m_value.toStringList());
    } else {
        qDebug() << "Cannot connect widget " << widget << toString();
    }

    // Copy tooltip, but only if there's nothing explcitly set on the widget yet.
    if (widget->toolTip().isEmpty())
        widget->setToolTip(toolTip());
}

/*
    Disconnects the \c SavedAction from a widget.

    \sa apply(), connectWidget()
*/
void SavedAction::disconnectWidget()
{
    m_widget = 0;
}

void SavedAction::apply(QSettings *s)
{
    if (auto button = qobject_cast<QCheckBox *>(m_widget))
        setValue(button->isChecked());
    else if (auto lineEdit = qobject_cast<QLineEdit *>(m_widget))
        setValue(lineEdit->text());
    else if (auto spinBox = qobject_cast<QSpinBox *>(m_widget))
        setValue(spinBox->value());
    else if (auto pathChooser = qobject_cast<PathChooser *>(m_widget))
        setValue(pathChooser->path());
    else if (auto groupBox = qobject_cast<QGroupBox *>(m_widget))
        setValue(groupBox->isChecked());
    else if (auto textEdit = qobject_cast<QTextEdit *>(m_widget))
        setValue(textEdit->toPlainText());
    else if (auto editor = qobject_cast<PathListEditor *>(m_widget))
        setValue(editor->pathList());
    if (s)
       writeSettings(s);
}

/*
    Default text to be used in labels if this SavedAction is
    used in a settings dialog.

    This typically is similar to the text this SavedAction shows
    when used in menus, but differs in capitalization.


    \sa text()
*/
QString SavedAction::dialogText() const
{
    return m_dialogText;
}

void SavedAction::setDialogText(const QString &dialogText)
{
    m_dialogText = dialogText;
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
    if (widget)
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

} // namespace Utils
