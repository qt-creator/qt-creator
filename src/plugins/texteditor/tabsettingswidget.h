// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/aspects.h>

namespace TextEditor {

class TabSettings;

class TEXTEDITOR_EXPORT TabSettingsWidget : public Utils::AspectContainer
{
    Q_OBJECT

public:
    enum CodingStyleLink {
        CppLink,
        QtQuickLink
    };

    TabSettingsWidget();
    ~TabSettingsWidget() override;

    TabSettings tabSettings() const;

    void setCodingStyleWarningVisible(bool visible);
    void setTabSettings(const TextEditor::TabSettings& s);

signals:
    void settingsChanged(const TextEditor::TabSettings &);
    void codingStyleLinkClicked(TextEditor::TabSettingsWidget::CodingStyleLink link);

private:
    void addToLayoutImpl(Layouting::Layout &parent) override;
    void codingStyleLinkActivated(const QString &linkString);

    Utils::BoolAspect autoDetect{this};
    Utils::SelectionAspect tabPolicy{this};
    Utils::IntegerAspect tabSize{this};
    Utils::IntegerAspect indentSize{this};
    Utils::SelectionAspect continuationAlignBehavior{this};

    QLabel *m_codingStyleWarning;
};

} // namespace TextEditor
