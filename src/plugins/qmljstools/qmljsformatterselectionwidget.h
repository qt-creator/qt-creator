// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljscodestylesettings.h"

#include <utils/aspects.h>

namespace QmlJSTools {
class QmlJSCodeStyleSettings;

class QmlCodeStyleWidgetBase : public QWidget
{
    Q_OBJECT
public:
    QmlCodeStyleWidgetBase(QWidget *parent = nullptr);
    virtual ~QmlCodeStyleWidgetBase() = default;
    enum Formatter {
        Builtin,
        QmlFormat,
        Custom,
        Count
    };
    virtual void setCodeStyleSettings(const QmlJSCodeStyleSettings &s) = 0;
    virtual void setPreferences(QmlJSCodeStylePreferences *preferences) = 0;
    virtual void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *preferences) = 0;

signals:
    void settingsChanged(const QmlJSCodeStyleSettings &);
};
class FormatterSelectionWidget : public QmlCodeStyleWidgetBase
{
    Q_OBJECT
public:
    explicit FormatterSelectionWidget(QWidget *parent);

    const Utils::SelectionAspect &selection() const { return m_formatterSelection; }
    Utils::SelectionAspect &selection() { return m_formatterSelection; }

    void setCodeStyleSettings(const QmlJSCodeStyleSettings &s) override;
    void setPreferences(QmlJSCodeStylePreferences *preferences) override;
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *preferences) override;

private:
    void slotSettingsChanged();
    Utils::SelectionAspect m_formatterSelection;
    QmlJSCodeStylePreferences *m_preferences = nullptr;
};

} // namespace QmlJSTools
