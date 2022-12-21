// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT CommentsSettings
{
public:
    CommentsSettings();

    void toSettings(QSettings *s) const;
    void fromSettings(QSettings *s);

    bool equals(const CommentsSettings &other) const;

    friend bool operator==(const CommentsSettings &a, const CommentsSettings &b)
    { return a.equals(b); }

    friend bool operator!=(const CommentsSettings &a, const CommentsSettings &b)
    { return !(a == b); }

    bool m_enableDoxygen;
    bool m_generateBrief;
    bool m_leadingAsterisks;
};

} // namespace TextEditor
