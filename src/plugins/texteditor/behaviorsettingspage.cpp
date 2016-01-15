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

#include "behaviorsettingspage.h"

#include "behaviorsettings.h"
#include "typingsettings.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "extraencodingsettings.h"
#include "ui_behaviorsettingspage.h"
#include "simplecodestylepreferences.h"
#include "texteditorconstants.h"
#include "codestylepool.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/hostosinfo.h>

// for opening the respective coding style preferences
#include <cpptools/cpptoolsconstants.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <QPointer>
#include <QSettings>
#include <QTextCodec>

using namespace TextEditor;

struct BehaviorSettingsPage::BehaviorSettingsPagePrivate
{
    explicit BehaviorSettingsPagePrivate(const BehaviorSettingsPageParameters &p);

    const BehaviorSettingsPageParameters m_parameters;
    QPointer<QWidget> m_widget;
    Internal::Ui::BehaviorSettingsPage *m_page;

    void init();

    CodeStylePool *m_defaultCodeStylePool;
    SimpleCodeStylePreferences *m_codeStyle;
    SimpleCodeStylePreferences *m_pageCodeStyle;
    TypingSettings m_typingSettings;
    StorageSettings m_storageSettings;
    BehaviorSettings m_behaviorSettings;
    ExtraEncodingSettings m_extraEncodingSettings;
};

BehaviorSettingsPage::BehaviorSettingsPagePrivate::BehaviorSettingsPagePrivate
    (const BehaviorSettingsPageParameters &p)
    : m_parameters(p), m_page(0), m_pageCodeStyle(0)
{
}

void BehaviorSettingsPage::BehaviorSettingsPagePrivate::init()
{
    const QSettings *s = Core::ICore::settings();
    m_codeStyle->fromSettings(m_parameters.settingsPrefix, s);
    m_typingSettings.fromSettings(m_parameters.settingsPrefix, s);
    m_storageSettings.fromSettings(m_parameters.settingsPrefix, s);
    m_behaviorSettings.fromSettings(m_parameters.settingsPrefix, s);
    m_extraEncodingSettings.fromSettings(m_parameters.settingsPrefix, s);
}

BehaviorSettingsPage::BehaviorSettingsPage(const BehaviorSettingsPageParameters &p,
                                           QObject *parent)
  : TextEditorOptionsPage(parent),
    d(new BehaviorSettingsPagePrivate(p))
{
    // global tab preferences for all other languages
    d->m_codeStyle = new SimpleCodeStylePreferences(this);
    d->m_codeStyle->setDisplayName(tr("Global", "Settings"));
    d->m_codeStyle->setId(Constants::GLOBAL_SETTINGS_ID);

    // default pool for all other languages
    d->m_defaultCodeStylePool = new CodeStylePool(0, this); // Any language
    d->m_defaultCodeStylePool->addCodeStyle(d->m_codeStyle);
    d->init();

    setId(p.id);
    setDisplayName(p.displayName);
}

BehaviorSettingsPage::~BehaviorSettingsPage()
{
    delete d;
}

QWidget *BehaviorSettingsPage::widget()
{
    if (!d->m_widget) {
        d->m_widget = new QWidget;
        d->m_page = new Internal::Ui::BehaviorSettingsPage;
        d->m_page->setupUi(d->m_widget);
        if (Utils::HostOsInfo::isMacHost())
            d->m_page->gridLayout->setContentsMargins(-1, 0, -1, 0); // don't ask.
        d->m_pageCodeStyle = new SimpleCodeStylePreferences(d->m_widget);
        d->m_pageCodeStyle->setDelegatingPool(d->m_codeStyle->delegatingPool());
        d->m_pageCodeStyle->setTabSettings(d->m_codeStyle->tabSettings());
        d->m_pageCodeStyle->setCurrentDelegate(d->m_codeStyle->currentDelegate());
        d->m_page->behaviorWidget->setCodeStyle(d->m_pageCodeStyle);

        TabSettingsWidget *tabSettingsWidget = d->m_page->behaviorWidget->tabSettingsWidget();
        tabSettingsWidget->setCodingStyleWarningVisible(true);
        connect(tabSettingsWidget, &TabSettingsWidget::codingStyleLinkClicked,
                this, &BehaviorSettingsPage::openCodingStylePreferences);

        settingsToUI();
    }
    return d->m_widget;
}

