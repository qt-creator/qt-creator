// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscodestylepreferences.h"
#include "qmljstoolsconstants.h"

using namespace Utils;

namespace QmlJSTools {

QmlJSCodeStylePreferences::QmlJSCodeStylePreferences(QObject *parent) :
      ICodeStylePreferences(parent)
{
    setSettingsSuffix("CodeStyleSettings");
    setGlobalSettingsCategory(Constants::QML_JS_CODE_STYLE_SETTINGS_ID);
}

QVariant QmlJSCodeStylePreferences::value() const
{
    QVariant v;
    v.setValue(codeStyleSettings());
    return v;
}

void QmlJSCodeStylePreferences::setValue(const QVariant &data)
{
    if (!data.canConvert<QmlJSCodeStyleSettings>())
        return;

    setCodeStyleSettings(data.value<QmlJSCodeStyleSettings>());
}

QmlJSCodeStyleSettings QmlJSCodeStylePreferences::codeStyleSettings() const
{
    return m_data;
}

void QmlJSCodeStylePreferences::setCodeStyleSettings(const QmlJSCodeStyleSettings &data)
{
    if (m_data == data)
        return;

    m_data = data;

    QVariant v;
    v.setValue(data);
    emit valueChanged(v);
    if (!currentDelegate())
        emit currentValueChanged(v);
}

QmlJSCodeStyleSettings QmlJSCodeStylePreferences::currentCodeStyleSettings() const
{
    QVariant v = currentValue();
    if (!v.canConvert<QmlJSCodeStyleSettings>()) {
        // warning
        return {};
    }
    return v.value<QmlJSCodeStyleSettings>();
}

Store QmlJSCodeStylePreferences::toMap() const
{
    Store map = ICodeStylePreferences::toMap();
    if (!currentDelegate()) {
        const Store dataMap = m_data.toMap();
        for (auto it = dataMap.begin(), end = dataMap.end(); it != end; ++it)
            map.insert(it.key(), it.value());
    }
    return map;
}

void QmlJSCodeStylePreferences::fromMap(const Store &map)
{
    ICodeStylePreferences::fromMap(map);
    if (!currentDelegate())
        m_data.fromMap(map);
}

} // namespace QmlJSTools
