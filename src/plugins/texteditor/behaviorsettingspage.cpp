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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "behaviorsettingspage.h"

#include "behaviorsettings.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "extraencodingsettings.h"
#include "ui_behaviorsettingspage.h"

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

    TabSettings m_tabSettings;
    StorageSettings m_storageSettings;
    BehaviorSettings m_behaviorSettings;
    ExtraEncodingSettings m_extraEncodingSettings;

    QString m_searchKeywords;
};

BehaviorSettingsPage::BehaviorSettingsPagePrivate::BehaviorSettingsPagePrivate
    (const BehaviorSettingsPageParameters &p)
    : m_parameters(p), m_page(0)
{
    if (const QSettings *s = Core::ICore::instance()->settings()) {
        m_tabSettings.fromSettings(m_parameters.settingsPrefix, s);
        m_storageSettings.fromSettings(m_parameters.settingsPrefix, s);
        m_behaviorSettings.fromSettings(m_parameters.settingsPrefix, s);
        m_extraEncodingSettings.fromSettings(m_parameters.settingsPrefix, s);
    }
}

BehaviorSettingsPage::BehaviorSettingsPage(const BehaviorSettingsPageParameters &p,
                                           QObject *parent)
  : TextEditorOptionsPage(parent),
    m_d(new BehaviorSettingsPagePrivate(p))
{
}

BehaviorSettingsPage::~BehaviorSettingsPage()
{
    delete m_d;
}

QString BehaviorSettingsPage::id() const
{
    return m_d->m_parameters.id;
}

QString BehaviorSettingsPage::displayName() const
{
    return m_d->m_parameters.displayName;
}

QWidget *BehaviorSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_d->m_page = new Ui::BehaviorSettingsPage;
    m_d->m_page->setupUi(w);

    settingsToUI();

    if (m_d->m_searchKeywords.isEmpty())
        m_d->m_searchKeywords = m_d->m_page->behaviorWidget->collectUiKeywords();

    return w;
}

void BehaviorSettingsPage::apply()
{
    if (!m_d->m_page) // page was never shown
        return;

    TabSettings newTabSettings;
    StorageSettings newStorageSettings;
    BehaviorSettings newBehaviorSettings;
    ExtraEncodingSettings newExtraEncodingSettings;

    settingsFromUI(&newTabSettings, &newStorageSettings, &newBehaviorSettings,
                   &newExtraEncodingSettings);

    QSettings *s = Core::ICore::instance()->settings();

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

    if (newBehaviorSettings != m_d->m_behaviorSettings) {
        m_d->m_behaviorSettings = newBehaviorSettings;
        if (s)
            m_d->m_behaviorSettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit behaviorSettingsChanged(newBehaviorSettings);
    }

    if (newExtraEncodingSettings != m_d->m_extraEncodingSettings) {
        m_d->m_extraEncodingSettings = newExtraEncodingSettings;
        if (s)
            m_d->m_extraEncodingSettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit extraEncodingSettingsChanged(newExtraEncodingSettings);
    }

    if (s) {
        s->setValue(QLatin1String(Core::Constants::SETTINGS_DEFAULTTEXTENCODING),
                    m_d->m_page->behaviorWidget->assignedCodec()->name());
    }
}

void BehaviorSettingsPage::settingsFromUI(TabSettings *tabSettings,
                                          StorageSettings *storageSettings,
                                          BehaviorSettings *behaviorSettings,
                                          ExtraEncodingSettings *extraEncodingSettings) const
{
    m_d->m_page->behaviorWidget->assignedTabSettings(tabSettings);
    m_d->m_page->behaviorWidget->assignedStorageSettings(storageSettings);
    m_d->m_page->behaviorWidget->assignedBehaviorSettings(behaviorSettings);
    m_d->m_page->behaviorWidget->assignedExtraEncodingSettings(extraEncodingSettings);
}

void BehaviorSettingsPage::settingsToUI()
{
    m_d->m_page->behaviorWidget->setAssignedTabSettings(m_d->m_tabSettings);
    m_d->m_page->behaviorWidget->setAssignedStorageSettings(m_d->m_storageSettings);
    m_d->m_page->behaviorWidget->setAssignedBehaviorSettings(m_d->m_behaviorSettings);
    m_d->m_page->behaviorWidget->setAssignedExtraEncodingSettings(m_d->m_extraEncodingSettings);
    m_d->m_page->behaviorWidget->setAssignedCodec(
        Core::EditorManager::instance()->defaultTextCodec());
}

void BehaviorSettingsPage::finish()
{
    if (!m_d->m_page) // page was never shown
        return;
    delete m_d->m_page;
    m_d->m_page = 0;
}

const TabSettings &BehaviorSettingsPage::tabSettings() const
{
    return m_d->m_tabSettings;
}

const StorageSettings &BehaviorSettingsPage::storageSettings() const
{
    return m_d->m_storageSettings;
}

const BehaviorSettings &BehaviorSettingsPage::behaviorSettings() const
{
    return m_d->m_behaviorSettings;
}

const ExtraEncodingSettings &BehaviorSettingsPage::extraEncodingSettings() const
{
    return m_d->m_extraEncodingSettings;
}

bool BehaviorSettingsPage::matches(const QString &s) const
{
    return m_d->m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
