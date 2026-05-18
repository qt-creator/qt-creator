// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "behaviorsettingswidget.h"

#include "behaviorsettings.h"
#include "extraencodingsettings.h"
#include "simplecodestylepreferenceswidget.h"
#include "storagesettings.h"
#include "tabsettingswidget.h"
#include "typingsettings.h"

#include <utils/layoutbuilder.h>

namespace TextEditor {

BehaviorSettingsWidget::BehaviorSettingsWidget(TypingSettings *typingSettings,
                                               StorageSettings *storageSettings,
                                               BehaviorSettings *behaviorSettings,
                                               ExtraEncodingSettings *encodingSettings,
                                               QWidget *parent)
    : QWidget(parent)
{
    tabPreferencesWidget = new SimpleCodeStylePreferencesWidget(this);

    using namespace Layouting;

    Column {
        tabPreferencesWidget,
        typingSettings,
        storageSettings,
        encodingSettings,
        behaviorSettings,
        st,
        noMargin,
    }.attachTo(this);
}

void BehaviorSettingsWidget::setCodeStyle(ICodeStylePreferences *preferences)
{
    tabPreferencesWidget->setPreferences(preferences);
}

TabSettingsWidget *BehaviorSettingsWidget::tabSettingsWidget() const
{
    return tabPreferencesWidget->tabSettingsWidget();
}

} // TextEditor
