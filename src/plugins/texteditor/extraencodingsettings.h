// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/store.h>

namespace TextEditor {

class TEXTEDITOR_EXPORT ExtraEncodingSettings
{
public:
    ExtraEncodingSettings();
    ~ExtraEncodingSettings();

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);

    bool equals(const ExtraEncodingSettings &s) const;

    friend bool operator==(const ExtraEncodingSettings &a, const ExtraEncodingSettings &b)
    { return a.equals(b); }

    friend bool operator!=(const ExtraEncodingSettings &a, const ExtraEncodingSettings &b)
    { return !a.equals(b); }

    static QStringList lineTerminationModeNames();

    enum Utf8BomSetting {
        AlwaysAdd = 0,
        OnlyKeep = 1,
        AlwaysDelete = 2
    };
    Utf8BomSetting m_utf8BomSetting;
};

} // TextEditor
