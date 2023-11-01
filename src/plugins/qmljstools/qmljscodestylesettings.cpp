// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscodestylesettings.h"

#include "qmljscodestylepreferences.h"
#include "qmljstoolssettings.h"

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/tabsettings.h>

#include <cplusplus/Overview.h>

#include <utils/qtcassert.h>

static const char lineLengthKey[] = "LineLength";

using namespace Utils;

namespace QmlJSTools {

// QmlJSCodeStyleSettings

QmlJSCodeStyleSettings::QmlJSCodeStyleSettings() = default;

Store QmlJSCodeStyleSettings::toMap() const
{
    return {
        {lineLengthKey, lineLength}
    };
}

void QmlJSCodeStyleSettings::fromMap(const Store &map)
{
    lineLength = map.value(lineLengthKey, lineLength).toInt();
}

bool QmlJSCodeStyleSettings::equals(const QmlJSCodeStyleSettings &rhs) const
{
    return lineLength == rhs.lineLength;
}

QmlJSCodeStyleSettings QmlJSCodeStyleSettings::currentGlobalCodeStyle()
{
    QmlJSCodeStylePreferences *QmlJSCodeStylePreferences = QmlJSToolsSettings::globalCodeStyle();
    QTC_ASSERT(QmlJSCodeStylePreferences, return QmlJSCodeStyleSettings());

    return QmlJSCodeStylePreferences->currentCodeStyleSettings();
}

TextEditor::TabSettings QmlJSCodeStyleSettings::currentGlobalTabSettings()
{
    QmlJSCodeStylePreferences *QmlJSCodeStylePreferences = QmlJSToolsSettings::globalCodeStyle();
    QTC_ASSERT(QmlJSCodeStylePreferences, return TextEditor::TabSettings());

    return QmlJSCodeStylePreferences->currentTabSettings();
}

} // namespace QmlJSTools
