/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <utils/savedaction.h>

#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QDebug>
#include <QSettings>

#include <QAbstractButton>
#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QSpinBox>
#include <QTextEdit>

using namespace Utils;

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
    connect(this, SIGNAL(triggered(bool)), this, SLOT(actionTriggered(bool)));
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
    return QLatin1String("value: ") + m_value.toString()
        + QLatin1String("  defaultvalue: ") + m_defaultValue.toString()
        + QLatin1String("  settingskey: ") + m_settingsGroup
        + QLatin1Char('/') + m_settingsKey;
}

/*!
    \fn QAction *SavedAction::updatedAction(const QString &text)

    Adjust the \c text() of the underlying action.

    This can be used to update the item shortly before e.g. a menu is shown.

    If the item's \c textPattern() is empty the \a text will be used
    verbatim.

    Otherwise, the behaviour depends on \a text: if it is non-empty,
    \c QString(textPattern()).arg(text), otherwise, \c textPattern()
    with the "%1" placeholder removed will be used.

    \sa textPattern(), setTextPattern()
*/
QAction *SavedAction::updatedAction(const QString &text0)
{
    QString text = text0;
    bool enabled = true;
    if (!m_textPattern.isEmpty()) {
        if (text.isEmpty()) {
            text = m_textPattern;
            text.remove(QLatin1String("\"%1\""));
            text.remove(QLatin1String("%1"));
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
    //qDebug() << "READING: " << var.isValid() << m_settingsKey << " -> " << m_value
    //    << " (default: " << m_defaultValue << ")" << var;
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
    //qDebug() << "WRITING: " << m_settingsKey << " -> " << toString();
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
    } else if (QSpinBox *spinBox = qobject_cast<QSpinBox *>(widget)) {
        spinBox->setValue(m_value.toInt());
        //qDebug() << "SETTING VALUE" << spinBox->value();
        connect(spinBox, SIGNAL(valueChanged(int)),
            this, SLOT(spinBoxValueChanged(int)));
        connect(spinBox, SIGNAL(valueChanged(QString)),
            this, SLOT(spinBoxValueChanged(QString)));
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
    } else if (QGroupBox *groupBox = qobject_cast<QGroupBox *>(widget)) {
        if (!groupBox->isCheckable())
            qDebug() << "connectWidget to non-checkable group box" << widget << toString();
        groupBox->setChecked(m_value.toBool());
        connect(groupBox, SIGNAL(toggled(bool)), this, SLOT(groupBoxToggled(bool)));
    } else if (QTextEdit *textEdit = qobject_cast<QTextEdit *>(widget)) {
        textEdit->setPlainText(m_value.toString());
        connect(textEdit, SIGNAL(textChanged()), this, SLOT(textEditTextChanged()));
    } else {
        qDebug() << "Cannot connect widget " << widget << toString();
    }
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
    if (QAbstractButton *button = qobject_cast<QAbstractButton *>(m_widget))
        setValue(button->isChecked());
    else if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(m_widget))
        setValue(lineEdit->text());
    else if (QSpinBox *spinBox = qobject_cast<QSpinBox *>(m_widget))
        setValue(spinBox->value());
    else if (PathChooser *pathChooser = qobject_cast<PathChooser *>(m_widget))
        setValue(pathChooser->path());
    else if (const QGroupBox *groupBox = qobject_cast<QGroupBox *>(m_widget))
        setValue(groupBox->isChecked());
    else if (const QTextEdit *textEdit = qobject_cast<QTextEdit *>(m_widget))
        setValue(textEdit->toPlainText());
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

void SavedAction::spinBoxValueChanged(int value)
{
    QSpinBox *spinBox = qobject_cast<QSpinBox *>(sender());
    QTC_ASSERT(spinBox, return);
    if (m_applyMode == ImmediateApply)
        setValue(value);
}

void SavedAction::spinBoxValueChanged(QString value)
{
    QSpinBox *spinBox = qobject_cast<QSpinBox *>(sender());
    QTC_ASSERT(spinBox, return);
    if (m_applyMode == ImmediateApply)
        setValue(value);
}

void SavedAction::pathChooserEditingFinished()
{
    PathChooser *pathChooser = qobject_cast<PathChooser *>(sender());
    QTC_ASSERT(pathChooser, return);
    if (m_applyMode == ImmediateApply)
        setValue(pathChooser->path());
}

void SavedAction::textEditTextChanged()
{
    QTextEdit *textEdit = qobject_cast<QTextEdit *>(sender());
    QTC_ASSERT(textEdit, return);
    if (m_applyMode == ImmediateApply)
        setValue(textEdit->toPlainText());
}

void SavedAction::groupBoxToggled(bool checked)
{
    if (m_applyMode == ImmediateApply)
        setValue(QVariant(checked));
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

QString SavedActionSet::searchKeyWords() const
{
    const QChar blank = QLatin1Char(' ');
    QString rc;
    foreach (SavedAction *action, m_list) {
        if (!rc.isEmpty())
            rc += blank;
        rc += action->text();
    }
    rc.remove(QLatin1Char('&'));
    return rc;
}

