// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "icodestylepreferences.h"

namespace TextEditor {

class TEXTEDITOR_EXPORT SimpleCodeStylePreferences : public ICodeStylePreferences
{
public:
    explicit SimpleCodeStylePreferences(QObject *parentObject = nullptr);

    QVariant value() const override;
    void setValue(const QVariant &) override;
};

} // namespace TextEditor
