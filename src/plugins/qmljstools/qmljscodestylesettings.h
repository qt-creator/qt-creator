// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include <utils/store.h>

#include <optional>

namespace TextEditor { class TabSettings; }

namespace QmlJSTools {

class QMLJSTOOLS_EXPORT QmlJSCodeStyleSettings
{
public:
    QmlJSCodeStyleSettings();

    int lineLength = 80;

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);

    bool equals(const QmlJSCodeStyleSettings &rhs) const;
    bool operator==(const QmlJSCodeStyleSettings &s) const { return equals(s); }
    bool operator!=(const QmlJSCodeStyleSettings &s) const { return !equals(s); }

    static QmlJSCodeStyleSettings currentGlobalCodeStyle();
    static TextEditor::TabSettings currentGlobalTabSettings();
};

} // namespace CppEditor

Q_DECLARE_METATYPE(QmlJSTools::QmlJSCodeStyleSettings)
