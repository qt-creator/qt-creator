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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef SAVED_ACTION_H
#define SAVED_ACTION_H

#include "utils_global.h"

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QList>

#include <QtGui/QAction>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {

enum ApplyMode { ImmediateApply, DeferedApply };

class QTCREATOR_UTILS_EXPORT SavedAction : public QAction
{
    Q_OBJECT

public:
    SavedAction(QObject *parent = 0);

    virtual QVariant value() const;
    Q_SLOT virtual void setValue(const QVariant &value, bool doemit = true);

    virtual QVariant defaultValue() const;
    Q_SLOT virtual void setDefaultValue(const QVariant &value);

    virtual QAction *updatedAction(const QString &newText);
    Q_SLOT virtual void trigger(const QVariant &data);

    // used for persistency
    virtual QString settingsKey() const;
    Q_SLOT virtual void setSettingsKey(const QString &key);
    Q_SLOT virtual void setSettingsKey(const QString &group, const QString &key);

    virtual QString settingsGroup() const;
    Q_SLOT virtual void setSettingsGroup(const QString &group);

    virtual void readSettings(QSettings *settings);
    Q_SLOT virtual void writeSettings(QSettings *settings);
    
    virtual void connectWidget(QWidget *widget, ApplyMode applyMode = DeferedApply);
    virtual void disconnectWidget();
    Q_SLOT virtual void apply(QSettings *settings);

    virtual QString textPattern() const;
    Q_SLOT virtual void setTextPattern(const QString &value);

    QString toString() const;

signals:
    void valueChanged(const QVariant &newValue);

private:
    Q_SLOT void uncheckableButtonClicked();
    Q_SLOT void checkableButtonClicked(bool);
    Q_SLOT void lineEditEditingFinished();
    Q_SLOT void pathChooserEditingFinished();
    Q_SLOT void actionTriggered(bool);
    Q_SLOT void spinBoxValueChanged(int);
    Q_SLOT void spinBoxValueChanged(QString);

    QVariant m_value;
    QVariant m_defaultValue;
    QString m_settingsKey;
    QString m_settingsGroup;
    QString m_textPattern;
    QString m_textData;
    QWidget *m_widget;
    ApplyMode m_applyMode;
};

class QTCREATOR_UTILS_EXPORT SavedActionSet
{
public:
    SavedActionSet() {}
    ~SavedActionSet() {}

    void insert(SavedAction *action, QWidget *widget);
    void apply(QSettings *settings);
    void finish();
    void clear() { m_list.clear(); }

private:
    QList<SavedAction *> m_list;
};

} // namespace Utils

#endif // SAVED_ACTION_H
