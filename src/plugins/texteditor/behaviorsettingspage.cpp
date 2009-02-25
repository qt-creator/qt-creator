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

#include "behaviorsettingspage.h"
#include "interactionsettings.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "ui_behaviorsettingspage.h"

#include <coreplugin/icore.h>

#include <QtCore/QSettings>

using namespace TextEditor;

struct BehaviorSettingsPage::BehaviorSettingsPagePrivate
{
    explicit BehaviorSettingsPagePrivate(const BehaviorSettingsPageParameters &p);

    const BehaviorSettingsPageParameters m_parameters;
    Ui::BehaviorSettingsPage m_page;
    TabSettings m_tabSettings;
    StorageSettings m_storageSettings;
    InteractionSettings m_interactionSettings;
};

BehaviorSettingsPage::BehaviorSettingsPagePrivate::BehaviorSettingsPagePrivate
    (const BehaviorSettingsPageParameters &p)
  : m_parameters(p)
{
    if (const QSettings *s = Core::ICore::instance()->settings()) {
        m_tabSettings.fromSettings(m_parameters.settingsPrefix, s);
        m_storageSettings.fromSettings(m_parameters.settingsPrefix, s);
        m_interactionSettings.fromSettings(m_parameters.settingsPrefix, s);
    }
}

BehaviorSettingsPage::BehaviorSettingsPage(const BehaviorSettingsPageParameters &p,
                                 QObject *parent)
  : Core::IOptionsPage(parent),
    m_d(new BehaviorSettingsPagePrivate(p))
{
}

BehaviorSettingsPage::~BehaviorSettingsPage()
{
    delete m_d;
}

QString BehaviorSettingsPage::name() const
{
    return m_d->m_parameters.name;
}

QString BehaviorSettingsPage::category() const
{
    return m_d->m_parameters.category;
}

QString BehaviorSettingsPage::trCategory() const
{
    return m_d->m_parameters.trCategory;
}

QWidget *BehaviorSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_d->m_page.setupUi(w);
    settingsToUI();
    return w;
}

void BehaviorSettingsPage::apply()
{
    TabSettings newTabSettings;
    StorageSettings newStorageSettings;
    InteractionSettings newInteractionSettings;

    settingsFromUI(newTabSettings, newStorageSettings, newInteractionSettings);

    Core::ICore *core = Core::ICore::instance();
    QSettings *s = core->settings();

    if (newTabSettings != m_d->m_tabSettings) {
        m_d->m_tabSettings = newTabSettings;
        if (s)
            m_d->m_tabSettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit tabSettingsChanged(newTabSettings);
    }

    if (newStorageSettings != m_d->m_storageSettings) {
        m_d->m_storageSettings = newStorageSettings;
        if (s)
            m_d->m_storageSettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit storageSettingsChanged(newStorageSettings);
    }

    if (newInteractionSettings != m_d->m_interactionSettings) {
        m_d->m_interactionSettings = newInteractionSettings;
        if (s)
            m_d->m_interactionSettings.toSettings(m_d->m_parameters.settingsPrefix, s);
    }
}

void BehaviorSettingsPage::settingsFromUI(TabSettings &tabSettings,
                                         StorageSettings &storageSettings,
                                         InteractionSettings &interactionSettings) const
{
    tabSettings.m_spacesForTabs = m_d->m_page.insertSpaces->isChecked();
    tabSettings.m_autoIndent = m_d->m_page.autoIndent->isChecked();
    tabSettings.m_smartBackspace = m_d->m_page.smartBackspace->isChecked();
    tabSettings.m_tabSize = m_d->m_page.tabSize->value();
    tabSettings.m_indentSize = m_d->m_page.indentSize->value();

    storageSettings.m_cleanWhitespace = m_d->m_page.cleanWhitespace->isChecked();
    storageSettings.m_inEntireDocument = m_d->m_page.inEntireDocument->isChecked();
    storageSettings.m_cleanIndentation = m_d->m_page.cleanIndentation->isChecked();
    storageSettings.m_addFinalNewLine = m_d->m_page.addFinalNewLine->isChecked();

    interactionSettings.m_useVim = m_d->m_page.useVim->isChecked();
}

void BehaviorSettingsPage::settingsToUI()
{
    const TabSettings &tabSettings = m_d->m_tabSettings;
    m_d->m_page.insertSpaces->setChecked(tabSettings.m_spacesForTabs);
    m_d->m_page.autoIndent->setChecked(tabSettings.m_autoIndent);
    m_d->m_page.smartBackspace->setChecked(tabSettings.m_smartBackspace);
    m_d->m_page.tabSize->setValue(tabSettings.m_tabSize);
    m_d->m_page.indentSize->setValue(tabSettings.m_indentSize);

    const StorageSettings &storageSettings = m_d->m_storageSettings;
    m_d->m_page.cleanWhitespace->setChecked(storageSettings.m_cleanWhitespace);
    m_d->m_page.inEntireDocument->setChecked(storageSettings.m_inEntireDocument);
    m_d->m_page.cleanIndentation->setChecked(storageSettings.m_cleanIndentation);
    m_d->m_page.addFinalNewLine->setChecked(storageSettings.m_addFinalNewLine);

    const InteractionSettings &interactionSettings = m_d->m_interactionSettings;
    m_d->m_page.useVim->setChecked(interactionSettings.m_useVim);
}

TabSettings BehaviorSettingsPage::tabSettings() const
{
    return m_d->m_tabSettings;
}

StorageSettings BehaviorSettingsPage::storageSettings() const
{
    return m_d->m_storageSettings;
}

InteractionSettings BehaviorSettingsPage::interactionSettings() const
{
    return m_d->m_interactionSettings;
}
