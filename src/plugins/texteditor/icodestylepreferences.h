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

#ifndef ICODESTYLEPREFERENCES_H
#define ICODESTYLEPREFERENCES_H

#include "texteditor_global.h"

#include <QObject>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

namespace Internal {
class ICodeStylePreferencesPrivate;
}

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

    QString id() const;
    void setId(const QString &name);

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

    QString currentDelegateId() const;
    void setCurrentDelegate(const QString &id);

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

protected slots:
    void slotCodeStyleRemoved(ICodeStylePreferences *preferences);

private:
    Internal::ICodeStylePreferencesPrivate *d;
};


} // namespace TextEditor

#endif // ICODESTYLEPREFERENCES_H
