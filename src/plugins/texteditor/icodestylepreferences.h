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

#include "texteditor_global.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QVariant;
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

namespace Internal { class ICodeStylePreferencesPrivate; }

class TabSettings;
class CodeStylePool;

class TEXTEDITOR_EXPORT ICodeStylePreferences : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
public:
    // pool is a pool which will be used by this preferences for setting delegates
    explicit ICodeStylePreferences(QObject *parentObject = 0);
    virtual ~ICodeStylePreferences();

    QByteArray id() const;
    void setId(const QByteArray &name);

    QString displayName() const;
    void setDisplayName(const QString &name);

    bool isReadOnly() const;
    void setReadOnly(bool on);

    void setTabSettings(const TabSettings &settings);
    TabSettings tabSettings() const;
    TabSettings currentTabSettings() const;

    virtual QVariant value() const = 0;
    virtual void setValue(const QVariant &) = 0;

    QVariant currentValue() const; // may be from grandparent

    ICodeStylePreferences *currentPreferences() const; // may be grandparent

    CodeStylePool *delegatingPool() const;
    void setDelegatingPool(CodeStylePool *pool);

    ICodeStylePreferences *currentDelegate() const; // null or one of delegates from the pool
    void setCurrentDelegate(ICodeStylePreferences *delegate);

    QByteArray currentDelegateId() const;
    void setCurrentDelegate(const QByteArray &id);

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    // make below 2 protected?
    virtual void toMap(const QString &prefix, QVariantMap *map) const;
    virtual void fromMap(const QString &prefix, const QVariantMap &map);

signals:
    void tabSettingsChanged(const TextEditor::TabSettings &settings);
    void currentTabSettingsChanged(const TextEditor::TabSettings &settings);
    void valueChanged(const QVariant &);
    void currentValueChanged(const QVariant &);
    void currentDelegateChanged(TextEditor::ICodeStylePreferences *currentDelegate);
    void currentPreferencesChanged(TextEditor::ICodeStylePreferences *currentPreferences);
    void displayNameChanged(const QString &newName);

protected:
    virtual QString settingsSuffix() const = 0;

private:
    void codeStyleRemoved(ICodeStylePreferences *preferences);

    Internal::ICodeStylePreferencesPrivate *d;
};


} // namespace TextEditor
