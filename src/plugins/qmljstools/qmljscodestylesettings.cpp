// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscodestylesettings.h"

#include "qmljscodestylesettings.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolssettings.h"

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/tabsettings.h>

#include <cplusplus/Overview.h>

#include <utils/qtcassert.h>

static const char lineLengthKey[] = "LineLength";
static const char qmlformatIniContentKey[] = "QmlFormatIniContent";
static const char formatterKey[] = "Formatter";
static const char customFormatterPathKey[] = "CustomFormatterPath";
static const char customFormatterArgumentsKey[] = "CustomFormatterArguments";

using namespace Utils;
namespace QmlJSTools {

// QmlJSCodeStyleSettings

QmlJSCodeStyleSettings::QmlJSCodeStyleSettings() = default;

Store QmlJSCodeStyleSettings::toMap() const
{
    return {
        {formatterKey, formatter},
        {lineLengthKey, lineLength},
        {qmlformatIniContentKey, qmlformatIniContent},
        {customFormatterPathKey, customFormatterPath.toUrlishString()},
        {customFormatterArgumentsKey, customFormatterArguments}
    };
}

void QmlJSCodeStyleSettings::fromMap(const Store &map)
{
    lineLength = map.value(lineLengthKey, lineLength).toInt();
    qmlformatIniContent = map.value(qmlformatIniContentKey, qmlformatIniContent).toString();
    formatter = static_cast<Formatter>(map.value(formatterKey, formatter).toInt());
    customFormatterPath = Utils::FilePath::fromString(map.value(customFormatterPathKey).toString());
    customFormatterArguments = map.value(customFormatterArgumentsKey).toString();
}

bool QmlJSCodeStyleSettings::equals(const QmlJSCodeStyleSettings &rhs) const
{
    return lineLength == rhs.lineLength && qmlformatIniContent == rhs.qmlformatIniContent
           && formatter == rhs.formatter && customFormatterPath == rhs.customFormatterPath
           && customFormatterArguments == rhs.customFormatterArguments;
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

Id QmlJSCodeStyleSettings::settingsId()
{
    return Constants::QML_JS_CODE_STYLE_SETTINGS_ID;
}

} // namespace QmlJSTools
