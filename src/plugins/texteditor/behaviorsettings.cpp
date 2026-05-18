// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "behaviorsettings.h"

#include "behaviorsettingswidget.h"
#include "codestylepool.h"
#include "extraencodingsettings.h"
#include "simplecodestylepreferences.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "tabsettingswidget.h"
#include "texteditorconstants.h"
#include "texteditorsettings.h"
#include "texteditortr.h"
#include "typingsettings.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>

// for opening the respective coding style preferences
#include <cppeditor/cppeditorconstants.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <QGridLayout>
#include <QSpacerItem>

using namespace Utils;

namespace TextEditor {

BehaviorSettings::BehaviorSettings(const Key &keyPrefix)
{
    setSettingsGroup("textBehaviorSettings");

    mouseHiding.setSettingsKey(keyPrefix + "MouseHiding");
    mouseHiding.setDefaultValue(true);
    mouseHiding.setVisible(!HostOsInfo::isMacHost());
    mouseHiding.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    mouseHiding.setLabelText(Tr::tr("Hide mouse cursor while typing"));

    mouseNavigation.setSettingsKey(keyPrefix + "MouseNavigation");
    mouseNavigation.setDefaultValue(true);
    mouseNavigation.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    mouseNavigation.setLabelText(Tr::tr("Enable &mouse navigation"));

    scrollWheelZooming.setSettingsKey(keyPrefix + "ScrollWheelZooming");
    scrollWheelZooming.setDefaultValue(true);
    scrollWheelZooming.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    scrollWheelZooming.setLabelText(Tr::tr("Enable scroll &wheel zooming"));

    constrainHoverTooltips.setSettingsKey(keyPrefix + "ConstrainTooltips");
    constrainHoverTooltips.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    constrainHoverTooltips.setUseDataAsSavedValue();
    constrainHoverTooltips.setLabelText(Tr::tr("Show help tooltips using the mouse:"));
    constrainHoverTooltips.addOption({
        Tr::tr("On Mouseover"),
        Tr::tr("Displays context-sensitive help or type information on mouseover."),
        QVariant(false)
    });
    constrainHoverTooltips.addOption({
        Tr::tr("On Shift+Mouseover"),
        Tr::tr("Displays context-sensitive help or type information on Shift+Mouseover."),
        QVariant(true)
    });
    constrainHoverTooltips.setLabelText(Tr::tr("Show help tooltips using the mouse:"));

    camelCaseNavigation.setSettingsKey(keyPrefix + "CamelCaseNavigation");
    camelCaseNavigation.setDefaultValue(true);
    camelCaseNavigation.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    camelCaseNavigation.setLabelText(Tr::tr("Enable built-in camel case &navigation"));

    keyboardTooltips.setSettingsKey(keyPrefix + "KeyboardTooltips");
    keyboardTooltips.setDefaultValue(false);
    keyboardTooltips.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    keyboardTooltips.setLabelText(Tr::tr("Show help tooltips using keyboard shortcut (Alt)"));
    keyboardTooltips.setToolTip(Tr::tr("Pressing Alt displays context-sensitive help or type information as tooltips."));

    smartSelectionChanging.setSettingsKey(keyPrefix + "SmartSelectionChanging");
    smartSelectionChanging.setDefaultValue(true);
    smartSelectionChanging.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    smartSelectionChanging.setLabelText(Tr::tr("Enable smart selection changing"));
    smartSelectionChanging.setToolTip(Tr::tr("Using Select Block Up / Down actions will now provide smarter selections."));
}

void BehaviorSettings::setData(const BehaviorSettingsData &data)
{
    mouseHiding.setValue(data.m_mouseHiding);
    mouseNavigation.setValue(data.m_mouseNavigation);
    scrollWheelZooming.setValue(data.m_scrollWheelZooming);
    constrainHoverTooltips.setValue(data.m_constrainHoverTooltips);
    camelCaseNavigation.setValue(data.m_camelCaseNavigation);
    keyboardTooltips.setValue(data.m_keyboardTooltips);
    smartSelectionChanging.setValue(data.m_smartSelectionChanging);
}

void BehaviorSettings::apply()
{
    AspectContainer::apply();
    AspectContainer::writeSettings();
}

BehaviorSettingsData BehaviorSettings::data() const
{
    BehaviorSettingsData d;
    d.m_mouseHiding = mouseHiding();
    d.m_mouseNavigation = mouseNavigation();
    d.m_scrollWheelZooming = scrollWheelZooming();
    d.m_constrainHoverTooltips = constrainHoverTooltips();
    d.m_camelCaseNavigation = camelCaseNavigation();
    d.m_keyboardTooltips = keyboardTooltips();
    d.m_smartSelectionChanging = smartSelectionChanging();
    return d;
}

BehaviorSettings &globalBehaviorSettings()
{
    static BehaviorSettings theGlobalBehaviorSettings;
    return theGlobalBehaviorSettings;
}

// BehaviorSettingsPage

class BehaviorSettingsPage : public Core::IOptionsPage
{
public:
    BehaviorSettingsPage();
    ~BehaviorSettingsPage() override;

