// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/store.h>

namespace TextEditor {

class TEXTEDITOR_EXPORT MarginSettings
{
public:
    MarginSettings();

    void toSettings(Utils::QtcSettings *s) const;
    void fromSettings(Utils::QtcSettings *s);

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);

    bool equals(const MarginSettings &other) const;

    friend bool operator==(const MarginSettings &one, const MarginSettings &two)
    { return one.equals(two); }
    friend bool operator!=(const MarginSettings &one, const MarginSettings &two)
    { return !one.equals(two); }

    bool m_showMargin;
    bool m_tintMarginArea;
    bool m_useIndenter;
    int m_marginColumn;
};

} // namespace TextEditor
