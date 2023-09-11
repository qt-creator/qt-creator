// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "icodestylepreferences.h"
#include "codestylepool.h"
#include "tabsettings.h"

#include <coreplugin/icore.h>

using namespace Utils;

static const char currentPreferencesKey[] = "CurrentPreferences";

namespace TextEditor {
namespace Internal {

class ICodeStylePreferencesPrivate
{
public:
    CodeStylePool *m_pool = nullptr;
    ICodeStylePreferences *m_currentDelegate = nullptr;
    TabSettings m_tabSettings;
    QByteArray m_id;
    QString m_displayName;
    bool m_readOnly = false;
    bool m_temporarilyReadOnly = false;
    bool m_isAdditionalTabDisabled = false;
    Key m_settingsSuffix;
};

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

void ICodeStylePreferences::setTemporarilyReadOnly(bool on)
{
    d->m_temporarilyReadOnly = on;
}

bool ICodeStylePreferences::isTemporarilyReadOnly() const
{
    return d->m_temporarilyReadOnly;
}

bool ICodeStylePreferences::isAdditionalTabDisabled() const
{
    return d->m_isAdditionalTabDisabled;
}

void ICodeStylePreferences::setIsAdditionalTabDisabled(bool on)
{
    d->m_isAdditionalTabDisabled = on;
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
    auto prefs = (ICodeStylePreferences *)this;
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

    setCurrentDelegate(nullptr);
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

void ICodeStylePreferences::setSettingsSuffix(const Key &suffix)
{
    d->m_settingsSuffix = suffix;
}

void ICodeStylePreferences::toSettings(const Key &category) const
{
    Utils::storeToSettings(category + d->m_settingsSuffix, Core::ICore::settings(), toMap());
}

void ICodeStylePreferences::fromSettings(const Key &category)
{
    fromMap(Utils::storeFromSettings(category + d->m_settingsSuffix, Core::ICore::settings()));
}

Store ICodeStylePreferences::toMap() const
{
    if (!currentDelegate())
        return d->m_tabSettings.toMap();
    return {{currentPreferencesKey, currentDelegateId()}};
}

void ICodeStylePreferences::fromMap(const Store &map)
{
    d->m_tabSettings.fromMap(map);
    const QByteArray delegateId = map.value(currentPreferencesKey).toByteArray();
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
        ICodeStylePreferences *newCurrentPreferences = nullptr;
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

} // TextEditor
