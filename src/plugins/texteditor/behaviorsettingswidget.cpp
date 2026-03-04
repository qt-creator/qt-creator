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

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

namespace TextEditor {

BehaviorSettingsWidget::BehaviorSettingsWidget(TypingSettings *typingSettings,
                                               StorageSettings *storageSettings,
                                               BehaviorSettings *behaviorSettings,
                                               ExtraEncodingSettings *encodingSettings,
                                               QWidget *parent)
    : QWidget(parent),
      typingSettings(typingSettings),
      storageSettings(storageSettings),
      behaviorSettings(behaviorSettings),
      encodingSettings(encodingSettings)
{
    tabPreferencesWidget = new SimpleCodeStylePreferencesWidget(this);

    using namespace Layouting;

    Group typing {
        title(Tr::tr("Typing")),
        Row {
            Form {
                typingSettings->autoIndent, br,
                typingSettings->smartBackspaceBehavior, br,
                typingSettings->tabKeyBehavior, br,
                typingSettings->preferSingleLineComments, br,
                typingSettings->commentPosition, br
            }, st
        }
    };

    Group storage {
        title(Tr::tr("Cleanups Upon Saving")),
        Layouting::toolTip(Tr::tr("Cleanup actions which are automatically performed "
                                  "right before the file is saved to disk.")),
        Column {
            storageSettings->cleanWhitespace,
                Row { Space(30), storageSettings->inEntireDocument },
                Row { Space(30), storageSettings->cleanIndentation },
                Row { Space(30), storageSettings->skipTrailingWhitespace,
                                 storageSettings->ignoreFileTypes },
            storageSettings->addFinalNewLine,
        }
    };

    Group encoding {
        title(Tr::tr("File Encodings")),
        Row {
            Form {
                encodingSettings->defaultEncoding, br,
                encodingSettings->utf8BomSetting, br,
                encodingSettings->lineEndingSetting, br,
            }, st
        }
    };

    Group mouse {
        title(Tr::tr("Mouse and Keyboard")),
        Row {
            Form {
                behaviorSettings->mouseHiding, br,
                behaviorSettings->mouseNavigation, br,
                behaviorSettings->scrollWheelZooming, br,
                behaviorSettings->camelCaseNavigation, br,
                behaviorSettings->smartSelectionChanging, br,
                behaviorSettings->keyboardTooltips, br,
                behaviorSettings->constrainHoverTooltips, br
            }, st
        }
    };

    Column {
        tabPreferencesWidget,
        typing,
        storage,
        encoding,
        mouse,
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