void BehaviorSettingsPage::apply()
{
    if (!d->m_page) // page was never shown
        return;

    TypingSettings newTypingSettings;
    StorageSettings newStorageSettings;
    BehaviorSettings newBehaviorSettings;
    ExtraEncodingSettings newExtraEncodingSettings;

    settingsFromUI(&newTypingSettings, &newStorageSettings,
                   &newBehaviorSettings, &newExtraEncodingSettings);

    QSettings *s = Core::ICore::settings();

    if (d->m_codeStyle->tabSettings() != d->m_pageCodeStyle->tabSettings()) {
        d->m_codeStyle->setTabSettings(d->m_pageCodeStyle->tabSettings());
        if (s)
            d->m_codeStyle->toSettings(d->m_parameters.settingsPrefix, s);
    }

    if (d->m_codeStyle->currentDelegate() != d->m_pageCodeStyle->currentDelegate()) {
        d->m_codeStyle->setCurrentDelegate(d->m_pageCodeStyle->currentDelegate());
        if (s)
            d->m_codeStyle->toSettings(d->m_parameters.settingsPrefix, s);
    }

    if (newTypingSettings != d->m_typingSettings) {
        d->m_typingSettings = newTypingSettings;
        if (s)
            d->m_typingSettings.toSettings(d->m_parameters.settingsPrefix, s);

        emit typingSettingsChanged(newTypingSettings);
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

void BehaviorSettingsPage::settingsFromUI(TypingSettings *typingSettings,
                                          StorageSettings *storageSettings,
                                          BehaviorSettings *behaviorSettings,
                                          ExtraEncodingSettings *extraEncodingSettings) const
{
    d->m_page->behaviorWidget->assignedTypingSettings(typingSettings);
    d->m_page->behaviorWidget->assignedStorageSettings(storageSettings);
    d->m_page->behaviorWidget->assignedBehaviorSettings(behaviorSettings);
    d->m_page->behaviorWidget->assignedExtraEncodingSettings(extraEncodingSettings);
}

void BehaviorSettingsPage::settingsToUI()
{
    d->m_page->behaviorWidget->setAssignedTypingSettings(d->m_typingSettings);
    d->m_page->behaviorWidget->setAssignedStorageSettings(d->m_storageSettings);
    d->m_page->behaviorWidget->setAssignedBehaviorSettings(d->m_behaviorSettings);
    d->m_page->behaviorWidget->setAssignedExtraEncodingSettings(d->m_extraEncodingSettings);
    d->m_page->behaviorWidget->setAssignedCodec(Core::EditorManager::defaultTextCodec());
}

void BehaviorSettingsPage::finish()
{
    delete d->m_widget;
    if (!d->m_page) // page was never shown
        return;
    delete d->m_page;
    d->m_page = 0;
}

ICodeStylePreferences *BehaviorSettingsPage::codeStyle() const
{
    return d->m_codeStyle;
}

CodeStylePool *BehaviorSettingsPage::codeStylePool() const
{
    return d->m_defaultCodeStylePool;
}

const TypingSettings &BehaviorSettingsPage::typingSettings() const
{
    return d->m_typingSettings;
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


void BehaviorSettingsPage::openCodingStylePreferences(TabSettingsWidget::CodingStyleLink link)
{
    switch (link) {
    case TabSettingsWidget::CppLink:
        Core::ICore::showOptionsDialog(CppTools::Constants::CPP_CODE_STYLE_SETTINGS_ID);
        break;
    case TabSettingsWidget::QtQuickLink:
        Core::ICore::showOptionsDialog(QmlJSTools::Constants::QML_JS_CODE_STYLE_SETTINGS_ID);
        break;
    }
}
