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

#include "qmljscodestylepreferences.h"

namespace QmlJSTools {

QmlJSCodeStylePreferences::QmlJSCodeStylePreferences(QObject *parent) :
      ICodeStylePreferences(parent)
{
    setSettingsSuffix("CodeStyleSettings");

    connect(this, &QmlJSCodeStylePreferences::currentValueChanged,
            this, &QmlJSCodeStylePreferences::slotCurrentValueChanged);
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
    emit codeStyleSettingsChanged(m_data);
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

void QmlJSCodeStylePreferences::slotCurrentValueChanged(const QVariant &value)
{
    if (!value.canConvert<QmlJSCodeStyleSettings>())
        return;

    emit currentCodeStyleSettingsChanged(value.value<QmlJSCodeStyleSettings>());
}

QVariantMap QmlJSCodeStylePreferences::toMap() const
{
    QVariantMap map = ICodeStylePreferences::toMap();
    if (!currentDelegate()) {
        const QVariantMap dataMap = m_data.toMap();
        for (auto it = dataMap.begin(), end = dataMap.end(); it != end; ++it)
            map.insert(it.key(), it.value());
    }
    return map;
}

void QmlJSCodeStylePreferences::fromMap(const QVariantMap &map)
{
    ICodeStylePreferences::fromMap(map);
    if (!currentDelegate())
        m_data.fromMap(map);
}

} // namespace QmlJSTools
