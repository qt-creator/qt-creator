// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/aspects.h>
#include <utils/store.h>

namespace TextEditor {

class TEXTEDITOR_EXPORT MarginSettingsData
{
public:
    MarginSettingsData() = default;

    bool m_showMargin = false;
    bool m_tintMarginArea = true;
    bool m_useIndenter = false;
    int m_marginColumn = 80;
    int m_centerEditorContentWidthPercent = 100;
};

class TEXTEDITOR_EXPORT MarginSettings : public Utils::AspectContainer
{
public:
    explicit MarginSettings(const Utils::Key &keyPrefix = {});

    MarginSettingsData data() const;
    void setData(const MarginSettingsData &data);

    Utils::BoolAspect showMargin{this};
    Utils::BoolAspect tintMarginArea{this};
    Utils::BoolAspect useIndenter{this};
    Utils::IntegerAspect marginColumn{this};
    Utils::IntegerAspect centerEditorContentWidthPercent{this};
};

TEXTEDITOR_EXPORT MarginSettings &marginSettings();

} // namespace TextEditor
