// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "simplecodestylepreferences.h"
#include "texteditorconstants.h"

#include <QVariant>

namespace TextEditor {

SimpleCodeStylePreferences::SimpleCodeStylePreferences(QObject *parent)
    : ICodeStylePreferences(parent)
{
    setSettingsSuffix("TabPreferences");
    setGlobalSettingsCategory(Constants::TEXT_EDITOR_BEHAVIOR_SETTINGS);
}

QVariant SimpleCodeStylePreferences::value() const
{
    return QVariant();
}

void SimpleCodeStylePreferences::setValue(const QVariant &value)
{
    Q_UNUSED(value)
}

} // TextEditor
