// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "behaviorsettingswidget.h"

#include "behaviorsettings.h"
#include "extraencodingsettings.h"
#include "simplecodestylepreferenceswidget.h"
#include "storagesettings.h"
#include "tabsettingswidget.h"
#include "texteditortr.h"
#include "typingsettings.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

#include <QGroupBox>

namespace TextEditor {

struct BehaviorSettingsWidgetPrivate
{
    SimpleCodeStylePreferencesWidget *tabPreferencesWidget;
    QGroupBox *groupBoxStorageSettings;
    QGroupBox *groupBoxTyping;
    QGroupBox *groupBoxEncodings;
    QGroupBox *groupBoxMouse;

    TypingSettings *typingSettings;
    StorageSettings *storageSettings;
    BehaviorSettings *behaviorSettings;
    ExtraEncodingSettings *encodingSettings;
};

BehaviorSettingsWidget::BehaviorSettingsWidget(TypingSettings *typingSettings,
                                               StorageSettings *storageSettings,
                                               BehaviorSettings *behaviorSettings,
                                               ExtraEncodingSettings *encodingSettings,
                                               QWidget *parent)
    : QWidget(parent)
    , d(new BehaviorSettingsWidgetPrivate)
{
    d->typingSettings = typingSettings;
    d->storageSettings = storageSettings;
    d->behaviorSettings = behaviorSettings;
    d->encodingSettings = encodingSettings;

    d->tabPreferencesWidget = new SimpleCodeStylePreferencesWidget(this);

    d->groupBoxTyping = new QGroupBox(Tr::tr("Typing"));

    d->groupBoxStorageSettings = new QGroupBox(Tr::tr("Cleanups Upon Saving"));
    d->groupBoxStorageSettings->setToolTip(Tr::tr("Cleanup actions which are automatically performed "
                                              "right before the file is saved to disk."));
    d->groupBoxEncodings = new QGroupBox(Tr::tr("File Encodings"));

    d->groupBoxMouse = new QGroupBox(Tr::tr("Mouse and Keyboard"));

    using namespace Layouting;

    Row {
        Form {
            d->typingSettings->autoIndent, br,
            d->typingSettings->smartBackspaceBehavior, br,
            d->typingSettings->tabKeyBehavior, br,
            d->typingSettings->preferSingleLineComments, br,
            d->typingSettings->commentPosition, br
        }, st
    }.attachTo(d->groupBoxTyping);

    Column {
        d->storageSettings->cleanWhitespace,
            Row { Space(30), d->storageSettings->inEntireDocument },
            Row { Space(30), d->storageSettings->cleanIndentation },
            Row { Space(30), d->storageSettings->skipTrailingWhitespace,
                             d->storageSettings->ignoreFileTypes },
        d->storageSettings->addFinalNewLine,
    }.attachTo(d->groupBoxStorageSettings);

    Row {
        Form {
            d->encodingSettings->defaultEncoding, br,
            d->encodingSettings->utf8BomSetting, br,
            d->encodingSettings->lineEndingSetting, br,
        }, st
    }.attachTo(d->groupBoxEncodings);

    Row {
        Form {
            d->behaviorSettings->mouseHiding, br,
            d->behaviorSettings->mouseNavigation, br,
            d->behaviorSettings->scrollWheelZooming, br,
            d->behaviorSettings->camelCaseNavigation, br,
            d->behaviorSettings->smartSelectionChanging, br,
            d->behaviorSettings->keyboardTooltips, br,
            d->behaviorSettings->constrainHoverTooltips, br
        }, st
    }.attachTo(d->groupBoxMouse);

    Column {
        d->tabPreferencesWidget,
        d->groupBoxTyping,
        d->groupBoxStorageSettings,
        d->groupBoxEncodings,
        d->groupBoxMouse,
        st,
        noMargin,
    }.attachTo(this);
}

BehaviorSettingsWidget::~BehaviorSettingsWidget()
{
    delete d;
}

bool BehaviorSettingsWidget::isDirty() const
{
    return d->typingSettings->isDirty()
        || d->behaviorSettings->isDirty()
        || d->storageSettings->isDirty()
        || d->encodingSettings->isDirty();
}

void BehaviorSettingsWidget::apply()
{
    d->typingSettings->apply();
    d->behaviorSettings->apply();
    d->storageSettings->apply();
    d->encodingSettings->apply();
}

void BehaviorSettingsWidget::setActive(bool active)
{
    d->tabPreferencesWidget->setEnabled(active);
    d->groupBoxTyping->setEnabled(active);
    d->groupBoxEncodings->setEnabled(active);
    d->groupBoxMouse->setEnabled(active);
    d->groupBoxStorageSettings->setEnabled(active);
}

void BehaviorSettingsWidget::setCodeStyle(ICodeStylePreferences *preferences)
{
    d->tabPreferencesWidget->setPreferences(preferences);
}

TabSettingsWidget *BehaviorSettingsWidget::tabSettingsWidget() const
{
    return d->tabPreferencesWidget->tabSettingsWidget();
}

} // TextEditor
