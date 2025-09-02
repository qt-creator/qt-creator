// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include <texteditor/icodestylepreferences.h>

#include <utils/filepath.h>
#include <utils/store.h>

namespace TextEditor { class TabSettings; }
namespace Utils { class Id; }

namespace QmlJSTools {

class QMLJSTOOLS_EXPORT QmlJSCodeStyleSettings
{
public:
    QmlJSCodeStyleSettings();

    enum Formatter {
        Builtin,
        QmlFormat,
        Custom
    };

    int lineLength = 80;
    QString qmlformatIniContent;
    Formatter formatter = Builtin;
    Utils::FilePath customFormatterPath;
    QString customFormatterArguments;

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);

    bool equals(const QmlJSCodeStyleSettings &rhs) const;
    bool operator==(const QmlJSCodeStyleSettings &s) const { return equals(s); }
    bool operator!=(const QmlJSCodeStyleSettings &s) const { return !equals(s); }

    static QmlJSCodeStyleSettings currentGlobalCodeStyle();
    static TextEditor::TabSettings currentGlobalTabSettings();
    static Utils::Id settingsId();
};

using QmlJSCodeStylePreferences = TextEditor::TypedCodeStylePreferences<QmlJSCodeStyleSettings>;

} // namespace QmlJSTools

Q_DECLARE_METATYPE(QmlJSTools::QmlJSCodeStyleSettings)
