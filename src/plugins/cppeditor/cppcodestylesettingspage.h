// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppcodestylesettings.h"

#include <QWidget>

namespace CppEditor {

namespace Internal {  class CppCodeStylePreferencesWidgetPrivate;  }

class CPPEDITOR_EXPORT CppCodeStylePreferencesWidget : public QWidget
{
public:
    explicit CppCodeStylePreferencesWidget(CppCodeStylePreferences *codeStylePreferences);
    ~CppCodeStylePreferencesWidget() override;

    void apply();
    void cancel();

private:
    Internal::CppCodeStylePreferencesWidgetPrivate *d = nullptr;
};

namespace Internal { void setupCppCodeStyleSettings();  }

} // namespace CppEditor