    ICodeStylePreferences *codeStyle() const;
    CodeStylePool *codeStylePool() const;

private:
    class BehaviorSettingsPagePrivate *d;
};

class BehaviorSettingsPagePrivate : public QObject
{
public:
    BehaviorSettingsPagePrivate();

    const Key m_settingsPrefix{"text"};
    TextEditor::BehaviorSettingsWidget *m_behaviorWidget = nullptr;

    CodeStylePool *m_defaultCodeStylePool = nullptr;
    SimpleCodeStylePreferences *m_codeStyle = nullptr;
    SimpleCodeStylePreferences *m_pageCodeStyle = nullptr;
};

BehaviorSettingsPagePrivate::BehaviorSettingsPagePrivate()
{
    // global tab preferences for all other languages
    m_codeStyle = new SimpleCodeStylePreferences(this);
    m_codeStyle->setDisplayName(Tr::tr("Global", "Settings"));
    m_codeStyle->setId(Constants::GLOBAL_SETTINGS_ID);

    // default pool for all other languages
    m_defaultCodeStylePool = new CodeStylePool(nullptr, this); // Any language
    m_defaultCodeStylePool->addCodeStyle(m_codeStyle);

    m_codeStyle->fromSettings(m_settingsPrefix);
}

class BehaviorSettingsWidgetImpl : public Core::IOptionsPageWidget
{
public:
    BehaviorSettingsWidgetImpl(BehaviorSettingsPagePrivate *d) : d(d)
    {
        d->m_behaviorWidget = new BehaviorSettingsWidget(&globalTypingSettings(),
                                                         &globalStorageSettings(),
                                                         &globalBehaviorSettings(),
                                                         &globalExtraEncodingSettings(),
                                                         this);

        auto verticalSpacer = new QSpacerItem(20, 13, QSizePolicy::Minimum, QSizePolicy::Expanding);

        auto gridLayout = new QGridLayout(this);
        if (Utils::HostOsInfo::isMacHost())
            gridLayout->setContentsMargins(-1, 0, -1, 0); // don't ask.
        gridLayout->addWidget(d->m_behaviorWidget, 0, 0, 1, 1);
        gridLayout->addItem(verticalSpacer, 1, 0, 1, 1);

        d->m_pageCodeStyle = new SimpleCodeStylePreferences(this);
        d->m_pageCodeStyle->setDelegatingPool(d->m_codeStyle->delegatingPool());
        d->m_pageCodeStyle->setTabSettings(d->m_codeStyle->tabSettings());
        d->m_pageCodeStyle->setCurrentDelegate(d->m_codeStyle->currentDelegate());
        d->m_behaviorWidget->setCodeStyle(d->m_pageCodeStyle);

        TabSettingsWidget *tabSettingsWidget = d->m_behaviorWidget->tabSettingsWidget();
        tabSettingsWidget->setCodingStyleWarningVisible(true);
        connect(tabSettingsWidget, &TabSettingsWidget::codingStyleLinkClicked,
                this, [] (TabSettingsWidget::CodingStyleLink link) {
            switch (link) {
            case TabSettingsWidget::CppLink:
                Core::ICore::showSettings(CppEditor::Constants::CPP_CODE_STYLE_SETTINGS_ID);
                break;
            case TabSettingsWidget::QtQuickLink:
                Core::ICore::showSettings(QmlJSTools::Constants::QML_JS_CODE_STYLE_SETTINGS_ID);
                break;
            }
        });

        installMarkSettingsDirtyTriggerRecursively(d->m_pageCodeStyle);
        installCheckSettingsDirtyTrigger(&globalTypingSettings());
        installCheckSettingsDirtyTrigger(&globalStorageSettings());
        installCheckSettingsDirtyTrigger(&globalBehaviorSettings());
        installCheckSettingsDirtyTrigger(&globalExtraEncodingSettings());
    }

