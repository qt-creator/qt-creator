// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/aspects.h>

namespace TextEditor {

class ICodeStylePreferences;
class TabSettingsData;

class TEXTEDITOR_EXPORT TabSettings : public Utils::AspectContainer
{
    Q_OBJECT

public:
    enum CodingStyleLink {
        CppLink,
        QtQuickLink
    };

    TabSettings();
    ~TabSettings() override;

    void setPreferences(ICodeStylePreferences *preferences);

    TabSettingsData data() const;

    void setCodingStyleWarningVisible(bool visible);
    void setData(const TextEditor::TabSettingsData& s);

signals:
    void settingsChanged(const TextEditor::TabSettingsData &);
    void codingStyleLinkClicked(TextEditor::TabSettings::CodingStyleLink link);

private:
    void codingStyleLinkActivated(const QString &linkString);
    void slotCurrentPreferencesChanged(ICodeStylePreferences *preferences);
    void slotTabSettingsChanged(const TextEditor::TabSettingsData &settings);

    ICodeStylePreferences *m_preferences = nullptr;

    Utils::BoolAspect autoDetect{this};
    Utils::SelectionAspect tabPolicy{this};
    Utils::IntegerAspect tabSize{this};
    Utils::IntegerAspect indentSize{this};
    Utils::SelectionAspect continuationAlignBehavior{this};

    QLabel *m_codingStyleWarning;
};

} // namespace TextEditor
