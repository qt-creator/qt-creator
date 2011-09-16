/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "behaviorsettingspage.h"

#include "behaviorsettings.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "extraencodingsettings.h"
#include "ui_behaviorsettingspage.h"
#include "tabpreferences.h"
#include "texteditorconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QSettings>
#include <QtCore/QTextCodec>

using namespace TextEditor;

struct BehaviorSettingsPage::BehaviorSettingsPagePrivate
{
    explicit BehaviorSettingsPagePrivate(const BehaviorSettingsPageParameters &p);

    const BehaviorSettingsPageParameters m_parameters;
    Ui::BehaviorSettingsPage *m_page;

    void init();

    TabPreferences *m_tabPreferences;
    TabPreferences *m_pageTabPreferences;
    StorageSettings m_storageSettings;
    BehaviorSettings m_behaviorSettings;
    ExtraEncodingSettings m_extraEncodingSettings;

    QString m_searchKeywords;
};

BehaviorSettingsPage::BehaviorSettingsPagePrivate::BehaviorSettingsPagePrivate
    (const BehaviorSettingsPageParameters &p)
    : m_parameters(p), m_page(0), m_pageTabPreferences(0)
{
}

void BehaviorSettingsPage::BehaviorSettingsPagePrivate::init()
{
    if (const QSettings *s = Core::ICore::instance()->settings()) {
        m_tabPreferences->fromSettings(m_parameters.settingsPrefix, s);
        m_storageSettings.fromSettings(m_parameters.settingsPrefix, s);
        m_behaviorSettings.fromSettings(m_parameters.settingsPrefix, s);
        m_extraEncodingSettings.fromSettings(m_parameters.settingsPrefix, s);
    }
}

BehaviorSettingsPage::BehaviorSettingsPage(const BehaviorSettingsPageParameters &p,
                                           QObject *parent)
  : TextEditorOptionsPage(parent),
    d(new BehaviorSettingsPagePrivate(p))
{
    d->m_tabPreferences = new TabPreferences(QList<IFallbackPreferences *>(), this);
    d->m_tabPreferences->setDisplayName(tr("Global", "Settings"));
    d->m_tabPreferences->setId(Constants::GLOBAL_SETTINGS_ID);
    d->init();
}

BehaviorSettingsPage::~BehaviorSettingsPage()
{
    delete d;
}

QString BehaviorSettingsPage::id() const
{
    return d->m_parameters.id;
}

QString BehaviorSettingsPage::displayName() const
{
    return d->m_parameters.displayName;
}

QWidget *BehaviorSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    d->m_page = new Ui::BehaviorSettingsPage;
    d->m_page->setupUi(w);
    d->m_pageTabPreferences = new TabPreferences(d->m_tabPreferences->fallbacks(), w);
    d->m_pageTabPreferences->setSettings(d->m_tabPreferences->settings());
    d->m_pageTabPreferences->setCurrentFallback(d->m_tabPreferences->currentFallback());
    d->m_page->behaviorWidget->setTabPreferences(d->m_pageTabPreferences);

    settingsToUI();

    if (d->m_searchKeywords.isEmpty())
        d->m_searchKeywords = d->m_page->behaviorWidget->collectUiKeywords();

    return w;
}

void BehaviorSettingsPage::apply()
{
    if (!d->m_page) // page was never shown
        return;

    StorageSettings newStorageSettings;
    BehaviorSettings newBehaviorSettings;
    ExtraEncodingSettings newExtraEncodingSettings;

    settingsFromUI(&newStorageSettings, &newBehaviorSettings,
                   &newExtraEncodingSettings);

    QSettings *s = Core::ICore::instance()->settings();

    if (d->m_tabPreferences->settings() != d->m_pageTabPreferences->settings()) {
        d->m_tabPreferences->setSettings(d->m_pageTabPreferences->settings());
        if (s)
            d->m_tabPreferences->toSettings(d->m_parameters.settingsPrefix, s);
    }

    if (d->m_tabPreferences->currentFallback() != d->m_pageTabPreferences->currentFallback()) {
        d->m_tabPreferences->setCurrentFallback(d->m_pageTabPreferences->currentFallback());
        if (s)
            d->m_tabPreferences->toSettings(d->m_parameters.settingsPrefix, s);
    }

    if (newStorageSettings != d->m_storageSettings) {
        d->m_storageSettings = newStorageSettings;
        if (s)
            d->m_storageSettings.toSettings(d->m_parameters.settingsPrefix, s);

        emit storageSettingsChanged(newStorageSettings);
    }

    if (newBehaviorSettings != d->m_behaviorSettings) {
        d->m_behaviorSettings = newBehaviorSettings;
        if (s)
            d->m_behaviorSettings.toSettings(d->m_parameters.settingsPrefix, s);

        emit behaviorSettingsChanged(newBehaviorSettings);
    }

    if (newExtraEncodingSettings != d->m_extraEncodingSettings) {
        d->m_extraEncodingSettings = newExtraEncodingSettings;
        if (s)
            d->m_extraEncodingSettings.toSettings(d->m_parameters.settingsPrefix, s);

        emit extraEncodingSettingsChanged(newExtraEncodingSettings);
    }

    if (s) {
        s->setValue(QLatin1String(Core::Constants::SETTINGS_DEFAULTTEXTENCODING),
                    d->m_page->behaviorWidget->assignedCodec()->name());
    }
}

void BehaviorSettingsPage::settingsFromUI(StorageSettings *storageSettings,
                                          BehaviorSettings *behaviorSettings,
                                          ExtraEncodingSettings *extraEncodingSettings) const
{
    d->m_page->behaviorWidget->assignedStorageSettings(storageSettings);
    d->m_page->behaviorWidget->assignedBehaviorSettings(behaviorSettings);
    d->m_page->behaviorWidget->assignedExtraEncodingSettings(extraEncodingSettings);
}

void BehaviorSettingsPage::settingsToUI()
{
    d->m_page->behaviorWidget->setAssignedStorageSettings(d->m_storageSettings);
    d->m_page->behaviorWidget->setAssignedBehaviorSettings(d->m_behaviorSettings);
    d->m_page->behaviorWidget->setAssignedExtraEncodingSettings(d->m_extraEncodingSettings);
    d->m_page->behaviorWidget->setAssignedCodec(
        Core::EditorManager::instance()->defaultTextCodec());
}

void BehaviorSettingsPage::finish()
{
    if (!d->m_page) // page was never shown
        return;
    delete d->m_page;
    d->m_page = 0;
}

const StorageSettings &BehaviorSettingsPage::storageSettings() const
{
    return d->m_storageSettings;
}

const BehaviorSettings &BehaviorSettingsPage::behaviorSettings() const
{
    return d->m_behaviorSettings;
}

const ExtraEncodingSettings &BehaviorSettingsPage::extraEncodingSettings() const
{
    return d->m_extraEncodingSettings;
}

TabPreferences *BehaviorSettingsPage::tabPreferences() const
{
    return d->m_tabPreferences;
}

bool BehaviorSettingsPage::matches(const QString &s) const
{
    return d->m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
