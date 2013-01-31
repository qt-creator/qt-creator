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

#include "cppcodestylepreferences.h"

using namespace CppTools;

static const char settingsSuffixKey[] = "CodeStyleSettings";

CppCodeStylePreferences::CppCodeStylePreferences(QObject *parent) :
    ICodeStylePreferences(parent)
{
    connect(this, SIGNAL(currentValueChanged(QVariant)),
            this, SLOT(slotCurrentValueChanged(QVariant)));
}

QVariant CppCodeStylePreferences::value() const
{
    QVariant v;
    v.setValue(codeStyleSettings());
    return v;
}

void CppCodeStylePreferences::setValue(const QVariant &data)
{
    if (!data.canConvert<CppCodeStyleSettings>())
        return;

    setCodeStyleSettings(data.value<CppCodeStyleSettings>());
}

CppCodeStyleSettings CppCodeStylePreferences::codeStyleSettings() const
{
    return m_data;
}

void CppCodeStylePreferences::setCodeStyleSettings(const CppCodeStyleSettings &data)
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

CppCodeStyleSettings CppCodeStylePreferences::currentCodeStyleSettings() const
{
    QVariant v = currentValue();
    if (!v.canConvert<CppCodeStyleSettings>()) {
        // warning
        return CppCodeStyleSettings();
    }
    return v.value<CppCodeStyleSettings>();
}

void CppCodeStylePreferences::slotCurrentValueChanged(const QVariant &value)
{
    if (!value.canConvert<CppCodeStyleSettings>())
        return;

    emit currentCodeStyleSettingsChanged(value.value<CppCodeStyleSettings>());
}

QString CppCodeStylePreferences::settingsSuffix() const
{
    return QLatin1String(settingsSuffixKey);
}

void CppCodeStylePreferences::toMap(const QString &prefix, QVariantMap *map) const
{
    ICodeStylePreferences::toMap(prefix, map);
    if (currentDelegate())
        return;

    m_data.toMap(prefix, map);
}

void CppCodeStylePreferences::fromMap(const QString &prefix, const QVariantMap &map)
{
    ICodeStylePreferences::fromMap(prefix, map);
    if (currentDelegate())
        return;

    m_data.fromMap(prefix, map);
}

