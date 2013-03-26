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
    QString m_id;
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

QString ICodeStylePreferences::id() const
{
    return d->m_id;
}

void ICodeStylePreferences::setId(const QString &name)
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
        disconnect(d->m_pool, SIGNAL(codeStyleRemoved(ICodeStylePreferences*)),
                this, SLOT(slotCodeStyleRemoved(ICodeStylePreferences*)));
    }
    d->m_pool = pool;
    if (d->m_pool) {
        connect(d->m_pool, SIGNAL(codeStyleRemoved(ICodeStylePreferences*)),
                this, SLOT(slotCodeStyleRemoved(ICodeStylePreferences*)));
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
        disconnect(d->m_currentDelegate, SIGNAL(currentTabSettingsChanged(TextEditor::TabSettings)),
                   this, SIGNAL(currentTabSettingsChanged(TextEditor::TabSettings)));
        disconnect(d->m_currentDelegate, SIGNAL(currentValueChanged(QVariant)),
                   this, SIGNAL(currentValueChanged(QVariant)));
        disconnect(d->m_currentDelegate, SIGNAL(currentPreferencesChanged(TextEditor::ICodeStylePreferences*)),
                   this, SIGNAL(currentPreferencesChanged(TextEditor::ICodeStylePreferences*)));
    }
    d->m_currentDelegate = delegate;
    if (d->m_currentDelegate) {
        connect(d->m_currentDelegate, SIGNAL(currentTabSettingsChanged(TextEditor::TabSettings)),
                   this, SIGNAL(currentTabSettingsChanged(TextEditor::TabSettings)));
        connect(d->m_currentDelegate, SIGNAL(currentValueChanged(QVariant)),
                this, SIGNAL(currentValueChanged(QVariant)));
        connect(d->m_currentDelegate, SIGNAL(currentPreferencesChanged(TextEditor::ICodeStylePreferences*)),
                   this, SIGNAL(currentPreferencesChanged(TextEditor::ICodeStylePreferences*)));
    }
    emit currentDelegateChanged(d->m_currentDelegate);
    emit currentPreferencesChanged(currentPreferences());
    emit currentTabSettingsChanged(currentTabSettings());
    emit currentValueChanged(currentValue());
}

QString ICodeStylePreferences::currentDelegateId() const
{
    if (currentDelegate())
        return currentDelegate()->id();
    return id(); // or 0?
}

void ICodeStylePreferences::setCurrentDelegate(const QString &id)
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
    const QString delegateId = map.value(prefix + QLatin1String(currentPreferencesKey)).toString();
    if (delegatingPool()) {
        ICodeStylePreferences *delegate = delegatingPool()->codeStyle(delegateId);
        if (!delegateId.isEmpty() && delegate)
            setCurrentDelegate(delegate);
    }
}

void ICodeStylePreferences::slotCodeStyleRemoved(ICodeStylePreferences *preferences)
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

