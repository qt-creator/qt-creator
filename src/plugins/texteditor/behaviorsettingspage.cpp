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

#include <QSettings>
#include <QTextCodec>

using namespace TextEditor;

struct BehaviorSettingsPage::BehaviorSettingsPagePrivate
{
    explicit BehaviorSettingsPagePrivate(const BehaviorSettingsPageParameters &p);

    const BehaviorSettingsPageParameters m_parameters;
    Internal::Ui::BehaviorSettingsPage *m_page;

    void init();

    CodeStylePool *m_defaultCodeStylePool;
    SimpleCodeStylePreferences *m_codeStyle;
    SimpleCodeStylePreferences *m_pageCodeStyle;
    TypingSettings m_typingSettings;
    StorageSettings m_storageSettings;
    BehaviorSettings m_behaviorSettings;
    ExtraEncodingSettings m_extraEncodingSettings;

    QString m_searchKeywords;
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
    d->m_codeStyle->setId(QLatin1String(Constants::GLOBAL_SETTINGS_ID));

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

QWidget *BehaviorSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    d->m_page = new Internal::Ui::BehaviorSettingsPage;
    d->m_page->setupUi(w);
    if (Utils::HostOsInfo::isMacHost())
        d->m_page->gridLayout->setContentsMargins(-1, 0, -1, 0); // don't ask.
    d->m_pageCodeStyle = new SimpleCodeStylePreferences(w);
    d->m_pageCodeStyle->setDelegatingPool(d->m_codeStyle->delegatingPool());
    d->m_pageCodeStyle->setTabSettings(d->m_codeStyle->tabSettings());
    d->m_pageCodeStyle->setCurrentDelegate(d->m_codeStyle->currentDelegate());
    d->m_page->behaviorWidget->setCodeStyle(d->m_pageCodeStyle);

    TabSettingsWidget *tabSettingsWidget = d->m_page->behaviorWidget->tabSettingsWidget();
    tabSettingsWidget->setCodingStyleWarningVisible(true);
    connect(tabSettingsWidget, SIGNAL(codingStyleLinkClicked(TextEditor::TabSettingsWidget::CodingStyleLink)),
            this, SLOT(openCodingStylePreferences(TextEditor::TabSettingsWidget::CodingStyleLink)));

    settingsToUI();

    if (d->m_searchKeywords.isEmpty())
        d->m_searchKeywords = d->m_page->behaviorWidget->collectUiKeywords();

    return w;
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

bool BehaviorSettingsPage::matches(const QString &s) const
{
    return d->m_searchKeywords.contains(s, Qt::CaseInsensitive);
}


void BehaviorSettingsPage::openCodingStylePreferences(TabSettingsWidget::CodingStyleLink link)
{
    switch (link) {
    case TextEditor::TabSettingsWidget::CppLink:
        Core::ICore::showOptionsDialog(CppTools::Constants::CPP_SETTINGS_CATEGORY,
                                       CppTools::Constants::CPP_CODE_STYLE_SETTINGS_ID);
        break;
    case TextEditor::TabSettingsWidget::QtQuickLink:
        Core::ICore::showOptionsDialog(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML,
                                       QmlJSTools::Constants::QML_JS_CODE_STYLE_SETTINGS_ID);
        break;
    }
}
