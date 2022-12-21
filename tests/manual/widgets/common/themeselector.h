// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include<QComboBox>

namespace ManualTest {

class ThemeSelector : public QComboBox {
public:
    ThemeSelector(QWidget *parent = nullptr);

    static void setTheme(const QString &themeFile);
};

} // namespace ManualTest