    bool isDirty() const;
    void apply() final;

    BehaviorSettingsPagePrivate *d;
};

BehaviorSettingsPage::BehaviorSettingsPage()
  : d(new BehaviorSettingsPagePrivate)
{
    setId(Constants::TEXT_EDITOR_BEHAVIOR_SETTINGS);
    setDisplayName(Tr::tr("Behavior"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setWidgetCreator([this] { return new BehaviorSettingsWidgetImpl(d); });
}

BehaviorSettingsPage::~BehaviorSettingsPage()
{
    delete d;
}

bool BehaviorSettingsWidgetImpl::isDirty() const
{
    if (globalTypingSettings().isDirty())
        return true;
    if (globalStorageSettings().isDirty())
        return true;
    if (globalBehaviorSettings().isDirty())
        return true;
    if (globalExtraEncodingSettings().isDirty())
        return true;

    if (d->m_codeStyle->tabSettings() != d->m_pageCodeStyle->tabSettings())
        return true;
    if (d->m_codeStyle->currentDelegate() != d->m_pageCodeStyle->currentDelegate())
        return true;

    return false;
}

void BehaviorSettingsWidgetImpl::apply()
{
    if (!d->m_behaviorWidget) // page was never shown
        return;

    globalTypingSettings().apply();
    globalBehaviorSettings().apply();
    globalStorageSettings().apply();
    globalExtraEncodingSettings().apply();

    if (d->m_codeStyle->tabSettings() != d->m_pageCodeStyle->tabSettings()) {
        d->m_codeStyle->setTabSettings(d->m_pageCodeStyle->tabSettings());
        d->m_codeStyle->toSettings(d->m_settingsPrefix);
    }

    if (d->m_codeStyle->currentDelegate() != d->m_pageCodeStyle->currentDelegate()) {
        d->m_codeStyle->setCurrentDelegate(d->m_pageCodeStyle->currentDelegate());
        d->m_codeStyle->toSettings(d->m_settingsPrefix);
    }
}

ICodeStylePreferences *BehaviorSettingsPage::codeStyle() const
{
    return d->m_codeStyle;
}

CodeStylePool *BehaviorSettingsPage::codeStylePool() const
{
    return d->m_defaultCodeStylePool;
}

static BehaviorSettingsPage &behaviorSettingsPage()
{
    static BehaviorSettingsPage theBehaviorSettingsPage;
    return theBehaviorSettingsPage;
}

ICodeStylePreferences *globalCodeStyle()
{
    return behaviorSettingsPage().codeStyle();
}

CodeStylePool *globalCodeStylePool()
{
    return behaviorSettingsPage().codeStylePool();
}

void Internal::setupBehaviorSettings()
{
    globalBehaviorSettings().readSettings();
    (void) behaviorSettingsPage();
}

} // namespace TextEditor
