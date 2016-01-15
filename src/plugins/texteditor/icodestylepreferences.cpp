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

#include "icodestylepreferences.h"
#include "codestylepool.h"
#include "tabsettings.h"
#include <utils/settingsutils.h>

#include <QSettings>

using namespace TextEditor;

static const char currentPreferencesKey[] = "CurrentPreferences";

namespace TextEditor {
namespace Internal {

class ICodeStylePreferencesPrivate
{
public:
    ICodeStylePreferencesPrivate()
        : m_pool(0),
          m_currentDelegate(0),
          m_readOnly(false)
    {}

    CodeStylePool *m_pool;
    ICodeStylePreferences *m_currentDelegate;
    TabSettings m_tabSettings;
    QByteArray m_id;
    QString m_displayName;
    bool m_readOnly;
};

}
}

ICodeStylePreferences::ICodeStylePreferences(QObject *parent) :
    QObject(parent),
    d(new Internal::ICodeStylePreferencesPrivate)
{
}

ICodeStylePreferences::~ICodeStylePreferences()
{
    delete d;
}

QByteArray ICodeStylePreferences::id() const
{
    return d->m_id;
}

void ICodeStylePreferences::setId(const QByteArray &name)
{
    d->m_id = name;
}

QString ICodeStylePreferences::displayName() const
{
    return d->m_displayName;
}

void ICodeStylePreferences::setDisplayName(const QString &name)
{
    d->m_displayName = name;
    emit displayNameChanged(name);
}

bool ICodeStylePreferences::isReadOnly() const
{
    return d->m_readOnly;
}

void ICodeStylePreferences::setReadOnly(bool on)
{
    d->m_readOnly = on;
}

void ICodeStylePreferences::setTabSettings(const TabSettings &settings)
{
    if (d->m_tabSettings == settings)
        return;

    d->m_tabSettings = settings;

    emit tabSettingsChanged(d->m_tabSettings);
    if (!currentDelegate())
        emit currentTabSettingsChanged(d->m_tabSettings);
}

TabSettings ICodeStylePreferences::tabSettings() const
{
    return d->m_tabSettings;
}

TabSettings ICodeStylePreferences::currentTabSettings() const
{
    return currentPreferences()->tabSettings();
}

QVariant ICodeStylePreferences::currentValue() const
{
    return currentPreferences()->value();
}

ICodeStylePreferences *ICodeStylePreferences::currentPreferences() const
{
    ICodeStylePreferences *prefs = (ICodeStylePreferences *)this;
    while (prefs->currentDelegate())
        prefs = prefs->currentDelegate();
    return prefs;
}

CodeStylePool *ICodeStylePreferences::delegatingPool() const
{
    return d->m_pool;
}

void ICodeStylePreferences::setDelegatingPool(CodeStylePool *pool)
{
    if (pool == d->m_pool)
        return;

    setCurrentDelegate(0);
    if (d->m_pool) {
        disconnect(d->m_pool, &CodeStylePool::codeStyleRemoved,
                   this, &ICodeStylePreferences::codeStyleRemoved);
    }
    d->m_pool = pool;
    if (d->m_pool) {
        connect(d->m_pool, &CodeStylePool::codeStyleRemoved,
                this, &ICodeStylePreferences::codeStyleRemoved);
    }
}

ICodeStylePreferences *ICodeStylePreferences::currentDelegate() const
{
    return d->m_currentDelegate;
}

void ICodeStylePreferences::setCurrentDelegate(ICodeStylePreferences *delegate)
{
    if (delegate && d->m_pool && !d->m_pool->codeStyles().contains(delegate)) {
        // warning
        return;
    }

    if (delegate == this || (delegate && delegate->id() == id())) {
        // warning
        return;
    }

    if (d->m_currentDelegate == delegate)
        return; // nothing changes

    if (d->m_currentDelegate) {
        disconnect(d->m_currentDelegate, &ICodeStylePreferences::currentTabSettingsChanged,
                   this, &ICodeStylePreferences::currentTabSettingsChanged);
        disconnect(d->m_currentDelegate, &ICodeStylePreferences::currentValueChanged,
                   this, &ICodeStylePreferences::currentValueChanged);
        disconnect(d->m_currentDelegate, &ICodeStylePreferences::currentPreferencesChanged,
                   this, &ICodeStylePreferences::currentPreferencesChanged);
    }
    d->m_currentDelegate = delegate;
    if (d->m_currentDelegate) {
        connect(d->m_currentDelegate, &ICodeStylePreferences::currentTabSettingsChanged,
                   this, &ICodeStylePreferences::currentTabSettingsChanged);
        connect(d->m_currentDelegate, &ICodeStylePreferences::currentValueChanged,
                this, &ICodeStylePreferences::currentValueChanged);
        connect(d->m_currentDelegate, &ICodeStylePreferences::currentPreferencesChanged,
                   this, &ICodeStylePreferences::currentPreferencesChanged);
    }
    emit currentDelegateChanged(d->m_currentDelegate);
    emit currentPreferencesChanged(currentPreferences());
    emit currentTabSettingsChanged(currentTabSettings());
    emit currentValueChanged(currentValue());
}

QByteArray ICodeStylePreferences::currentDelegateId() const
{
    if (currentDelegate())
        return currentDelegate()->id();
    return id(); // or 0?
}

void ICodeStylePreferences::setCurrentDelegate(const QByteArray &id)
{
    if (d->m_pool)
        setCurrentDelegate(d->m_pool->codeStyle(id));
}

void ICodeStylePreferences::toSettings(const QString &category, QSettings *s) const
{
    Utils::toSettings(settingsSuffix(), category, s, this);
}

void ICodeStylePreferences::fromSettings(const QString &category, const QSettings *s)
{
    Utils::fromSettings(settingsSuffix(), category, s, this);
}

void ICodeStylePreferences::toMap(const QString &prefix, QVariantMap *map) const
{
    if (!currentDelegate())
        d->m_tabSettings.toMap(prefix, map);
    else
        map->insert(prefix + QLatin1String(currentPreferencesKey), currentDelegateId());
}

void ICodeStylePreferences::fromMap(const QString &prefix, const QVariantMap &map)
{
    d->m_tabSettings.fromMap(prefix, map);
    const QByteArray delegateId = map.value(prefix + QLatin1String(currentPreferencesKey)).toByteArray();
    if (delegatingPool()) {
        ICodeStylePreferences *delegate = delegatingPool()->codeStyle(delegateId);
        if (!delegateId.isEmpty() && delegate)
            setCurrentDelegate(delegate);
    }
}

void ICodeStylePreferences::codeStyleRemoved(ICodeStylePreferences *preferences)
{
    if (currentDelegate() == preferences) {
        CodeStylePool *pool = delegatingPool();
        QList<ICodeStylePreferences *> codeStyles = pool->codeStyles();
        const int idx = codeStyles.indexOf(preferences);
        ICodeStylePreferences *newCurrentPreferences = 0;
        int i = idx + 1;
        // go forward
        while (i < codeStyles.count()) {
            ICodeStylePreferences *prefs = codeStyles.at(i);
            if (prefs->id() != id()) {
                newCurrentPreferences = prefs;
                break;
            }
            i++;
        }
        // go backward if still empty
        if (!newCurrentPreferences) {
            i = idx - 1;
            while (i >= 0) {
                ICodeStylePreferences *prefs = codeStyles.at(i);
                if (prefs->id() != id()) {
                    newCurrentPreferences = prefs;
                    break;
                }
                i--;
            }
        }
        setCurrentDelegate(newCurrentPreferences);
    }
}

