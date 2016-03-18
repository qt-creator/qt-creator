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

#pragma once

#include "utils_global.h"

#include <QAction>

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

    QVariant value() const;
    void setValue(const QVariant &value, bool doemit = true);

    QVariant defaultValue() const;
    void setDefaultValue(const QVariant &value);

    void trigger(const QVariant &data);

    // used for persistency
    QString settingsKey() const;
    void setSettingsKey(const QString &key);
    void setSettingsKey(const QString &group, const QString &key);

    QString settingsGroup() const;
    void setSettingsGroup(const QString &group);

    virtual void readSettings(const QSettings *settings);
    virtual void writeSettings(QSettings *settings);

    void connectWidget(QWidget *widget, ApplyMode applyMode = DeferedApply);
    void disconnectWidget();
    void apply(QSettings *settings);

    QString toString() const;

    QString dialogText() const;
    void setDialogText(const QString &dialogText);

signals:
    void valueChanged(const QVariant &newValue);

private:
    void actionTriggered(bool);

    QVariant m_value;
    QVariant m_defaultValue;
    QString m_settingsKey;
    QString m_settingsGroup;
    QString m_dialogText;
    QWidget *m_widget;
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
