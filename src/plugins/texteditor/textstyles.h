// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditorconstants.h"

#include <utils/sizedarray.h>

#include <QList>

namespace TextEditor {
using MixinTextStyles = Utils::SizedArray<TextStyle, 6>;

struct TextStyles {
    TextStyle mainStyle = C_TEXT;
    MixinTextStyles mixinStyles;

    static TextStyles mixinStyle(TextStyle main, const QList<TextStyle> &mixins)
    {
        TextStyles res;
        res.mainStyle = main;
        res.mixinStyles.initializeElements();
        for (TextStyle mixin : mixins)
            res.mixinStyles.push_back(mixin);
        return res;
    }

    static TextStyles mixinStyle(TextStyle main, TextStyle mixin)
    {
        return mixinStyle(main, QList<TextStyle>{mixin});
    }
};

} // namespace TextEditor
