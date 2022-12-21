// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include <QWidget>

namespace TextEditor {
class ICodeStylePreferences;
}

namespace QmlJSTools {
class QmlJSCodeStyleSettings;
class QmlJSCodeStyleSettingsWidget;
class QmlJSCodeStylePreferences;

class QMLJSTOOLS_EXPORT QmlJSCodeStylePreferencesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QmlJSCodeStylePreferencesWidget(QWidget *parent = nullptr);

    void setPreferences(QmlJSCodeStylePreferences *tabPreferences);

private:
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences* preferences);
    void slotSettingsChanged(const QmlJSCodeStyleSettings &settings);

    QmlJSCodeStyleSettingsWidget *m_codeStyleSettingsWidget;
    QmlJSCodeStylePreferences *m_preferences = nullptr;
};

} // namespace QmlJSTools
